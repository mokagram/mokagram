// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: ложные пароли экрана блокировки.
//
// Файл tdata/dc_cache: соль, KDF, восемь блобов одинакового размера и
// список паролей, зашифрованный локальным ключом. Каждый блоб зашифрован
// ключом от своего ложного пароля, пустые места заняты случайными блобами,
// так что по файлу не видно, сколько паролей заведено.
#include "mokagram/passcode/mokagram_fake_passcode.h"

#include "base/random.h"
#include "base/timer.h"
#include "base/weak_ptr.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/version.h"
#include "data/data_session.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "mtproto/mtproto_auth_key.h"
#include "mtproto/mtproto_proxy_data.h"
#include "storage/cache/storage_cache_database.h"
#include "storage/details/storage_file_utilities.h"
#include "storage/storage_domain.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>

namespace Mokagram::FakePasscode {
namespace {

using namespace Storage::details;

constexpr auto kVersion = 1;
constexpr auto kSaltSize = 32;
constexpr auto kBlobPayload = 1024;
constexpr auto kLogoutTimeout = 10 * crl::time(1000);
constexpr auto kLogoutCheckEach = crl::time(300);

struct State {
	bool loaded = false;
	QByteArray salt;
	PasscodeKdf kdf;
	std::vector<QByteArray> blobs;
	std::vector<Entry> list;
	int pendingId = 0;
	bool busy = false;
};

struct Waiter {
	std::vector<base::weak_ptr<Main::Account>> accounts;
	Actions actions;
	crl::time started = 0;
};

[[nodiscard]] State &GetState() {
	static auto result = State();
	return result;
}

[[nodiscard]] Waiter &GetWaiter() {
	static auto result = Waiter();
	return result;
}

[[nodiscard]] base::Timer &GetWaitTimer() {
	static auto result = base::Timer();
	return result;
}

[[nodiscard]] QString BasePath() {
	return cWorkingDir() + u"tdata/"_q;
}

[[nodiscard]] QString FileName() {
	return u"dc_cache"_q;
}

[[nodiscard]] MTP::AuthKeyPtr LocalKey() {
	return Core::App().domain().started()
		? Core::App().domain().local().localKey()
		: nullptr;
}

[[nodiscard]] bool SameKdf(const PasscodeKdf &a, const PasscodeKdf &b) {
	return (a.kind == b.kind)
		&& (a.memory == b.memory)
		&& (a.time == b.time)
		&& (a.parallel == b.parallel);
}

[[nodiscard]] QByteArray PackKdf(const PasscodeKdf &kdf) {
	auto result = QByteArray();
	auto stream = QDataStream(&result, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_5_1);
	stream << kdf.kind << kdf.memory << kdf.time << kdf.parallel;
	return result;
}

[[nodiscard]] std::optional<PasscodeKdf> UnpackKdf(const QByteArray &data) {
	auto stream = QDataStream(data);
	stream.setVersion(QDataStream::Qt_5_1);
	auto result = PasscodeKdf();
	stream >> result.kind >> result.memory >> result.time >> result.parallel;
	if (stream.status() != QDataStream::Ok
		|| !result.valid()
		|| !result.costWithinLimits()) {
		return std::nullopt;
	}
	return result;
}

[[nodiscard]] QByteArray PackBlobs(const std::vector<QByteArray> &blobs) {
	auto result = QByteArray();
	auto stream = QDataStream(&result, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_5_1);
	stream << qint32(blobs.size());
	for (const auto &blob : blobs) {
		stream << blob;
	}
	return result;
}

[[nodiscard]] bool UnpackBlobs(
		const QByteArray &packed,
		std::vector<QByteArray> &blobs) {
	auto stream = QDataStream(packed);
	stream.setVersion(QDataStream::Qt_5_1);
	auto count = qint32();
	stream >> count;
	if (stream.status() != QDataStream::Ok || count != kMaxEntries) {
		return false;
	}
	blobs.resize(count);
	for (auto &blob : blobs) {
		stream >> blob;
		if (blob.size() != blobs.front().size()) {
			return false;
		}
	}
	return stream.status() == QDataStream::Ok && !blobs.front().isEmpty();
}

void WriteActions(QDataStream &stream, const Actions &actions) {
	const auto flags = (actions.clearCache ? 1 : 0)
		| (actions.clearProxies ? 2 : 0)
		| (actions.deleteAll ? 4 : 0);
	stream << qint32(flags) << qint32(actions.logoutAccounts.size());
	for (const auto index : actions.logoutAccounts) {
		stream << qint32(index);
	}
}

[[nodiscard]] bool ReadActions(QDataStream &stream, Actions &actions) {
	auto flags = qint32();
	auto logouts = qint32();
	stream >> flags >> logouts;
	if (stream.status() != QDataStream::Ok
		|| logouts < 0
		|| logouts > Main::Domain::kPremiumMaxAccounts) {
		return false;
	}
	actions.clearCache = (flags & 1) != 0;
	actions.clearProxies = (flags & 2) != 0;
	actions.deleteAll = (flags & 4) != 0;
	for (auto i = 0; i != logouts; ++i) {
		auto index = qint32();
		stream >> index;
		actions.logoutAccounts.push_back(index);
	}
	return stream.status() == QDataStream::Ok;
}

[[nodiscard]] QByteArray PackList(const std::vector<Entry> &list) {
	auto result = QByteArray();
	auto stream = QDataStream(&result, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_5_1);
	stream << qint32(kVersion) << qint32(list.size());
	for (const auto &entry : list) {
		stream
			<< qint32(entry.id)
			<< entry.name
			<< entry.passcode.toUtf8()
			<< qint32(entry.unlocks ? 1 : 0);
		WriteActions(stream, entry.actions);
	}
	return result;
}

[[nodiscard]] bool UnpackList(
		const QByteArray &packed,
		std::vector<Entry> &list) {
	auto stream = QDataStream(packed);
	stream.setVersion(QDataStream::Qt_5_1);
	auto version = qint32();
	auto count = qint32();
	stream >> version >> count;
	if (stream.status() != QDataStream::Ok
		|| version != kVersion
		|| count < 0
		|| count > kMaxEntries) {
		return false;
	}
	list.clear();
	for (auto i = 0; i != count; ++i) {
		auto entry = Entry();
		auto id = qint32();
		auto passcode = QByteArray();
		auto unlocks = qint32();
		stream >> id >> entry.name >> passcode >> unlocks;
		if (stream.status() != QDataStream::Ok
			|| !ReadActions(stream, entry.actions)) {
			return false;
		}
		entry.id = id;
		entry.passcode = QString::fromUtf8(passcode);
		entry.unlocks = (unlocks != 0);
		list.push_back(std::move(entry));
	}
	return stream.status() == QDataStream::Ok;
}

struct Plain {
	QByteArray salt;
	PasscodeKdf kdf;
	std::vector<QByteArray> blobs;
	QByteArray encrypted;
};

[[nodiscard]] std::optional<Plain> ReadPlain() {
	auto file = FileReadDescriptor();
	if (!ReadFile(file, FileName(), BasePath())) {
		return std::nullopt;
	}
	auto result = Plain();
	auto packed = QByteArray();
	auto kdf = QByteArray();
	file.stream >> result.salt >> kdf >> packed >> result.encrypted;
	if (!CheckStreamStatus(file.stream)
		|| result.salt.size() != kSaltSize
		|| !UnpackBlobs(packed, result.blobs)) {
		return std::nullopt;
	}
	const auto parsed = UnpackKdf(kdf);
	if (!parsed) {
		return std::nullopt;
	}
	result.kdf = *parsed;
	return result;
}

void RemoveFile() {
	const auto base = BasePath() + FileName();
	QFile::remove(base + 's');
	QFile::remove(base + '0');
	QFile::remove(base + '1');
}

void Load() {
	auto &state = GetState();
	if (state.loaded) {
		return;
	}
	const auto key = LocalKey();
	if (!key) {
		return;
	}
	state.loaded = true;
	auto plain = ReadPlain();
	if (!plain) {
		return;
	}
	auto decrypted = EncryptedDescriptor();
	auto packed = QByteArray();
	if (!DecryptLocal(decrypted, plain->encrypted, key)) {
		return;
	}
	decrypted.stream >> packed;
	auto list = std::vector<Entry>();
	if (!UnpackList(packed, list)) {
		return;
	}
	state.salt = plain->salt;
	state.kdf = plain->kdf;
	state.blobs = std::move(plain->blobs);
	state.list = std::move(list);
}

[[nodiscard]] QByteArray MakeBlobWith(
		const Entry &entry,
		const QByteArray &realPasscode,
		const MTP::AuthKeyPtr &key) {
	auto inner = QByteArray();
	{
		auto stream = QDataStream(&inner, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << qint32(entry.id) << (entry.unlocks
			? realPasscode
			: QByteArray());
		// Пароль, который открывает клиент, хранит действия в списке:
		// их прочитают уже после разблокировки.
		WriteActions(stream, entry.unlocks ? Actions() : entry.actions);
	}
	const auto room = kBlobPayload - int(inner.size()) - 4;
	if (room < 0) {
		return QByteArray();
	}
	auto padding = QByteArray(room, Qt::Uninitialized);
	base::RandomFill(padding.data(), padding.size());
	auto blob = EncryptedDescriptor(kBlobPayload);
	blob.stream.writeRawData(inner.constData(), int(inner.size()));
	blob.stream << padding;
	return PrepareEncrypted(blob, key);
}

// Потокобезопасно: ключ от пароля по той же схеме, что у настоящего.
[[nodiscard]] QByteArray MakeBlob(
		const Entry &entry,
		const QByteArray &realPasscode,
		const QByteArray &salt,
		const PasscodeKdf &kdf) {
	const auto key = CreatePasscodeKey(entry.passcode.toUtf8(), salt, kdf);
	return key ? MakeBlobWith(entry, realPasscode, key) : QByteArray();
}

[[nodiscard]] QByteArray MakeDecoy() {
	auto data = MTP::AuthKey::Data();
	base::RandomFill(data.data(), data.size());
	return MakeBlobWith(
		Entry(),
		QByteArray(),
		std::make_shared<MTP::AuthKey>(data));
}

[[nodiscard]] bool WriteFile() {
	auto &state = GetState();
	const auto key = LocalKey();
	if (!key || state.salt.size() != kSaltSize) {
		return false;
	}
	while (state.blobs.size() < size_t(kMaxEntries)) {
		state.blobs.push_back(MakeDecoy());
	}
	const auto packed = PackList(state.list);
	QDir().mkpath(BasePath());
	auto file = FileWriteDescriptor(FileName(), BasePath(), true);
	file.writeData(state.salt);
	file.writeData(PackKdf(state.kdf));
	file.writeData(PackBlobs(state.blobs));
	auto encrypted = EncryptedDescriptor(packed.size() + sizeof(quint32));
	encrypted.stream << packed;
	file.writeEncrypted(encrypted, key);
	return file.finish();
}

[[nodiscard]] QString DefaultDownloadsDirectory() {
	return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
		+ '/'
		+ AppName.utf16()
		+ '/';
}

void RemoveDownloads() {
	if (Core::App().settings().downloadPath().isEmpty()
		&& Core::App().canReadDefaultDownloadPath()) {
		QDir(DefaultDownloadsDirectory()).removeRecursively();
	}
}

void ClearCache() {
	for (const auto &entry : Core::App().domain().accounts()) {
		if (const auto session = entry.account->maybeSession()) {
			session->data().cache().clear();
			session->data().cacheBigFile().clear();
		}
	}
	RemoveDownloads();
}

void RemoveFiles(const QStringList &patterns, bool directories) {
	const auto dir = QDir(BasePath());
	const auto filter = directories
		? (QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)
		: (QDir::Files | QDir::Hidden);
	for (const auto &info : dir.entryInfoList(patterns, filter)) {
		if (directories) {
			QDir(info.absoluteFilePath()).removeRecursively();
		} else {
			QFile::remove(info.absoluteFilePath());
		}
	}
}

void WipeCacheFiles() {
	RemoveFiles({ u"user_data*"_q }, true);
	RemoveDownloads();
}

// Клиент ещё не открыт: стираем сессии прямо с диска.
void WipeSessionFiles() {
	const auto hex = u"[0-9A-F]"_q.repeated(16);
	RemoveFiles({
		u"key_data*"_q,
		FileName() + '*',
		hex + '*',
	}, false);
	RemoveFiles({ hex, u"account_*"_q }, true);
	WipeCacheFiles();
}

void ClearProxies() {
	auto &proxy = Core::App().settings().proxy();
	if (proxy.list().empty() && !proxy.selected()) {
		return;
	}
	proxy.setList({});
	proxy.setProxyRotationPreferredIndices({});
	Core::App().setCurrentProxy(
		MTP::ProxyData(),
		MTP::ProxyData::Settings::System);
	Core::App().saveSettingsDelayed();
}

void FinishActions(const Actions &actions) {
	if (actions.clearProxies) {
		ClearProxies();
	}
	if (actions.deleteAll) {
		GetState() = State{ .loaded = true };
		RemoveFile();
	}
}

void CheckLoggedOut() {
	auto &waiter = GetWaiter();
	const auto timedOut = (crl::now() - waiter.started) > kLogoutTimeout;
	auto waiting = false;
	for (const auto &weak : waiter.accounts) {
		const auto account = weak.get();
		if (!account || !account->sessionExists()) {
			continue;
		} else if (timedOut) {
			account->forcedLogOut();
		} else {
			waiting = true;
		}
	}
	if (waiting) {
		return;
	}
	GetWaitTimer().cancel();
	const auto actions = base::take(waiter.actions);
	waiter.accounts.clear();
	FinishActions(actions);
}

void Run(const Actions &actions) {
	if (actions.clearCache || actions.deleteAll) {
		ClearCache();
	}
	auto &waiter = GetWaiter();
	waiter = Waiter();
	for (const auto &entry : Core::App().domain().accounts()) {
		if (entry.account->maybeSession()
			&& (actions.deleteAll
				|| ranges::contains(actions.logoutAccounts, entry.index))) {
			waiter.accounts.push_back(base::make_weak(entry.account.get()));
		}
	}
	for (const auto &weak : waiter.accounts) {
		if (const auto account = weak.get()) {
			account->logOut();
		}
	}
	if (waiter.accounts.empty()) {
		FinishActions(actions);
		return;
	}
	waiter.actions = actions;
	waiter.started = crl::now();
	GetWaitTimer().setCallback(CheckLoggedOut);
	GetWaitTimer().callEach(kLogoutCheckEach);
}

[[nodiscard]] std::optional<Match> MatchWithKey(
		const std::vector<QByteArray> &blobs,
		const MTP::AuthKeyPtr &key) {
	if (!key) {
		return std::nullopt;
	}
	auto found = std::optional<Match>();
	// Расшифровываем все блобы, даже после находки: время ответа одно.
	for (const auto &blob : blobs) {
		auto decrypted = EncryptedDescriptor();
		if (!DecryptLocal(decrypted, blob, key) || found) {
			continue;
		}
		auto result = Match();
		auto id = qint32();
		decrypted.stream >> id >> result.realPasscode;
		if (decrypted.stream.status() == QDataStream::Ok
			&& ReadActions(decrypted.stream, result.actions)) {
			result.id = id;
			found = std::move(result);
		}
	}
	return found;
}

// Все блобы заново: новая соль, текущий KDF, настоящий пароль.
void RebuildAsync(const QByteArray &realPasscode, Fn<void(bool)> done) {
	auto &state = GetState();
	auto salt = QByteArray(kSaltSize, Qt::Uninitialized);
	base::RandomFill(salt.data(), salt.size());
	const auto kdf = DefaultPasscodeKdf();
	const auto entries = state.list;
	state.busy = true;
	crl::async([=] {
		auto blobs = std::vector<QByteArray>();
		for (const auto &entry : entries) {
			auto blob = MakeBlob(entry, realPasscode, salt, kdf);
			if (blob.isEmpty()) {
				blobs.clear();
				break;
			}
			blobs.push_back(std::move(blob));
		}
		crl::on_main([=] {
			auto &state = GetState();
			state.busy = false;
			if (blobs.size() != entries.size()
				|| PackList(entries) != PackList(state.list)) {
				done(false);
				return;
			}
			state.salt = salt;
			state.kdf = kdf;
			state.blobs = blobs;
			done(WriteFile());
		});
	});
}

} // namespace

void MatchAsync(
		const QByteArray &passcode,
		Fn<void(std::optional<Match>)> done) {
	auto plain = ReadPlain();
	if (!plain) {
		// Файла нет: всё равно платим ту же цену, чтобы время не выдало.
		plain = Plain{
			.salt = QByteArray(kSaltSize, '\0'),
			.kdf = DefaultPasscodeKdf(),
		};
	}
	crl::async([=, plain = std::move(*plain)] {
		const auto key = CreatePasscodeKey(passcode, plain.salt, plain.kdf);
		auto result = MatchWithKey(plain.blobs, key);
		crl::on_main([=, result = std::move(result)] {
			done(result);
		});
	});
}

void SetPending(int id) {
	GetState().pendingId = id;
}

void RunPending() {
	const auto id = base::take(GetState().pendingId);
	if (!id) {
		return;
	}
	crl::on_main([=] {
		Load();
		const auto &list = GetState().list;
		const auto i = ranges::find(list, id, &Entry::id);
		if (i != end(list)) {
			Run(i->actions);
		}
	});
}

void RunSilent(const Actions &actions) {
	if (Core::App().domain().started()) {
		Run(actions);
		return;
	}
	if (actions.deleteAll) {
		WipeSessionFiles();
	} else if (actions.clearCache) {
		WipeCacheFiles();
	}
	if (actions.clearProxies) {
		ClearProxies();
	}
}

std::vector<Entry> List() {
	Load();
	return GetState().list;
}

void Add(
		Entry entry,
		const QString &realPasscode,
		Fn<void(AddResult)> done) {
	Load();
	auto &state = GetState();
	if (!state.loaded || entry.passcode.isEmpty()) {
		done(AddResult::Empty);
		return;
	} else if (state.list.size() >= size_t(kMaxEntries)) {
		done(AddResult::TooMany);
		return;
	} else if (state.busy) {
		done(AddResult::Busy);
		return;
	} else if (entry.passcode == realPasscode) {
		done(AddResult::SameAsReal);
		return;
	} else if (ranges::contains(state.list, entry.passcode, &Entry::passcode)) {
		done(AddResult::Duplicate);
		return;
	}
	const auto real = realPasscode.toUtf8();
	state.busy = true;
	const auto started = Core::App().domain().tryPasscode(real, [=](
			bool correct) mutable {
		auto &state = GetState();
		state.busy = false;
		if (!correct) {
			done(AddResult::WrongPasscode);
			return;
		}
		auto id = 1;
		for (const auto &existing : state.list) {
			id = std::max(id, existing.id + 1);
		}
		entry.id = id;
		state.list.push_back(entry);
		const auto kdf = DefaultPasscodeKdf();
		const auto fail = [=] {
			auto &state = GetState();
			const auto i = ranges::find(state.list, id, &Entry::id);
			if (i != end(state.list)) {
				state.list.erase(i);
			}
			state.loaded = false;
			done(AddResult::Failed);
		};
		if (SameKdf(state.kdf, kdf)
			&& state.salt.size() == kSaltSize
			&& state.blobs.size() == size_t(kMaxEntries)) {
			// Остальным блобам ключ не нужен: делаем только новый.
			const auto salt = state.salt;
			const auto index = int(state.list.size()) - 1;
			state.busy = true;
			crl::async([=] {
				auto blob = MakeBlob(entry, real, salt, kdf);
				crl::on_main([=] {
					auto &state = GetState();
					state.busy = false;
					if (blob.isEmpty() || state.salt != salt) {
						fail();
						return;
					}
					state.blobs[index] = blob;
					if (WriteFile()) {
						done(AddResult::Ok);
					} else {
						fail();
					}
				});
			});
		} else {
			RebuildAsync(real, [=](bool ok) {
				if (ok) {
					done(AddResult::Ok);
				} else {
					fail();
				}
			});
		}
	});
	if (!started) {
		state.busy = false;
		done(AddResult::Busy);
	}
}

void Update(int id, QString name, Actions actions) {
	Load();
	auto &state = GetState();
	const auto i = ranges::find(state.list, id, &Entry::id);
	if (i == end(state.list)) {
		return;
	}
	i->name = std::move(name);
	i->actions = std::move(actions);
	if (i->unlocks) {
		[[maybe_unused]] const auto written = WriteFile();
		return;
	}
	// Действия хранятся в самом блобе: пересобираем его.
	const auto entry = *i;
	const auto index = int(i - begin(state.list));
	const auto salt = state.salt;
	const auto kdf = state.kdf;
	crl::async([=] {
		auto blob = MakeBlob(entry, QByteArray(), salt, kdf);
		crl::on_main([=] {
			auto &state = GetState();
			if (blob.isEmpty()
				|| state.salt != salt
				|| index >= int(state.list.size())
				|| state.list[index].id != id) {
				return;
			}
			state.blobs[index] = blob;
			[[maybe_unused]] const auto written = WriteFile();
		});
	});
}

void Remove(int id) {
	Load();
	auto &state = GetState();
	const auto i = ranges::find(state.list, id, &Entry::id);
	if (i == end(state.list)) {
		return;
	}
	const auto index = int(i - begin(state.list));
	if (index < int(state.blobs.size())) {
		state.blobs.erase(state.blobs.begin() + index);
	}
	state.list.erase(i);
	if (state.list.empty()) {
		state = State{ .loaded = true };
		RemoveFile();
		return;
	}
	state.blobs.push_back(MakeDecoy());
	[[maybe_unused]] const auto written = WriteFile();
}

void RealPasscodeChanged(const QByteArray &passcode) {
	Load();
	auto &state = GetState();
	if (!state.loaded) {
		return;
	} else if (passcode.isEmpty()) {
		// Без настоящего пароля ложные не имеют смысла.
		state = State{ .loaded = true };
		RemoveFile();
		return;
	} else if (state.list.empty()) {
		return;
	}
	RebuildAsync(passcode, [](bool) {});
}

} // namespace Mokagram::FakePasscode
