// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: отложенный черновик переживает перезапуск клиента.
//
// Файл tdata/moka_stash зашифрован локальным ключом, как и черновики
// самого клиента. Ключ записи: аккаунт, чат, тема, монофорум.
#include "mokagram/stash/mokagram_compose_stash.h"

#include "core/application.h"
#include "data/data_compose_stash.h"
#include "data/data_peer.h"
#include "history/history.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "settings.h"
#include "storage/details/storage_file_utilities.h"
#include "storage/serialize_common.h"
#include "storage/storage_domain.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>

#include <map>
#include <set>

namespace Mokagram::Stash {
namespace {

using namespace Storage::details;

constexpr auto kVersion = qint32(1);
constexpr auto kMaxEntries = 1000;

struct State {
	bool loaded = false;
	std::map<QString, QByteArray> entries;
	std::set<QString> checked;
};

[[nodiscard]] State &GetState() {
	static auto result = State();
	return result;
}

[[nodiscard]] QString BasePath() {
	return cWorkingDir() + u"tdata/"_q;
}

[[nodiscard]] QString FileName() {
	return u"moka_stash"_q;
}

[[nodiscard]] MTP::AuthKeyPtr LocalKey() {
	return Core::App().domain().started()
		? Core::App().domain().local().localKey()
		: nullptr;
}

[[nodiscard]] QString EntryKey(
		not_null<History*> history,
		Data::DraftKey key) {
	return QString::number(history->session().userId().bare)
		+ ':' + QString::number(history->peer->id.value)
		+ ':' + QString::number(key.topicRootId().bare)
		+ ':' + QString::number(key.monoforumPeerId().value);
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
	auto file = FileReadDescriptor();
	if (!ReadEncryptedFile(file, FileName(), BasePath(), key)) {
		return;
	}
	auto version = qint32();
	auto count = qint32();
	file.stream >> version >> count;
	if (!CheckStreamStatus(file.stream)
		|| version != kVersion
		|| count < 0
		|| count > kMaxEntries) {
		return;
	}
	for (auto i = 0; i != count; ++i) {
		auto name = QString();
		auto payload = QByteArray();
		file.stream >> name >> payload;
		if (!CheckStreamStatus(file.stream)) {
			state.entries.clear();
			return;
		}
		state.entries.emplace(name, payload);
	}
}

void Write() {
	const auto key = LocalKey();
	if (!key) {
		return;
	}
	const auto &entries = GetState().entries;
	if (entries.empty()) {
		const auto base = BasePath() + FileName();
		QFile::remove(base + 's');
		QFile::remove(base + '0');
		QFile::remove(base + '1');
		return;
	}
	auto size = sizeof(qint32) * 2;
	for (const auto &[name, payload] : entries) {
		size += Serialize::stringSize(name) + sizeof(quint32) + payload.size();
	}
	auto data = EncryptedDescriptor(size);
	data.stream << kVersion << qint32(entries.size());
	for (const auto &[name, payload] : entries) {
		data.stream << name << payload;
	}
	QDir().mkpath(BasePath());
	auto file = FileWriteDescriptor(FileName(), BasePath());
	file.writeEncrypted(data, key);
}

[[nodiscard]] bool Storable(const Data::ComposeStash &stash) {
	const auto &draft = stash.draft;
	return !draft.hasRichMessage()
		&& (!draft.textWithTags.text.isEmpty()
			|| draft.reply.replying()
			|| draft.suggest.exists
			|| !stash.forward.ids.empty());
}

[[nodiscard]] QByteArray Pack(const Data::ComposeStash &stash) {
	auto bytes = QByteArray();
	auto buffer = QBuffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	auto stream = QDataStream(&buffer);
	stream.setVersion(QDataStream::Qt_5_1);

	const auto &draft = stash.draft;
	stream << draft.textWithTags.text;
	stream << qint32(draft.textWithTags.tags.size());
	for (const auto &tag : draft.textWithTags.tags) {
		stream << qint32(tag.offset) << qint32(tag.length) << tag.id;
	}
	const auto &reply = draft.reply;
	stream << quint64(reply.messageId.peer.value)
		<< qint64(reply.messageId.msg.bare)
		<< reply.quote.text
		<< qint32(reply.quoteOffset)
		<< qint64(reply.topicRootId.bare)
		<< quint64(reply.monoforumPeerId.value);
	const auto &suggest = draft.suggest;
	stream << quint32(suggest.exists)
		<< quint32(suggest.priceWhole)
		<< quint32(suggest.priceNano)
		<< quint32(suggest.ton)
		<< qint32(suggest.date)
		<< qint32(suggest.offerDuration);
	stream << qint32(draft.cursor.position)
		<< qint32(draft.cursor.anchor)
		<< qint32(draft.cursor.scroll);
	stream << qint32(stash.forward.ids.size());
	for (const auto &id : stash.forward.ids) {
		stream << quint64(id.peer.value) << qint64(id.msg.bare);
	}
	stream << qint32(stash.forward.options);
	return bytes;
}

[[nodiscard]] std::unique_ptr<Data::ComposeStash> Unpack(
		const QByteArray &payload) {
	auto stream = QDataStream(payload);
	stream.setVersion(QDataStream::Qt_5_1);

	auto text = QString();
	auto count = qint32();
	stream >> text >> count;
	if (stream.status() != QDataStream::Ok
		|| count < 0
		|| count > text.size()) {
		return nullptr;
	}
	auto with = TextWithTags{ .text = text };
	for (auto i = 0; i != count; ++i) {
		auto offset = qint32();
		auto length = qint32();
		auto id = QString();
		stream >> offset >> length >> id;
		with.tags.push_back({ offset, length, id });
	}
	auto reply = FullReplyTo();
	auto peer = quint64();
	auto msg = qint64();
	auto quote = QString();
	auto quoteOffset = qint32();
	auto topicRootId = qint64();
	auto monoforum = quint64();
	stream >> peer >> msg >> quote >> quoteOffset >> topicRootId >> monoforum;
	reply.messageId = FullMsgId(PeerId(peer), MsgId(msg));
	reply.quote.text = quote;
	reply.quoteOffset = quoteOffset;
	reply.topicRootId = MsgId(topicRootId);
	reply.monoforumPeerId = PeerId(monoforum);

	auto suggest = SuggestOptions();
	auto exists = quint32();
	auto whole = quint32();
	auto nano = quint32();
	auto ton = quint32();
	auto date = qint32();
	auto duration = qint32();
	stream >> exists >> whole >> nano >> ton >> date >> duration;
	suggest.exists = exists;
	suggest.priceWhole = whole;
	suggest.priceNano = nano;
	suggest.ton = ton;
	suggest.date = date;
	suggest.offerDuration = duration;

	auto position = qint32();
	auto anchor = qint32();
	auto scroll = qint32();
	stream >> position >> anchor >> scroll;

	auto forward = Data::ForwardDraft();
	auto ids = qint32();
	stream >> ids;
	if (stream.status() != QDataStream::Ok || ids < 0 || ids > 100000) {
		return nullptr;
	}
	for (auto i = 0; i != ids; ++i) {
		auto idPeer = quint64();
		auto idMsg = qint64();
		stream >> idPeer >> idMsg;
		forward.ids.push_back(FullMsgId(PeerId(idPeer), MsgId(idMsg)));
	}
	auto options = qint32();
	stream >> options;
	if (stream.status() != QDataStream::Ok) {
		return nullptr;
	}
	forward.options = Data::ForwardOptions(options);

	auto result = std::make_unique<Data::ComposeStash>();
	result->draft = Data::Draft(
		with,
		reply,
		suggest,
		MessageCursor(position, anchor, scroll),
		Data::WebPageDraft());
	result->forward = std::move(forward);
	return result;
}

} // namespace

void Persist(
		not_null<History*> history,
		Data::DraftKey key,
		const Data::ComposeStash *stash) {
	if (!key) {
		return;
	}
	Load();
	auto &state = GetState();
	if (!state.loaded) {
		return;
	}
	const auto name = EntryKey(history, key);
	state.checked.emplace(name);
	if (!stash || !Storable(*stash)) {
		if (state.entries.erase(name)) {
			Write();
		}
		return;
	} else if (state.entries.size() >= kMaxEntries
		&& !state.entries.contains(name)) {
		return;
	}
	state.entries[name] = Pack(*stash);
	Write();
}

void RestoreIfNeeded(not_null<History*> history, Data::DraftKey key) {
	if (!key) {
		return;
	}
	auto &state = GetState();
	const auto name = EntryKey(history, key);
	if (!state.checked.emplace(name).second) {
		return;
	}
	Load();
	if (history->composeStash(key)) {
		return;
	}
	const auto i = state.entries.find(name);
	if (i == end(state.entries)) {
		return;
	}
	if (auto restored = Unpack(i->second)) {
		history->setComposeStash(key, std::move(restored));
	}
}

} // namespace Mokagram::Stash
