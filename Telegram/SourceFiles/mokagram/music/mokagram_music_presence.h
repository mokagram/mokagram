// This file is part of mokaGram, https://github.com/mokagram/mokagram.
// GPLv3 with the attribution term of MOKAGRAM_NOTICE.md.
#pragma once

#include "base/basic_types.h"
#include "rpl/lifetime.h"

#include <QtCore/QString>

#include <map>
#include <vector>

class QPainter;
class QRect;
class UserData;

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace Mokagram::Music::Presence {

inline const auto kShareKey = QStringLiteral("music.share_playback");
inline const auto kShowKey = QStringLiteral("music.show_playback_status");

struct Playback {
	Main::Session *session = nullptr;
	QString title;
	QString artist;
	uint64 soundcloudId = 0;
	QString albumTitle;
};

struct Track {
	QString title;
	QString artist;
	uint64 soundcloudId = 0;
	QString artworkUrl; // только для своего профиля, наружу не передаётся
	QString albumTitle;

	explicit operator bool() const { return !title.isEmpty(); }
	friend bool operator==(const Track &, const Track &) = default;
};

using Tracks = std::map<uint64, Track>;

void Publish(Fn<Playback()> playback, rpl::lifetime &lifetime);
void Watch(
	not_null<UserData*> user,
	not_null<Ui::RpWidget*> view,
	Fn<void(Track)> changed,
	rpl::lifetime &lifetime);
void WatchList(
	not_null<Main::Session*> session,
	not_null<Ui::RpWidget*> view,
	Fn<std::vector<uint64>()> visible,
	Fn<void(Tracks)> changed,
	rpl::lifetime &lifetime);
[[nodiscard]] bool CanWatch(not_null<UserData*> user);
void PaintRing(QPainter &p, const QRect &avatar);
void OpenTrack(not_null<Window::SessionController*> controller, Track track);

} // namespace Mokagram::Music::Presence
