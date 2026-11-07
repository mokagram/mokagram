// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: точное «был в сети» без Premium.
#include "mokagram/peek/mokagram_peek.h"

#include "mokagram/lang/mokagram_lang.h"
#include "api/api_user_privacy.h"
#include "apiwrap.h"
#include "base/call_delayed.h"
#include "base/unixtime.h"
#include "data/data_changes.h"
#include "data/data_lastseen_status.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings.h"
#include "ui/toast/toast.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>

#include <map>

namespace Mokagram::Peek {
namespace {

using Privacy = Api::UserPrivacy;

constexpr auto kKey = Privacy::Key::LastSeen;
constexpr auto kAttempts = 4;
constexpr auto kRetryDelay = crl::time(700);
constexpr auto kRestoreAttempts = 5;
constexpr auto kMaxStored = 500;

enum class Kind {
	Online,
	Offline,
};

struct Result {
	Kind kind = Kind::Offline;
	TimeId time = 0;
	TimeId checkedAt = 0;
};

bool Running = false;

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

void Toast(const QString &text) {
	Ui::Toast::Show(text);
}

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/moka_peek.json"_q;
}

// Файл: { "restore": { self: правило }, "seen": { "self:target": ... } }.
[[nodiscard]] QJsonObject &Stored() {
	static auto result = [] {
		auto file = QFile(FilePath());
		return file.open(QIODevice::ReadOnly)
			? QJsonDocument::fromJson(file.readAll()).object()
			: QJsonObject();
	}();
	return result;
}

void Save() {
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	auto file = QFile(FilePath());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		file.write(QJsonDocument(Stored()).toJson(QJsonDocument::Compact));
	}
}

[[nodiscard]] QString SelfKey(not_null<Main::Session*> session) {
	return QString::number(session->userId().bare);
}

[[nodiscard]] QString SeenKey(not_null<UserData*> user) {
	return SelfKey(&user->session())
		+ ':'
		+ QString::number(peerToUser(user->id).bare);
}

[[nodiscard]] bool HasRealPremium(not_null<Main::Session*> session) {
	return session->user()->isPremium();
}

[[nodiscard]] QJsonArray Ids(const std::vector<not_null<PeerData*>> &list) {
	auto result = QJsonArray();
	for (const auto &peer : list) {
		result.push_back(QString::number(peer->id.value));
	}
	return result;
}

[[nodiscard]] std::vector<not_null<PeerData*>> Peers(
		not_null<Main::Session*> session,
		const QJsonArray &ids) {
	auto result = std::vector<not_null<PeerData*>>();
	for (const auto &id : ids) {
		const auto value = id.toString().toULongLong();
		if (value) {
			result.push_back(session->data().peer(PeerId(value)));
		}
	}
	return result;
}

[[nodiscard]] QJsonObject RuleToJson(const Privacy::Rule &rule) {
	return QJsonObject{
		{ u"option"_q, int(rule.option) },
		{ u"always"_q, Ids(rule.always.peers) },
		{ u"never"_q, Ids(rule.never.peers) },
		{ u"alwaysPremiums"_q, rule.always.premiums },
		{ u"alwaysMiniapps"_q, rule.always.miniapps },
		{ u"neverPremiums"_q, rule.never.premiums },
		{ u"neverMiniapps"_q, rule.never.miniapps },
		{ u"ignoreAlways"_q, rule.ignoreAlways },
		{ u"ignoreNever"_q, rule.ignoreNever },
	};
}

[[nodiscard]] Privacy::Rule RuleFromJson(
		not_null<Main::Session*> session,
		const QJsonObject &json) {
	auto rule = Privacy::Rule();
	rule.option = Privacy::Option(json.value(u"option"_q).toInt());
	rule.always.peers = Peers(session, json.value(u"always"_q).toArray());
	rule.never.peers = Peers(session, json.value(u"never"_q).toArray());
	rule.always.premiums = json.value(u"alwaysPremiums"_q).toBool();
	rule.always.miniapps = json.value(u"alwaysMiniapps"_q).toBool();
	rule.never.premiums = json.value(u"neverPremiums"_q).toBool();
	rule.never.miniapps = json.value(u"neverMiniapps"_q).toBool();
	rule.ignoreAlways = json.value(u"ignoreAlways"_q).toBool();
	rule.ignoreNever = json.value(u"ignoreNever"_q).toBool();
	return rule;
}

void SetRestore(not_null<Main::Session*> session, const QJsonObject &rule) {
	auto restore = Stored().value(u"restore"_q).toObject();
	if (rule.isEmpty()) {
		restore.remove(SelfKey(session));
	} else {
		restore.insert(SelfKey(session), rule);
	}
	Stored().insert(u"restore"_q, restore);
	Save();
}

[[nodiscard]] std::optional<Result> Load(not_null<UserData*> user) {
	const auto seen = Stored().value(u"seen"_q).toObject();
	const auto value = seen.value(SeenKey(user)).toObject();
	if (value.isEmpty()) {
		return std::nullopt;
	}
	return Result{
		.kind = value.value(u"online"_q).toBool()
			? Kind::Online
			: Kind::Offline,
		.time = TimeId(value.value(u"time"_q).toInteger()),
		.checkedAt = TimeId(value.value(u"checked"_q).toInteger()),
	};
}

[[nodiscard]] QString ExactText(const Result &result, TimeId now) {
	const auto seen = (result.kind == Kind::Online && result.checkedAt)
		? result.checkedAt
		: result.time;
	const auto when = base::unixtime::parse(seen);
	const auto today = base::unixtime::parse(now);
	const auto time = when.time().toString(u"HH:mm:ss"_q);
	if (when.date() == today.date()) {
		return tr::lng_status_lastseen_today(tr::now, lt_time, time);
	} else if (when.date().addDays(1) == today.date()) {
		return tr::lng_status_lastseen_yesterday(tr::now, lt_time, time);
	}
	return tr::lng_status_lastseen_date_time(
		tr::now,
		lt_date,
		QLocale().toString(when.date(), QLocale::ShortFormat),
		lt_time,
		time);
}

void Store(not_null<UserData*> user, const Result &result) {
	auto seen = Stored().value(u"seen"_q).toObject();
	if (seen.size() >= kMaxStored && !seen.contains(SeenKey(user))) {
		seen.remove(seen.begin().key());
	}
	seen.insert(SeenKey(user), QJsonObject{
		{ u"online"_q, result.kind == Kind::Online },
		{ u"time"_q, qint64(result.time) },
		{ u"checked"_q, qint64(result.checkedAt) },
	});
	Stored().insert(u"seen"_q, seen);
	Save();
	user->session().changes().peerUpdated(
		user,
		Data::PeerUpdate::Flag::OnlineStatus);
}

void Restore(
		not_null<Main::Session*> session,
		Privacy::Rule rule,
		int attempt,
		Fn<void()> finished) {
	session->api().userPrivacy().save(kKey, rule, [=] {
		SetRestore(session, QJsonObject());
		finished();
	}, [=] {
		if (attempt + 1 >= kRestoreAttempts) {
			Toast(U("Не удалось вернуть настройку «Последний визит». Проверьте её в Конфиденциальности."));
			finished();
			return;
		}
		base::call_delayed(kRetryDelay * 3, session, [=] {
			Restore(session, rule, attempt + 1, finished);
		});
	});
}

[[nodiscard]] std::optional<Result> ParseStatus(const MTPUserStatus &status) {
	auto result = Result{ .checkedAt = base::unixtime::now() };
	return status.match([&](const MTPDuserStatusOnline &data)
	-> std::optional<Result> {
		result.kind = Kind::Online;
		result.time = data.vexpires().v;
		return result;
	}, [&](const MTPDuserStatusOffline &data) -> std::optional<Result> {
		result.kind = Kind::Offline;
		result.time = data.vwas_online().v;
		return result;
	}, [](const auto &) -> std::optional<Result> {
		return std::nullopt;
	});
}

void Fetch(
		not_null<UserData*> user,
		int attempt,
		Fn<void(std::optional<Result>)> done) {
	const auto session = &user->session();
	const auto userId = peerToUser(user->id);
	session->api().request(MTPusers_GetUsers(
		MTP_vector<MTPInputUser>(1, user->inputUser())
	)).done([=](const MTPVector<MTPUser> &result) {
		session->data().processUsers(result);
		auto found = std::optional<Result>();
		for (const auto &item : result.v) {
			item.match([&](const MTPDuser &data) {
				if (UserId(data.vid()) != userId) {
					return;
				} else if (const auto status = data.vstatus()) {
					found = ParseStatus(*status);
				}
			}, [](const auto &) {
			});
		}
		if (found || attempt + 1 >= kAttempts) {
			done(found);
			return;
		}
		// Сервер применяет правило не мгновенно: спрашиваем ещё раз.
		base::call_delayed(kRetryDelay, session, [=] {
			if (const auto strong = session->data().userLoaded(userId)) {
				Fetch(strong, attempt + 1, done);
			} else {
				done(std::nullopt);
			}
		});
	}).fail([=] {
		done(std::nullopt);
	}).send();
}

void Begin(not_null<UserData*> user, const Privacy::Rule &original) {
	const auto session = &user->session();
	const auto userId = peerToUser(user->id);
	const auto finish = [=](std::optional<Result> result, bool changed) {
		const auto finalize = [=] {
			Running = false;
			if (const auto strong = session->data().userLoaded(userId)) {
				if (result) {
					Store(strong, *result);
					Toast(strong->shortName() + u": "_q + ExactText(
						*result,
						base::unixtime::now()));
				} else {
					Toast(U("Не получилось узнать время."));
				}
			}
		};
		if (changed) {
			Restore(session, original, 0, finalize);
		} else {
			finalize();
		}
	};

	const auto target = not_null<PeerData*>(user);
	auto modified = original;
	modified.ignoreAlways = false;
	auto &never = modified.never.peers;
	never.erase(ranges::remove(never, target), never.end());
	auto &always = modified.always.peers;
	if (!ranges::contains(always, target)) {
		always.push_back(target);
	}
	const auto already = (original.option == Privacy::Option::Everyone)
		&& !ranges::contains(original.never.peers, target);
	if (already) {
		Fetch(user, 0, [=](std::optional<Result> result) {
			finish(result, false);
		});
		return;
	}
	// Сначала запоминаем правило на диск: закроют клиент, вернём при старте.
	SetRestore(session, RuleToJson(original));
	Toast(U("Смотрю, когда был в сети…"));
	session->api().userPrivacy().save(kKey, modified, [=] {
		Fetch(user, 0, [=](std::optional<Result> result) {
			finish(result, true);
		});
	}, [=] {
		SetRestore(session, QJsonObject());
		finish(std::nullopt, false);
	});
}

} // namespace

bool Available(not_null<UserData*> user) {
	return !user->isSelf()
		&& !user->isBot()
		&& !user->isInaccessible()
		&& !user->isServiceUser()
		&& !HasRealPremium(&user->session());
}

void Start(not_null<UserData*> user) {
	const auto session = &user->session();
	if (HasRealPremium(session)) {
		Toast(U("С Premium время и так видно."));
		return;
	} else if (Running) {
		Toast(U("Уже смотрю, подождите секунду."));
		return;
	}
	Running = true;
	auto &privacy = session->api().userPrivacy();
	privacy.reload(kKey);
	const auto userId = peerToUser(user->id);
	const auto began = std::make_shared<bool>(false);
	privacy.value(
		kKey
	) | rpl::take(1) | rpl::on_next([=](const Privacy::Rule &rule) {
		*began = true;
		if (const auto strong = session->data().userLoaded(userId)) {
			Begin(strong, rule);
		} else {
			Running = false;
		}
	}, session->lifetime());
	base::call_delayed(kRetryDelay * 15, session, [=] {
		if (!*began) {
			Running = false;
			Toast(U("Не получилось загрузить настройки приватности."));
		}
	});
}

void RestoreIfNeeded(not_null<Main::Session*> session) {
	if (Running) {
		return;
	}
	const auto rule = Stored().value(u"restore"_q).toObject().value(
		SelfKey(session)).toObject();
	if (rule.isEmpty()) {
		return;
	}
	Running = true;
	Restore(session, RuleFromJson(session, rule), 0, [] {
		Running = false;
	});
}

QString Augment(
		not_null<UserData*> user,
		const QString &telegramText,
		TimeId now) {
	if (!Available(user)) {
		return telegramText;
	}
	const auto &lastseen = user->lastseen();
	if (!lastseen.isHidden() || lastseen.isLocalOnlineValue()) {
		return telegramText;
	}
	const auto result = Load(user);
	if (!result) {
		return telegramText;
	}
	return telegramText + u" · "_q + ExactText(*result, now);
}

} // namespace Mokagram::Peek
