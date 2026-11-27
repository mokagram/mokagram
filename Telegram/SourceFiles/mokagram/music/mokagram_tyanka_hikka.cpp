// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#include "mokagram/music/mokagram_tyanka_hikka.h"

#include "base/timer.h"
#include "base/weak_ptr.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "history/history_item.h"
#include "media/audio/media_audio_track.h"
#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/music/mokagram_soundcloud.h"
#include "mokagram/settings/mokagram_settings.h"
#include "settings.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>

#include <array>

namespace Mokagram::TyankaHikka {
namespace {

constexpr auto kBarkGap = crl::time(110);

struct ClipTiming {
	crl::time start;
	crl::time end;
};

// Single barks in the original recording, in milliseconds.
constexpr auto kBarks = std::to_array<ClipTiming>({
	{ 48730, 49010 },
	{ 50720, 50970 },
	{ 55180, 55490 },
});

[[nodiscard]] std::unique_ptr<Media::Audio::Track> LoadBark(
		const QString &path,
		int index) {
	const auto timing = kBarks[index];
	auto bark = Media::Audio::Current().createTrack();
	bark->fillFromFile(path, timing.start, timing.end - timing.start);
	return bark->failed() ? nullptr : std::move(bark);
}

[[nodiscard]] QString SettingKey() {
	return u"player.tyanka_hikka"_q;
}

[[nodiscard]] QString UnlockPath() {
	return cWorkingDir() + u"tdata/moka_tyanka_hikka.unlocked"_q;
}

[[nodiscard]] QString TrackPath() {
	const auto local = cWorkingDir() + u"tdata/moka_tyanka_hikka.m4a"_q;
	if (QFileInfo(local).size() > 0) {
		return local;
	}
	const auto cached = Music::CacheDir()
		+ u"/send/tyankahikka - тянка хикка - АХЕГАО (prod. Aki).m4a"_q;
	if (QFileInfo(cached).size() <= 0) {
		return {};
	}
	return QFile::copy(cached, local) ? local : cached;
}

[[nodiscard]] const QRegularExpression &ArtistPattern() {
	static const auto result = QRegularExpression(
		uR"((?<![\p{L}\p{N}_])(?:tyanka\s*hikka|тянка\s*хикка)(?![\p{L}\p{N}_]))"_q,
		QRegularExpression::CaseInsensitiveOption
			| QRegularExpression::UseUnicodePropertiesOption);
	return result;
}

void RegisterSetting(bool unlocked) {
	Settings::Register({
		.key = SettingKey(),
		.section = QString::fromUtf8(Lang::Tr("Плеер")),
		.title = QString::fromUtf8(Lang::Tr("Тянка хикка")),
		.description = QString::fromUtf8(Lang::Tr("Одно «ваф» при отправке сообщения и два «ваф» при входящем в открытом чате.")),
		.category = u"look"_q,
		.sectionIcon = u"hide"_q,
		.hidden = !unlocked,
	});
}

class Mode final : public base::has_weak_ptr {
public:
	Mode()
	: _unlocked(QFile::exists(UnlockPath()))
	, _nextBarkTimer([this] { playNextBark(); })
	, _waffleBarkTimer([this] { playNextWaffleBark(); }) {
		RegisterSetting(_unlocked);
		Settings::BoolValue(SettingKey()) | rpl::on_next([this](bool enabled) {
			setEnabled(_unlocked && enabled);
		}, _lifetime);
	}

	void unlock() {
		if (!_unlocked) {
			auto marker = QSaveFile(UnlockPath());
			if (!marker.open(QIODevice::WriteOnly) || !marker.commit()) {
				return;
			}
			_unlocked = true;
			RegisterSetting(true);
		}
		const auto wasEnabled = _enabled;
		Settings::SetBool(SettingKey(), true);
		if (!wasEnabled) {
			if (const auto window = Core::App().activeWindow()) {
				window->showToast(QString::fromUtf8(Lang::Tr(_path.isEmpty()
					? "Тянка хикка включена. Аудиофайл не найден."
					: "Режим «Тянка хикка» включён")));
			}
		}
	}

	void enqueueBarks(int count) {
		if (!_enabled || !Core::App().settings().notificationsVolume()) {
			return;
		}
		_pendingBarks += count;
		if (!_nextBarkTimer.isActive()) {
			playNextBark();
		}
	}

	void playWaffle(const Music::Track &track, WaffleBark bark) {
		if (!Core::App().settings().notificationsVolume()) {
			return;
		}
		switch (bark) {
		case WaffleBark::Double: _waffleBark = 0; _waffleCount = 2; break;
		case WaffleBark::Single: _waffleBark = 1; _waffleCount = 1; break;
		case WaffleBark::Triple: _waffleBark = 2; _waffleCount = 3; break;
		case WaffleBark::Alternate: _waffleBark = 2; _waffleCount = 1; break;
		}
		if (_waffleBarkTimer.isActive() || _waffleLoading) {
			return;
		}
		if (_wafflePath.isEmpty()) {
			_wafflePath = TrackPath();
			if (_wafflePath.isEmpty()) {
				_waffleLoading = true;
				Music::Download(track, [weak = base::make_weak(this)](
						const QString &path, const QString &error) {
					if (const auto self = weak.get()) {
						self->_waffleLoading = false;
						if (error.isEmpty() && !path.isEmpty()) {
							self->_wafflePath = path;
							self->playNextWaffleBark();
						}
					}
				});
				return;
			}
		}
		playNextWaffleBark();
	}

private:
	void setEnabled(bool enabled) {
		_enabled = enabled;
		if (enabled) {
			_path = TrackPath();
		} else {
			_nextBarkTimer.cancel();
			_pendingBarks = 0;
			_nextBark = 0;
			_barks = {};
			_path.clear();
		}
	}

	void playNextBark() {
		if (!_pendingBarks || !Core::App().settings().notificationsVolume()) {
			_pendingBarks = 0;
			return;
		}
		if (_path.isEmpty()) {
			_path = TrackPath();
			if (_path.isEmpty()) {
				_pendingBarks = 0;
				return;
			}
		}

		auto &bark = _barks[_nextBark];
		if (!bark) {
			bark = LoadBark(_path, _nextBark);
			if (!bark) {
				_path.clear();
				_pendingBarks = 0;
				return;
			}
		}
		--_pendingBarks;
		_nextBark = (_nextBark + 1) % kBarks.size();
		bark->playOnce();
		_nextBarkTimer.callOnce(bark->getLengthMs() + kBarkGap);
	}

	void playNextWaffleBark() {
		if (!_waffleCount || !Core::App().settings().notificationsVolume()) {
			_waffleCount = 0;
			return;
		}
		auto &bark = _waffleBarks[_waffleBark];
		if (!bark) {
			bark = LoadBark(_wafflePath, _waffleBark);
			if (!bark) {
				_wafflePath.clear();
				_waffleCount = 0;
				return;
			}
		}
		--_waffleCount;
		_waffleBark = (_waffleBark + 1) % kBarks.size();
		bark->playOnce();
		_waffleBarkTimer.callOnce(bark->getLengthMs() + kBarkGap);
	}

	bool _unlocked = false;
	bool _enabled = false;
	QString _path;
	int _pendingBarks = 0;
	size_t _nextBark = 0;
	std::array<std::unique_ptr<Media::Audio::Track>, kBarks.size()> _barks;
	base::Timer _nextBarkTimer;
	QString _wafflePath;
	bool _waffleLoading = false;
	int _waffleBark = 0;
	int _waffleCount = 0;
	std::array<std::unique_ptr<Media::Audio::Track>, kBarks.size()> _waffleBarks;
	base::Timer _waffleBarkTimer;
	rpl::lifetime _lifetime;
};

std::unique_ptr<Mode> Instance;

} // namespace

void Start() {
	Instance = std::make_unique<Mode>();
}

void Finish() {
	Instance.reset();
}

bool IsArtist(const QString &name) {
	const auto trimmed = name.trimmed();
	const auto match = ArtistPattern().match(trimmed);
	return match.hasMatch() && match.capturedLength() == trimmed.size();
}

void Unlock() {
	if (Instance) {
		Instance->unlock();
	}
}

void PlayWaffleBarks(const Music::Track &track, WaffleBark bark) {
	if (Instance) {
		Instance->playWaffle(track, bark);
	}
}

void OnOwnMessage() {
	if (Instance) {
		Instance->enqueueBarks(1);
	}
}

void OnIncomingMessage(not_null<HistoryItem*> item) {
	if (!Instance || item->out() || !item->isRegular() || item->isService()) {
		return;
	}
	const auto window = Core::App().activeWindow();
	if (!window || window->locked() || !window->widget()->isActiveWindow()) {
		return;
	}
	const auto controller = window->sessionController();
	if (!controller) {
		return;
	}
	const auto chat = controller->activeChatCurrent();
	if (chat.owningHistory() == item->history()
		&& (!chat.topic() || chat.topic() == item->topic())
		&& (!chat.sublist() || chat.sublist() == item->savedSublist())) {
		Instance->enqueueBarks(2);
	}
}

} // namespace Mokagram::TyankaHikka
