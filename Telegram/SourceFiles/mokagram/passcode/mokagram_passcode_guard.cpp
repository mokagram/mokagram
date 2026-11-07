// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: охрана код-пароля.
#include "mokagram/passcode/mokagram_passcode_guard.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/passcode/mokagram_fake_passcode.h"
#include "mokagram/settings/mokagram_settings.h"
#include "base/timer.h"
#include "core/application.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "storage/details/storage_file_utilities.h"
#include "storage/storage_domain.h"
#include "window/main_window.h"
#include "window/window_controller.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QGuiApplication>

namespace Mokagram::PasscodeGuard {
namespace {

using namespace Storage::details;

constexpr auto kFreeTries = 3;
constexpr auto kInactiveCheckEach = crl::time(1000);

// Индексы выбора в настройках.
constexpr int kWipeAttempts[] = { 0, 3, 5, 10, 20 };
constexpr int kLockDelays[] = { -1, 0, 60, 300, 900 };

struct State {
	bool loaded = false;
	int tries = 0;
	base::Timer inactiveTimer;
	crl::time inactiveSince = 0;
	rpl::lifetime lifetime;
};

[[nodiscard]] State &GetState() {
	static auto result = State();
	return result;
}

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

[[nodiscard]] QString BasePath() {
	return cWorkingDir() + u"tdata/"_q;
}

[[nodiscard]] QString FileName() {
	return u"passcode_state"_q;
}

[[nodiscard]] int WipeLimit() {
	const auto index = Settings::Int(u"passguard.wipe"_q);
	return (index > 0 && index < int(std::size(kWipeAttempts)))
		? kWipeAttempts[index]
		: 0;
}

[[nodiscard]] int LockDelay() {
	const auto index = Settings::Int(u"passguard.lock_inactive"_q);
	return (index > 0 && index < int(std::size(kLockDelays)))
		? kLockDelays[index]
		: -1;
}

[[nodiscard]] crl::time RequiredDelay(int tries) {
	return (tries < kFreeTries)
		? 0
		: std::min(tries - kFreeTries + 1, 6) * crl::time(5000);
}

void Write() {
	QDir().mkpath(BasePath());
	auto file = FileWriteDescriptor(FileName(), BasePath(), true);
	auto data = QByteArray();
	{
		auto stream = QDataStream(&data, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << qint32(GetState().tries);
	}
	file.writeData(data);
}

void Remove() {
	const auto base = BasePath() + FileName();
	QFile::remove(base + 's');
	QFile::remove(base + '0');
	QFile::remove(base + '1');
}

void SyncGlobals() {
	// Ожидание после перезапуска начинается заново: перевод системных
	// часов его не пропустит.
	cSetPasscodeBadTries(GetState().tries);
	cSetPasscodeLastTry(crl::now());
}

void Wipe() {
	LOG(("Mokagram Passcode Guard: too many failed attempts, wiping."));
	const auto started = Core::App().domain().started();
	auto actions = FakePasscode::Actions();
	if (!started || Settings::Int(u"passguard.wipe_action"_q) == 1) {
		actions.deleteAll = true;
	} else {
		for (const auto &entry : Core::App().domain().accounts()) {
			actions.logoutAccounts.push_back(entry.index);
		}
	}
	FakePasscode::RunSilent(actions);
	if (!started) {
		// Сессий на диске уже нет: перезапуск начнёт с чистого листа.
		crl::on_main(&Core::App(), [] { Core::Restart(); });
	}
}

[[nodiscard]] bool CanLock() {
	auto &app = Core::App();
	return app.domain().started()
		&& app.domain().local().hasLocalPasscode()
		&& !app.passcodeLocked();
}

void Lock() {
	if (CanLock()) {
		Core::App().lockByPasscode();
	}
}

[[nodiscard]] bool WindowInactive() {
	const auto window = Core::App().activePrimaryWindow();
	if (!window) {
		return false;
	}
	const auto widget = window->widget();
	// Wayland не сообщает окну, что оно свёрнуто: считаем по фокусу.
	return widget->isHidden()
		|| widget->isMinimized()
		|| (qGuiApp->applicationState() != Qt::ApplicationActive);
}

void CheckInactive() {
	auto &state = GetState();
	const auto delay = LockDelay();
	if (delay < 0 || !CanLock()) {
		state.inactiveTimer.cancel();
		state.inactiveSince = 0;
		return;
	} else if (!WindowInactive()) {
		state.inactiveSince = 0;
		return;
	}
	const auto now = crl::now();
	if (!state.inactiveSince) {
		state.inactiveSince = now;
	}
	if (now - state.inactiveSince >= delay * crl::time(1000)) {
		state.inactiveTimer.cancel();
		state.inactiveSince = 0;
		Lock();
	}
}

void ApplicationStateChanged(Qt::ApplicationState applicationState) {
	auto &state = GetState();
	if (applicationState == Qt::ApplicationActive) {
		state.inactiveTimer.cancel();
		state.inactiveSince = 0;
	} else if (LockDelay() >= 0 && !state.inactiveTimer.isActive()) {
		state.inactiveTimer.callEach(kInactiveCheckEach);
		CheckInactive();
	}
}

} // namespace

void EnsureLoaded() {
	auto &state = GetState();
	if (state.loaded) {
		return;
	}
	state.loaded = true;
	auto file = FileReadDescriptor();
	if (!ReadFile(file, FileName(), BasePath())) {
		return;
	}
	auto data = QByteArray();
	file.stream >> data;
	if (!CheckStreamStatus(file.stream)) {
		return;
	}
	auto stream = QDataStream(data);
	stream.setVersion(QDataStream::Qt_5_1);
	auto tries = qint32();
	stream >> tries;
	if (stream.status() != QDataStream::Ok || tries < 0) {
		return;
	}
	state.tries = tries;
	SyncGlobals();
}

bool RegisterFailure() {
	EnsureLoaded();
	auto &state = GetState();
	++state.tries;
	SyncGlobals();
	const auto limit = WipeLimit();
	if (limit > 0 && state.tries >= limit) {
		state.tries = 0;
		SyncGlobals();
		Remove();
		Wipe();
		return true;
	}
	Write();
	return false;
}

void RegisterSuccess() {
	EnsureLoaded();
	auto &state = GetState();
	const auto had = (state.tries > 0);
	state.tries = 0;
	cSetPasscodeBadTries(0);
	if (had) {
		Remove();
	}
}

crl::time WaitLeft() {
	EnsureLoaded();
	const auto delay = RequiredDelay(cPasscodeBadTries());
	const auto passed = crl::now() - cPasscodeLastTry();
	return std::max(delay - passed, crl::time(0));
}

int AttemptsBeforeWipe() {
	const auto limit = WipeLimit();
	if (limit <= 0) {
		return -1;
	}
	EnsureLoaded();
	return std::max(limit - GetState().tries, 0);
}

void Init() {
	auto &state = GetState();
	state.inactiveTimer.setCallback(CheckInactive);
	QObject::connect(
		qGuiApp,
		&QGuiApplication::applicationStateChanged,
		ApplicationStateChanged);
}

void ScreenLocked() {
	// Приходит из системного обработчика, вне цикла событий Qt.
	crl::on_main(&Core::App(), [] {
		if (Settings::Bool(u"passguard.lock_screen"_q)) {
			Lock();
		}
	});
}

void RegisterSettings() {
	const auto section = U("Код-пароль");
	Settings::Register({
		.key = u"passguard.lock_inactive"_q,
		.section = section,
		.title = U("Блокировать, когда окно не в фокусе"),
		.description = U("Клиент закроется код-паролем, если вы ушли в другое окно или свернули его."),
		.kind = Settings::Kind::Choice,
		.defaultValue = 0,
		.choices = {
			U("Выключено"),
			U("Сразу"),
			U("Через минуту"),
			U("Через 5 минут"),
			U("Через 15 минут"),
		},
		.category = u"privacy"_q,
		.sectionIcon = u"lock"_q,
		.icon = u"lock"_q,
	});
	Settings::Register({
		.key = u"passguard.lock_screen"_q,
		.section = section,
		.title = U("Блокировать вместе с компьютером"),
		.description = U("Когда блокируется экран компьютера, клиент тоже закрывается код-паролем."),
		.kind = Settings::Kind::Bool,
		.defaultValue = false,
		.category = u"privacy"_q,
		.sectionIcon = u"lock"_q,
		.icon = u"lock"_q,
	});
	Settings::Register({
		.key = u"passguard.wipe"_q,
		.section = section,
		.title = U("Стирать после неверных попыток"),
		.description = U("Если код-пароль ввели неверно столько раз подряд, клиент выходит из аккаунтов или стирает всё."),
		.kind = Settings::Kind::Choice,
		.defaultValue = 0,
		.choices = {
			U("Выключено"),
			U("После 3"),
			U("После 5"),
			U("После 10"),
			U("После 20"),
		},
		.category = u"privacy"_q,
		.sectionIcon = u"lock"_q,
		.icon = u"block"_q,
	});
	Settings::Register({
		.key = u"passguard.wipe_action"_q,
		.section = section,
		.title = U("Что стирать"),
		.description = U("Выход из всех аккаунтов или полное стирание данных клиента."),
		.kind = Settings::Kind::Choice,
		.defaultValue = 0,
		.choices = {
			U("Выйти из аккаунтов"),
			U("Стереть всё"),
		},
		.category = u"privacy"_q,
		.sectionIcon = u"lock"_q,
		.icon = u"block"_q,
	});
}

} // namespace Mokagram::PasscodeGuard
