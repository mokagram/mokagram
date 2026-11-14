/*
This file is part of mokaGram, a Telegram Desktop fork.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "mokagram/music/mokagram_island.h"

#include "mokagram/music/mokagram_island_glyphs.h"
#include "mokagram/music/mokagram_music_ui.h"
#include "mokagram/music/mokagram_soundcloud.h"
#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/settings/mokagram_settings.h"
#include "media/media_common.h"
#include "media/player/media_player_instance.h"
#include "media/audio/media_audio.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_file_origin.h"
#include "data/data_session.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"
#include "window/window_adaptive.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/shortcuts.h"
#include "ui/painter.h"
#include "ui/power_saving.h"
#include "ui/rp_widget.h"
#include "ui/effects/animations.h"
#include "ui/text/format_values.h"
#include "ui/text/format_song_document_name.h"
#include "ui/widgets/popup_menu.h"
#include "base/timer.h"
#include "styles/style_info.h"
#include "styles/style_widgets.h"
#include "styles/style_menu_icons.h"

#include <QtCore/QPointer>
#include <QtGui/QPainterPath>
#include <QtSvg/QSvgRenderer>

namespace Mokagram::Music {
namespace {

constexpr auto kType = AudioMsgId::Type::Song;
constexpr auto kHoverDelay = crl::time(220);
constexpr auto kLeaveDelay = crl::time(450);
constexpr auto kSubstep = 1. / 240.;
constexpr auto kWheelStep = 0.05;

struct Motion {
	double response = 0.5;
	double damping = 0.82;
};

// Пружины 1 в 1 из islandcn, lib/island/motion.ts.
constexpr auto kAppear = Motion{ 0.483, 0.62 };
constexpr auto kExpand = Motion{ 0.517, 0.824 };
constexpr auto kCollapse = Motion{ 0.479, 0.894 };
constexpr auto kDismiss = Motion{ 0.432, 1.019 };
constexpr auto kResize = Motion{ 0.5, 0.82 };

// Размеры в точках iPhone из lib/island/metrics.ts.
constexpr auto kIdleWidth = 95;
constexpr auto kCompactHeight = 37;
constexpr auto kCompactWidth = 220;
constexpr auto kCompactPadding = 7;
constexpr auto kExpandedWidth = 372;
constexpr auto kExpandedHeight = 202;
constexpr auto kExpandedRadius = 46;
constexpr auto kPadding = 18;
constexpr auto kCover = 52;
constexpr auto kProgressY = 88;
constexpr auto kTransportY = 130;
constexpr auto kVolumeY = 172;
constexpr auto kBars = 5;

[[nodiscard]] int S(int value) {
	return style::ConvertScale(value);
}

[[nodiscard]] double Lerp(double a, double b, double t) {
	return a + (b - a) * t;
}

[[nodiscard]] QRectF Lerp(const QRectF &a, const QRectF &b, double t) {
	return QRectF(
		Lerp(a.x(), b.x(), t),
		Lerp(a.y(), b.y(), t),
		Lerp(a.width(), b.width(), t),
		Lerp(a.height(), b.height(), t));
}

class Spring final {
public:
	void jump(double value) {
		_value = _target = value;
		_velocity = 0.;
	}
	void to(double target, Motion motion) {
		_target = target;
		_motion = motion;
	}
	[[nodiscard]] double value() const {
		return _value;
	}
	[[nodiscard]] double target() const {
		return _target;
	}
	[[nodiscard]] bool settled() const {
		return (_value == _target) && (_velocity == 0.);
	}
	void step(double dt, double epsilon) {
		const auto k = std::pow(2. * M_PI / _motion.response, 2);
		const auto c = 4. * M_PI * _motion.damping / _motion.response;
		_velocity += (-k * (_value - _target) - c * _velocity) * dt;
		_value += _velocity * dt;
		if (std::abs(_value - _target) < epsilon
			&& std::abs(_velocity) < epsilon * 10.) {
			jump(_target);
		}
	}

private:
	double _value = 0.;
	double _velocity = 0.;
	double _target = 0.;
	Motion _motion = kResize;

};

enum class Glyph {
	Pause,
	Play,
	Forward,
	Back,
	Output,
	Speaker,
	Repeat,
	RepeatOne,
	Shuffle,
	Lyrics,
	Queue,
};

[[nodiscard]] const char *GlyphSvg(Glyph glyph) {
	using namespace IslandGlyphs;
	switch (glyph) {
	case Glyph::Pause: return kGlyphPause;
	case Glyph::Play: return kGlyphPlay;
	case Glyph::Forward: return kGlyphForward;
	case Glyph::Back: return kGlyphBack;
	case Glyph::Output: return kGlyphOutput;
	case Glyph::Speaker: return kGlyphSpeaker;
	case Glyph::Repeat: return kGlyphRepeat;
	case Glyph::RepeatOne: return kGlyphRepeatOne;
	case Glyph::Shuffle: return kGlyphShuffle;
	case Glyph::Lyrics: return kGlyphLyrics;
	case Glyph::Queue: return kGlyphQueue;
	}
	return kGlyphPlay;
}

// Значок рисуется в квадрат size по центру c, растр кэшируется по размеру.
void PaintGlyph(QPainter &p, Glyph glyph, QPointF c, int size) {
	static auto cache = base::flat_map<std::pair<int, int>, QImage>();
	const auto ratio = style::DevicePixelRatio();
	const auto key = std::make_pair(int(glyph), size);
	auto i = cache.find(key);
	if (i == end(cache)) {
		auto image = QImage(
			QSize(size, size) * ratio,
			QImage::Format_ARGB32_Premultiplied);
		image.setDevicePixelRatio(ratio);
		image.fill(Qt::transparent);
		auto q = QPainter(&image);
		auto renderer = QSvgRenderer(QByteArray(GlyphSvg(glyph)));
		renderer.render(&q, QRectF(0, 0, size, size));
		q.end();
		i = cache.emplace(key, std::move(image)).first;
	}
	p.drawImage(QPointF(c.x() - size / 2., c.y() - size / 2.), i->second);
}

class Island final : public Ui::RpWidget {
public:
	Island(
		not_null<Ui::RpWidget*> parent,
		not_null<Window::SessionController*> controller,
		not_null<Ui::RpWidget*> anchor);

	void relayout();
	void appear();

protected:
	void paintEvent(QPaintEvent *e) override;
	void enterEventHook(QEnterEvent *e) override;
	void leaveEventHook(QEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;

private:
	enum class Part {
		None,
		Body,
		Cover,
		Shuffle,
		Previous,
		Play,
		Next,
		Repeat,
		Progress,
		Speaker,
		Volume,
		Lyrics,
		Queue,
	};

	void dismiss();
	void expand(bool expanded);
	void refreshTrack();
	void refreshState(const Media::Player::TrackState &state);
	void startFrames();
	bool frame();
	void applyGeometry();
	void setVolume(double volume, bool finished);
	void dragTo(QPoint point);
	void activate(Part part);
	[[nodiscard]] bool areaVisible() const;
	[[nodiscard]] int expandedWidth() const;
	[[nodiscard]] int compactWidth() const;
	[[nodiscard]] double expanded() const;
	[[nodiscard]] QRectF shapeRect() const;
	[[nodiscard]] QRectF coverRect() const;
	[[nodiscard]] QRectF barsRect() const;
	[[nodiscard]] QRectF progressRect() const;
	[[nodiscard]] QRectF volumeRect() const;
	[[nodiscard]] QPointF center(Part part) const;
	[[nodiscard]] Part partAt(QPoint point) const;
	[[nodiscard]] static double RatioIn(const QRectF &rect, QPoint point);
	void paintTitle(QPainter &p);
	void paintCover(QPainter &p);
	void paintBars(QPainter &p);
	void paintExpanded(QPainter &p, const QRectF &shape);
	void paintTrack(QPainter &p, const QRectF &rect, double ratio);
	void paintToggle(QPainter &p, Part part, Glyph glyph, bool active, bool enabled);

	const not_null<Window::SessionController*> _controller;
	const not_null<Ui::RpWidget*> _anchor;
	Ui::Animations::Basic _animation;
	base::Timer _hoverTimer;
	base::Timer _leaveTimer;
	base::unique_qptr<Ui::PopupMenu> _menu;
	std::shared_ptr<Data::DocumentMedia> _media;
	Spring _width;
	Spring _height;
	Spring _radius;
	Spring _expanded;
	Spring _alpha;
	Spring _level;
	crl::time _lastFrame = 0;
	double _phase = 0.;
	bool _shown = false;
	bool _wantExpanded = false;
	Part _dragging = Part::None;
	double _seekProgress = 0.;
	AudioMsgId _track;
	QString _title;
	QString _artist;
	crl::time _position = 0;
	crl::time _length = 0;
	bool _playing = false;
	Part _pressed = Part::None;
	int _shapeTop = 0;
	double _centerX = 0.;
	double _shapeLeft = 0.;

};

Island::Island(
	not_null<Ui::RpWidget*> parent,
	not_null<Window::SessionController*> controller,
	not_null<Ui::RpWidget*> anchor)
: RpWidget(parent)
, _controller(controller)
, _anchor(anchor)
, _animation([=] { return frame(); })
, _hoverTimer([=] { expand(true); })
, _leaveTimer([=] {
	if (_dragging == Part::None) {
		expand(false);
	}
}) {
	setMouseTracking(true);
	hide();

	const auto instance = Media::Player::instance();
	instance->updatedNotifier(
	) | rpl::filter([](const Media::Player::TrackState &state) {
		return state.id.type() == kType;
	}) | rpl::on_next([=](const Media::Player::TrackState &state) {
		refreshState(state);
	}, lifetime());
	instance->trackChanged(
	) | rpl::filter([](AudioMsgId::Type type) {
		return type == kType;
	}) | rpl::on_next([=] {
		refreshTrack();
	}, lifetime());
	instance->stops(kType) | rpl::on_next([=] {
		dismiss();
	}, lifetime());
	QueueChanges() | rpl::on_next([=] {
		refreshTrack();
	}, lifetime());

	auto &settings = Core::App().settings();
	rpl::merge(
		CoversChanged(),
		LyricsAvailabilityChanges(),
		_controller->session().downloaderTaskFinished(),
		settings.songVolumeChanges() | rpl::to_empty,
		settings.playerRepeatModeValue() | rpl::to_empty,
		settings.playerOrderModeValue() | rpl::to_empty,
		Mokagram::Settings::Changed() | rpl::to_empty
	) | rpl::on_next([=] {
		update();
	}, lifetime());

	rpl::merge(
		_anchor->geometryValue() | rpl::to_empty,
		_anchor->shownValue() | rpl::to_empty,
		_controller->mainSectionShownChanges() | rpl::to_empty,
		_controller->adaptive().value()
	) | rpl::on_next([=] {
		relayout();
	}, lifetime());

	refreshTrack();
	refreshState(instance->getState(kType));
}

bool Island::areaVisible() const {
	return !_controller->adaptive().isOneColumn()
		|| _controller->mainSectionShown();
}

int Island::expandedWidth() const {
	return std::max(
		std::min(S(kExpandedWidth), _anchor->width() - S(24)),
		S(kCompactWidth));
}

int Island::compactWidth() const {
	return std::min(S(kCompactWidth), expandedWidth());
}

double Island::expanded() const {
	return std::clamp(_expanded.value(), 0., 1.);
}

void Island::appear() {
	if (_shown) {
		return;
	}
	_shown = true;
	if (isHidden()) {
		_width.jump(S(kIdleWidth));
		_height.jump(S(kCompactHeight));
		_radius.jump(S(kCompactHeight) / 2.);
		_expanded.jump(0.);
		_alpha.jump(0.);
	}
	_width.to(_wantExpanded ? expandedWidth() : compactWidth(), kAppear);
	_alpha.to(1., kAppear);
	Shortcuts::ToggleMediaShortcuts(true);
	relayout();
	startFrames();
}

void Island::dismiss() {
	if (!_shown) {
		return;
	}
	_shown = false;
	_wantExpanded = false;
	_hoverTimer.cancel();
	_leaveTimer.cancel();
	_width.to(S(kIdleWidth), kDismiss);
	_height.to(S(kCompactHeight), kDismiss);
	_radius.to(S(kCompactHeight) / 2., kDismiss);
	_expanded.to(0., kDismiss);
	_alpha.to(0., kDismiss);
	startFrames();
}

void Island::expand(bool expanded) {
	_hoverTimer.cancel();
	if (!_shown || _wantExpanded == expanded) {
		return;
	}
	_wantExpanded = expanded;
	const auto motion = expanded ? kExpand : kCollapse;
	_width.to(expanded ? expandedWidth() : compactWidth(), motion);
	_height.to(S(expanded ? kExpandedHeight : kCompactHeight), motion);
	_radius.to(
		expanded ? S(kExpandedRadius) : (S(kCompactHeight) / 2.),
		motion);
	_expanded.to(expanded ? 1. : 0., motion);
	startFrames();
}

void Island::refreshTrack() {
	_track = Media::Player::instance()->current(kType);
	const auto document = _track.audio();
	if (!document) {
		_media = nullptr;
		_title = _artist = QString();
		update();
		return;
	}
	if (!_media || _media->owner() != document) {
		_media = document->createMediaView();
		_media->thumbnailWanted(Data::FileOrigin(_track.contextId()));
	}
	if (!OwnsTrack(_track) || !CurrentTrackInfo(_title, _artist)) {
		const auto name = Ui::Text::FormatSongNameFor(document).composedName();
		_title = name.title;
		_artist = name.performer;
	}
	if (_title.isEmpty()) {
		_title = document->filename();
	}
	update();
}

void Island::refreshState(const Media::Player::TrackState &state) {
	if (state.id != _track) {
		refreshTrack();
	}
	const auto frequency = std::max(state.frequency, 1);
	_position = (state.position * 1000LL) / frequency;
	_length = (state.length * 1000LL) / frequency;
	if (!_length && state.id.audio()) {
		_length = state.id.audio()->duration();
	}
	const auto playing = Media::Player::ShowPauseIcon(state.state);
	if (_playing != playing) {
		_playing = playing;
		_level.to(playing ? 1. : 0., kResize);
		startFrames();
	}
	update();
}

void Island::startFrames() {
	if (!_animation.animating()) {
		_lastFrame = crl::now();
		_animation.start();
	}
}

bool Island::frame() {
	const auto now = crl::now();
	const auto elapsed = std::min((now - _lastFrame) / 1000., 0.05);
	auto left = elapsed;
	_lastFrame = now;
	while (left > 0.) {
		const auto dt = std::min(left, kSubstep);
		left -= dt;
		_width.step(dt, 0.1);
		_height.step(dt, 0.1);
		_radius.step(dt, 0.1);
		_expanded.step(dt, 0.001);
		_alpha.step(dt, 0.001);
		_level.step(dt, 0.001);
	}
	const auto moving = !_width.settled()
		|| !_height.settled()
		|| !_radius.settled()
		|| !_expanded.settled()
		|| !_alpha.settled()
		|| !_level.settled();
	const auto bars = _playing
		&& !PowerSaving::On(PowerSaving::kAnimations);
	if (bars) {
		_phase += elapsed;
	}
	if (!_shown && _alpha.value() <= 0.) {
		hide();
		_media = nullptr;
		Shortcuts::ToggleMediaShortcuts(false);
		return false;
	}
	applyGeometry();
	update();
	return moving || bars;
}

void Island::relayout() {
	if (!_shown && isHidden()) {
		return;
	}
	if (!areaVisible() || _anchor->width() <= 0) {
		hide();
		return;
	}
	const auto area = _anchor->geometry();
	_centerX = area.x() + area.width() / 2.;
	_shapeTop = area.y() + (st::topBarHeight - S(kCompactHeight)) / 2;
	const auto target = _wantExpanded ? expandedWidth() : compactWidth();
	if (_shown && _width.target() != target) {
		_width.to(target, kResize);
		startFrames();
	}
	applyGeometry();
	if (_shown || _alpha.value() > 0.) {
		show();
		raise();
	}
}

// WHY: форма считается в дробных координатах родителя, а окно только
// накрывает её целыми пикселями, иначе округление дёргает пилюлю
// на полпикселя каждый кадр.
void Island::applyGeometry() {
	const auto left = _centerX - _width.value() / 2.;
	const auto x = int(std::floor(left));
	const auto right = int(std::ceil(left + _width.value()));
	_shapeLeft = left - x;
	setGeometry(x, _shapeTop, right - x, int(std::ceil(_height.value())));
}

QRectF Island::shapeRect() const {
	return QRectF(_shapeLeft, 0., _width.value(), _height.value());
}

QRectF Island::coverRect() const {
	const auto shape = shapeRect();
	const auto small = S(24);
	const auto compact = QRectF(
		shape.x() + S(kCompactPadding),
		shape.y() + (S(kCompactHeight) - small) / 2.,
		small,
		small);
	const auto full = QRectF(
		shape.x() + S(kPadding),
		shape.y() + S(kPadding),
		S(kCover),
		S(kCover));
	return Lerp(compact, full, expanded());
}

QRectF Island::barsRect() const {
	const auto shape = shapeRect();
	const auto width = S(kBars * 3 + (kBars - 1) * 2);
	const auto height = S(16);
	const auto compact = QRectF(
		shape.x() + shape.width() - S(kCompactPadding + 4) - width,
		shape.y() + (S(kCompactHeight) - height) / 2.,
		width,
		height);
	const auto full = QRectF(
		shape.x() + shape.width() - S(kPadding) - width,
		shape.y() + S(kPadding) + (S(kCover) - height) / 2.,
		width,
		height);
	return Lerp(compact, full, expanded());
}

QRectF Island::progressRect() const {
	const auto shape = shapeRect();
	const auto font = style::font(S(12), 0, 0);
	const auto time = font->width(u"-00:00"_q) + S(8);
	const auto left = shape.x() + S(kPadding) + time;
	const auto right = shape.x() + shape.width() - S(kPadding) - time;
	return QRectF(
		left,
		shape.y() + S(kProgressY) - S(3),
		std::max(right - left, 0.),
		S(6));
}

QRectF Island::volumeRect() const {
	const auto shape = shapeRect();
	const auto left = shape.x() + S(kPadding + 20 + 10);
	const auto right = shape.x() + shape.width()
		- S(kPadding + 22 + 16 + 22 + 16 + 20 + 10);
	return QRectF(
		left,
		shape.y() + S(kVolumeY) - S(3),
		std::max(right - left, 0.),
		S(6));
}

QPointF Island::center(Part part) const {
	const auto shape = shapeRect();
	const auto middle = shape.x() + shape.width() / 2.;
	const auto right = shape.x() + shape.width() - S(kPadding);
	const auto transport = shape.y() + S(kTransportY);
	const auto volume = shape.y() + S(kVolumeY);
	switch (part) {
	case Part::Shuffle: return { middle - S(132), transport };
	case Part::Previous: return { middle - S(66), transport };
	case Part::Play: return { middle, transport };
	case Part::Next: return { middle + S(66), transport };
	case Part::Repeat: return { middle + S(132), transport };
	case Part::Speaker: return { shape.x() + S(kPadding + 10), volume };
	case Part::Lyrics: return { right - S(22 + 16 + 11), volume };
	case Part::Queue: return { right - S(11), volume };
	default: return { middle, transport };
	}
}

Island::Part Island::partAt(QPoint point) const {
	const auto shape = shapeRect();
	auto path = QPainterPath();
	const auto radius = std::clamp(_radius.value(), 0., shape.height() / 2.);
	path.addRoundedRect(shape, radius, radius);
	if (!path.contains(QPointF(point))) {
		return Part::None;
	} else if (expanded() < 0.6) {
		return Part::Body;
	}
	const auto near = [&](Part part, int hit) {
		const auto c = center(part);
		return std::abs(point.x() - c.x()) <= hit
			&& std::abs(point.y() - c.y()) <= hit;
	};
	for (const auto part : { Part::Previous, Part::Play, Part::Next }) {
		if (near(part, S(24))) {
			return part;
		}
	}
	for (const auto part : { Part::Shuffle, Part::Repeat, Part::Speaker, Part::Lyrics, Part::Queue }) {
		if (near(part, S(16))) {
			return part;
		}
	}
	if (progressRect().adjusted(0, -S(10), 0, S(10)).contains(QPointF(point))) {
		return Part::Progress;
	} else if (volumeRect().adjusted(-S(4), -S(10), S(4), S(10)).contains(QPointF(point))) {
		return Part::Volume;
	}
	const auto info = QRectF(
		shape.x(),
		shape.y(),
		shape.width(),
		S(kPadding + kCover + 8));
	return info.contains(QPointF(point)) ? Part::Cover : Part::Body;
}

double Island::RatioIn(const QRectF &rect, QPoint point) {
	return rect.width() > 0.
		? std::clamp((point.x() - rect.x()) / rect.width(), 0., 1.)
		: 0.;
}

void Island::setVolume(double volume, bool finished) {
	auto &settings = Core::App().settings();
	volume = std::clamp(volume, 0., 1.);
	if (volume > 0. && finished) {
		settings.setRememberedSongVolume(volume);
	}
	if (volume != settings.songVolume()) {
		Media::Player::mixer()->setSongVolume(volume);
		settings.setSongVolume(volume);
	}
	if (finished) {
		Core::App().saveSettingsDelayed();
	}
}

void Island::paintEvent(QPaintEvent *e) {
	const auto alpha = std::clamp(_alpha.value(), 0., 1.);
	if (alpha <= 0.) {
		return;
	}
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	p.setOpacity(alpha);
	const auto shape = shapeRect();
	const auto radius = std::clamp(_radius.value(), 0., shape.height() / 2.);
	auto path = QPainterPath();
	path.addRoundedRect(shape, radius, radius);
	p.fillPath(path, QColor(0, 0, 0));
	p.setClipPath(path);
	paintCover(p);
	paintBars(p);
	paintTitle(p);
	const auto progress = expanded();
	const auto content = std::clamp((progress - 0.45) / 0.55, 0., 1.);
	if (content > 0.) {
		const auto scale = Lerp(0.85, 1., progress);
		const auto origin = QPointF(shape.center().x(), shape.y());
		p.translate(origin);
		p.scale(scale, scale);
		p.translate(-origin);
		p.setOpacity(alpha * content);
		paintExpanded(p, shape);
	}
}

void Island::paintCover(QPainter &p) {
	const auto rect = coverRect();
	const auto radius = Lerp(S(7), S(12), expanded());
	auto path = QPainterPath();
	path.addRoundedRect(rect, radius, radius);
	const auto cover = CoverForPlayback(_track);
	if (cover.isNull()) {
		p.fillPath(path, QColor(0x28, 0x28, 0x28));
		return;
	}
	p.save();
	p.setClipPath(path, Qt::IntersectClip);
	const auto side = std::min(cover.width(), cover.height());
	const auto source = QRect(
		(cover.width() - side) / 2,
		(cover.height() - side) / 2,
		side,
		side);
	p.drawImage(rect, cover, source);
	p.restore();
}

void Island::paintBars(QPainter &p) {
	const auto rect = barsRect();
	const auto accent = CurrentCoverAccent();
	const auto color = accent.isValid() ? accent : QColor(Qt::white);
	const auto level = std::clamp(_level.value(), 0., 1.);
	const auto bar = S(3);
	const auto gap = S(2);
	static constexpr auto kSpeed = std::array{ 9.1, 6.7, 11.3, 7.9, 10.2 };
	static constexpr auto kShift = std::array{ 0.0, 1.7, 0.6, 2.4, 1.1 };
	p.setPen(Qt::NoPen);
	p.setBrush(color);
	for (auto i = 0; i != kBars; ++i) {
		const auto wave = 0.5 + 0.5 * std::sin(_phase * kSpeed[i] + kShift[i]);
		const auto value = Lerp(0.22, 0.3 + 0.7 * wave, level);
		const auto height = std::max(rect.height() * value, double(bar));
		const auto x = rect.x() + i * (bar + gap);
		const auto y = rect.y() + (rect.height() - height) / 2.;
		p.drawRoundedRect(QRectF(x, y, bar, height), bar / 2., bar / 2.);
	}
}

void Island::paintTitle(QPainter &p) {
	const auto e = expanded();
	const auto shape = shapeRect();
	const auto font = style::font(S(15), style::FontFlag::Bold, 0);
	const auto artist = style::font(S(13), 0, 0);
	const auto smallCover = S(24);
	const auto smallBars = S(kBars * 3 + (kBars - 1) * 2);
	const auto compactLeft = shape.x() + S(kCompactPadding) + smallCover + S(10);
	const auto compactRight = shape.x() + shape.width()
		- S(kCompactPadding + 4) - smallBars - S(10);
	const auto compactScale = 13. / 15.;
	const auto compact = QRectF(
		compactLeft,
		shape.y() + (S(kCompactHeight) - font->height * compactScale) / 2.,
		std::max(compactRight - compactLeft, 0.) / compactScale,
		font->height);
	const auto fullLeft = shape.x() + S(kPadding + kCover + 12);
	const auto fullRight = shape.x() + shape.width()
		- S(kPadding) - smallBars - S(12);
	const auto full = QRectF(
		fullLeft,
		shape.y() + S(kPadding)
			+ (S(kCover) - font->height - S(2) - artist->height) / 2.,
		std::max(fullRight - fullLeft, 0.),
		font->height);
	const auto scale = Lerp(compactScale, 1., e);
	const auto width = Lerp(compact.width(), full.width(), e);
	const auto origin = QPointF(
		Lerp(compact.x(), full.x(), e),
		Lerp(compact.y(), full.y(), e));
	p.save();
	p.translate(origin);
	p.scale(scale, scale);
	p.setPen(Qt::white);
	p.setFont(font);
	p.drawText(
		QRectF(0., 0., width, font->height),
		Qt::AlignLeft | Qt::AlignVCenter,
		font->elided(_title, int(width)));
	p.restore();
}

void Island::paintTrack(QPainter &p, const QRectF &rect, double ratio) {
	const auto line = rect.height() / 2.;
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(255, 255, 255, 64));
	p.drawRoundedRect(rect, line, line);
	p.setBrush(Qt::white);
	p.drawRoundedRect(
		QRectF(rect.x(), rect.y(), rect.width() * ratio, rect.height()),
		line,
		line);
}

void Island::paintToggle(
		QPainter &p,
		Part part,
		Glyph glyph,
		bool active,
		bool enabled) {
	const auto c = center(part);
	const auto opacity = p.opacity();
	if (active) {
		const auto r = S(15);
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(255, 255, 255, 46));
		p.drawRoundedRect(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r), r, r);
	}
	p.setOpacity(opacity * (!enabled ? 0.3 : active ? 1. : 0.6));
	PaintGlyph(p, glyph, c, S(22));
	p.setOpacity(opacity);
}

void Island::paintExpanded(QPainter &p, const QRectF &shape) {
	const auto cover = coverRect();
	const auto bars = barsRect();
	const auto left = int(cover.x() + cover.width() + S(12));
	const auto width = std::max(int(bars.x()) - S(12) - left, 0);
	const auto titleFont = style::font(S(15), style::FontFlag::Bold, 0);
	const auto artistFont = style::font(S(13), 0, 0);
	const auto timeFont = style::font(S(12), 0, 0);
	const auto top = int(cover.y() + (cover.height()
		- titleFont->height - S(2) - artistFont->height) / 2.);
	p.setPen(QColor(0x8e, 0x8e, 0x93));
	p.setFont(artistFont);
	p.drawText(
		QRect(left, top + titleFont->height + S(2), width, artistFont->height),
		Qt::AlignLeft | Qt::AlignVCenter,
		artistFont->elided(_artist, width));

	const auto progress = progressRect();
	const auto ratio = (_dragging == Part::Progress)
		? _seekProgress
		: (_length > 0)
		? std::clamp(_position / double(_length), 0., 1.)
		: 0.;
	const auto played = crl::time(base::SafeRound(ratio * _length));
	paintTrack(p, progress, ratio);
	p.setPen(QColor(0x8e, 0x8e, 0x93));
	p.setFont(timeFont);
	const auto timeTop = int(progress.center().y()) - timeFont->height / 2;
	const auto timeWidth = int(progress.x() - shape.x()) - S(kPadding) - S(8);
	p.drawText(
		QRect(int(shape.x()) + S(kPadding), timeTop, timeWidth, timeFont->height),
		Qt::AlignLeft | Qt::AlignVCenter,
		Ui::FormatDurationText(played / 1000));
	p.drawText(
		QRect(int(progress.x() + progress.width()) + S(8), timeTop, timeWidth, timeFont->height),
		Qt::AlignRight | Qt::AlignVCenter,
		u"-"_q + Ui::FormatDurationText(std::max(_length - played, crl::time(0)) / 1000));

	const auto instance = Media::Player::instance();
	const auto opacity = p.opacity();
	const auto press = [&](Part part, int size) {
		return (_pressed == part) ? int(size * 0.88) : size;
	};
	const auto &settings = Core::App().settings();
	paintToggle(
		p,
		Part::Shuffle,
		Glyph::Shuffle,
		settings.playerOrderMode() == Media::OrderMode::Shuffle,
		true);
	p.setOpacity(opacity * (instance->previousAvailable(kType) ? 1. : 0.4));
	PaintGlyph(p, Glyph::Back, center(Part::Previous), press(Part::Previous, S(34)));
	p.setOpacity(opacity);
	PaintGlyph(
		p,
		_playing ? Glyph::Pause : Glyph::Play,
		center(Part::Play),
		press(Part::Play, S(30)));
	p.setOpacity(opacity * (instance->nextAvailable(kType) ? 1. : 0.4));
	PaintGlyph(p, Glyph::Forward, center(Part::Next), press(Part::Next, S(34)));
	p.setOpacity(opacity);
	const auto repeat = settings.playerRepeatMode();
	paintToggle(
		p,
		Part::Repeat,
		(repeat == Media::RepeatMode::One) ? Glyph::RepeatOne : Glyph::Repeat,
		repeat != Media::RepeatMode::None,
		true);

	const auto volume = volumeRect();
	const auto speaker = center(Part::Speaker);
	p.setOpacity(opacity * 0.6);
	PaintGlyph(p, Glyph::Speaker, speaker, S(20));
	PaintGlyph(
		p,
		Glyph::Output,
		QPointF(volume.x() + volume.width() + S(10 + 10), speaker.y()),
		S(20));
	p.setOpacity(opacity);
	paintTrack(p, volume, std::clamp(settings.songVolume(), 0., 1.));
	paintToggle(
		p,
		Part::Lyrics,
		Glyph::Lyrics,
		Mokagram::Settings::Bool(u"lyrics.enabled"_q),
		Mokagram::Settings::Bool(u"lyrics.enabled"_q) || LyricsAvailable());
	paintToggle(p, Part::Queue, Glyph::Queue, false, true);
}

void Island::enterEventHook(QEnterEvent *e) {
	_leaveTimer.cancel();
	if (!_wantExpanded) {
		_hoverTimer.callOnce(kHoverDelay);
	}
	RpWidget::enterEventHook(e);
}

void Island::leaveEventHook(QEvent *e) {
	_hoverTimer.cancel();
	if (_wantExpanded) {
		_leaveTimer.callOnce(kLeaveDelay);
	}
	setCursor(style::cur_default);
	RpWidget::leaveEventHook(e);
}

void Island::dragTo(QPoint point) {
	if (_dragging == Part::Progress) {
		_seekProgress = RatioIn(progressRect(), point);
		Media::Player::instance()->updateSeeking(kType, _seekProgress);
	} else if (_dragging == Part::Volume) {
		setVolume(RatioIn(volumeRect(), point), false);
	}
	update();
}

void Island::mouseMoveEvent(QMouseEvent *e) {
	const auto part = partAt(e->pos());
	if (part == Part::None && _wantExpanded && _dragging == Part::None) {
		_leaveTimer.callOnce(kLeaveDelay);
	} else if (part != Part::None) {
		_leaveTimer.cancel();
	}
	if (_dragging != Part::None) {
		dragTo(e->pos());
	}
	const auto clickable = (part != Part::None)
		&& (part != Part::Body || !_wantExpanded);
	setCursor(clickable ? style::cur_pointer : style::cur_default);
}

void Island::mousePressEvent(QMouseEvent *e) {
	if (e->button() != Qt::LeftButton) {
		return;
	}
	_pressed = partAt(e->pos());
	if (_pressed == Part::Progress && _length > 0) {
		_dragging = Part::Progress;
		Media::Player::instance()->startSeeking(kType);
		dragTo(e->pos());
	} else if (_pressed == Part::Volume) {
		_dragging = Part::Volume;
		dragTo(e->pos());
	}
	update();
}

void Island::mouseReleaseEvent(QMouseEvent *e) {
	if (e->button() != Qt::LeftButton) {
		return;
	}
	const auto pressed = std::exchange(_pressed, Part::None);
	const auto dragging = std::exchange(_dragging, Part::None);
	if (dragging == Part::Progress) {
		_seekProgress = RatioIn(progressRect(), e->pos());
		Media::Player::instance()->finishSeeking(kType, _seekProgress);
	} else if (dragging == Part::Volume) {
		setVolume(RatioIn(volumeRect(), e->pos()), true);
	} else if (pressed != Part::None && pressed == partAt(e->pos())) {
		activate(pressed);
	}
	update();
}

void Island::activate(Part part) {
	const auto instance = Media::Player::instance();
	auto &settings = Core::App().settings();
	switch (part) {
	case Part::Body:
		expand(true);
		break;
	case Part::Cover:
		expand(false);
		ShowFullscreenPlayer(_controller);
		break;
	case Part::Previous:
		instance->previous(kType);
		break;
	case Part::Play:
		instance->playPauseCancelClicked(kType);
		break;
	case Part::Next:
		instance->next(kType);
		break;
	case Part::Shuffle:
		settings.setPlayerOrderMode(
			(settings.playerOrderMode() == Media::OrderMode::Shuffle)
				? Media::OrderMode::Default
				: Media::OrderMode::Shuffle);
		Core::App().saveSettingsDelayed();
		break;
	case Part::Repeat:
		settings.setPlayerRepeatMode([&] {
			switch (settings.playerRepeatMode()) {
			case Media::RepeatMode::None: return Media::RepeatMode::One;
			case Media::RepeatMode::One: return Media::RepeatMode::All;
			case Media::RepeatMode::All: return Media::RepeatMode::None;
			}
			return Media::RepeatMode::None;
		}());
		Core::App().saveSettingsDelayed();
		break;
	case Part::Speaker:
		setVolume(
			(settings.songVolume() > 0.)
				? 0.
				: settings.rememberedSongVolume(),
			true);
		break;
	case Part::Lyrics:
		if (Mokagram::Settings::Bool(u"lyrics.enabled"_q) || LyricsAvailable()) {
			ToggleLyricsPanel(_controller, this);
		}
		break;
	case Part::Queue:
		expand(false);
		ShowPlayerBox(_controller);
		break;
	default:
		break;
	}
	update();
}

void Island::wheelEvent(QWheelEvent *e) {
	const auto delta = e->angleDelta().y();
	if (!delta) {
		return;
	}
	const auto volume = Core::App().settings().songVolume();
	setVolume(volume + kWheelStep * (delta / 120.), true);
	e->accept();
}

void Island::contextMenuEvent(QContextMenuEvent *e) {
	if (partAt(e->pos()) == Part::None) {
		return;
	}
	_menu = base::make_unique_q<Ui::PopupMenu>(this, st::popupMenuWithIcons);
	_menu->addAction(
		QString::fromUtf8(Mokagram::Lang::Tr("Закрыть")),
		[] { Media::Player::instance()->stopAndClose(); },
		&st::menuIconCancel);
	_menu->popup(e->globalPos());
}

[[nodiscard]] base::flat_map<QWidget*, QPointer<Island>> &Islands() {
	static auto result = base::flat_map<QWidget*, QPointer<Island>>();
	return result;
}

} // namespace

void UpdateIsland(
		not_null<Ui::RpWidget*> parent,
		not_null<Window::SessionController*> controller,
		not_null<Ui::RpWidget*> anchor,
		bool create) {
	auto &islands = Islands();
	auto island = islands[parent.get()].data();
	if (!island) {
		if (!create) {
			islands.remove(parent.get());
			return;
		}
		island = Ui::CreateChild<Island>(parent.get(), controller, anchor);
		islands[parent.get()] = island;
	}
	if (create) {
		island->appear();
	}
	island->relayout();
}

} // namespace Mokagram::Music
