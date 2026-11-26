/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/profile/info_profile_music_button.h"

#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_file_origin.h"
#include "history/view/media/history_view_document.h"
#include "main/main_session.h"
#include "media/audio/media_audio.h"
#include "media/player/media_player_instance.h"
#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/settings/mokagram_settings_box.h"
#include "ui/effects/animation_value.h"
#include "ui/color_contrast.h"
#include "ui/painter.h"
#include "ui/text/format_song_document_name.h"
#include "ui/text/text_custom_emoji.h"
#include "ui/ui_utility.h"

#include "styles/style_info.h"
#include "styles/style_info_profile_top_bar.h"

namespace Info::Profile {
namespace {

[[nodiscard]] float64 FigureHeight(const style::font &font) {
	return -font->metrics().tightBoundingRect(u"0123456789"_q).top();
}

// Mokagram: separate keyboard and pointer targets for navigation and playback.
constexpr auto kInset = 8;
constexpr auto kCover = 34;
constexpr auto kPlaybackTarget = 40;

void SetTrackFocus(Ui::AbstractButton &button, bool interactive) {
	button.setFocusPolicy(interactive ? Qt::TabFocus : Qt::NoFocus);
	if (!interactive) {
		button.clearFocus();
		button.clearState();
	}
}

void PaintFocus(QPainter &p, QRectF rect, int radius, QColor color) {
	const auto inset = style::ConvertScale(2);
	rect.adjust(inset, inset, -inset, -inset);
	p.setPen(QPen(color, inset));
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(rect, radius - inset, radius - inset);
}

} // namespace

PillColors ComputePillColors(std::optional<QColor> edge) {
	if (!edge) {
		return {
			.fill = anim::with_alpha(
				st::boxBg->c,
				1. - st::infoProfileTopBarActionButtonBgOpacity),
			.primary = st::windowBoldFg->c,
			.secondary = st::windowSubTextFg->c,
		};
	}
	const auto light = Ui::IsLightBackground(*edge);
	return {
		.fill = Ui::BlendColors(
			*edge,
			Qt::black,
			st::infoProfileTopBarActionButtonBgOpacity),
		.primary = light ? QColor(Qt::black) : st::groupCallMembersFg->c,
		.secondary = light
			? anim::with_alpha(
				QColor(Qt::black),
				st::groupCallVideoSubTextFg->c.alphaF())
			: st::groupCallVideoSubTextFg->c,
	};
}

void PaintPillBackground(
		QPainter &p,
		const QRect &rect,
		const PillColors &colors) {
	auto hq = PainterHighQualityEnabler(p);
	const auto radius = rect.height() / 2.;
	p.setPen(Qt::NoPen);
	p.setBrush(colors.fill);
	p.drawRoundedRect(rect, radius, radius);
}

int PillTextBaseline() {
	const auto figure = FigureHeight(st::infoProfileBalancePillAmountFont);
	return int(base::SafeRound(
		(st::infoProfileBalancePillHeight + figure) / 2.));
}

int PillArrowTop() {
	const auto figure = FigureHeight(st::infoProfileBalancePillAmountFont);
	const auto arrow = st::infoProfileBalancePillArrow.height();
	return int(base::SafeRound(PillTextBaseline() - (figure + arrow) / 2.));
}

class MusicButton::PlaybackButton final : public Ui::AbstractButton {
public:
	explicit PlaybackButton(MusicButton *parent)
	: AbstractButton(parent)
	, _row(parent)
	, _playLabel(QString::fromUtf8(Mokagram::Lang::Tr("Воспроизвести")))
	, _playText(QString::fromUtf8(Mokagram::Lang::Tr("Пуск")))
	, _pauseLabel(QString::fromUtf8(Mokagram::Lang::Tr("Пауза"))) {
		setAccessibleName(_playLabel);
		const auto refreshIcons = [=] {
			const auto repaint = [=] { update(); };
			const auto size = style::ConvertScale(18);
			using Mokagram::Settings::AltIconEmoji;
			_playIcon = AltIconEmoji(u"▶️"_q, repaint, size);
			_pauseIcon = AltIconEmoji(u"⏸️"_q, repaint, size);
			update();
		};
		Mokagram::Settings::RowIconsUpdated(
		) | rpl::on_next(refreshIcons, lifetime());
		refreshIcons();
	}

	void setPlaying(bool playing) {
		if (_playing != playing) {
			_playing = playing;
			setAccessibleName(_playing ? _pauseLabel : _playLabel);
			update();
		}
	}

private:
	void paintEvent(QPaintEvent *e) override {
		if (_row->_opacity <= 0.) {
			return;
		}
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		p.setOpacity(_row->_opacity);
		const auto &colors = _row->_colors;
		const auto size = style::ConvertScale(36);
		const auto circle = QRect(
			(width() - size) / 2,
			(height() - size) / 2,
			size,
			size);
		p.setPen(Qt::NoPen);
		p.setBrush(Ui::BlendColors(
			colors.fill,
			colors.primary,
			isDown() ? 0.24 : isOver() ? 0.18 : 0.10));
		p.drawEllipse(circle);
		if (hasFocus()) {
			PaintFocus(p, circle, size / 2, colors.primary);
		}
		const auto &icon = _playing ? _pauseIcon : _playIcon;
		if (icon && icon->ready()) {
			icon->paint(p, {
				.textColor = colors.primary,
				.now = crl::now(),
				.position = circle.topLeft()
					+ QPoint(1, 1) * ((size - icon->width()) / 2),
				.paused = true,
			});
		} else {
			p.setPen(colors.primary);
			p.setFont(st::infoProfileBalancePillFractionFont);
			p.drawText(circle, Qt::AlignCenter, _playing ? _pauseLabel : _playText);
		}
	}

	void focusInEvent(QFocusEvent *e) override {
		AbstractButton::focusInEvent(e);
		update();
	}

	void focusOutEvent(QFocusEvent *e) override {
		AbstractButton::focusOutEvent(e);
		clearState();
		update();
	}

	void onStateChanged(State was, StateChangeSource source) override {
		update();
	}

	const not_null<MusicButton*> _row;
	const QString _playLabel;
	const QString _playText;
	const QString _pauseLabel;
	std::unique_ptr<Ui::Text::CustomEmoji> _playIcon;
	std::unique_ptr<Ui::Text::CustomEmoji> _pauseIcon;
	bool _playing = false;
};

MusicButton::MusicButton(QWidget *parent)
: AbstractButton(parent)
, _playback(std::make_unique<PlaybackButton>(this))
, _colors(ComputePillColors(std::nullopt)) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	hide();

	const auto player = ::Media::Player::instance();
	_playback->setClickedCallback([=] {
		if (actionable()) {
			player->playPause(_track);
		}
	});
	player->updatedNotifier(
	) | rpl::on_next([=](const ::Media::Player::TrackState &state) {
		if (state.id.type() == AudioMsgId::Type::Song) {
			updatePlayback(state);
		}
	}, lifetime());
	player->stops(
		AudioMsgId::Type::Song
	) | rpl::on_next([=] {
		_playback->setPlaying(false);
	}, lifetime());
	style::PaletteChanged(
	) | rpl::on_next([=] {
		_colors = ComputePillColors(_overrideBg);
		update();
		_playback->update();
	}, lifetime());
}

MusicButton::~MusicButton() = default;

int MusicButton::RowHeight() {
	return style::ConvertScale(48);
}

void MusicButton::setTrack(const AudioMsgId &track) {
	_track = track;
	_coverLifetime.destroy();
	_media.reset();
	if (const auto document = track.audio()) {
		auto name = Ui::Text::FormatSongNameFor(document).composedName();
		_title = std::move(name.title);
		_performer = std::move(name.performer);
		_media = document->createMediaView();
		if (document->hasThumbnail() && !_media->thumbnail()) {
			document->session().downloaderTaskFinished(
			) | rpl::on_next([=] {
				if (_media->thumbnail()) {
					_coverLifetime.destroy();
					refreshText(width());
					update();
				}
			}, _coverLifetime);
			_media->thumbnailWanted(track.contextId());
		}
		setAccessibleName(_performer.isEmpty()
			? _title
			: _title + u", "_q + _performer);
		updatePlayback(::Media::Player::instance()->getState(
			AudioMsgId::Type::Song));
		refreshText(width());
		update();
	}
	setVisible(bool(track));
}

void MusicButton::updatePlayback(const ::Media::Player::TrackState &state) {
	_playback->setPlaying(_track
		&& state.id.audio() == _track.audio()
		&& state.id.contextId() == _track.contextId()
		&& ::Media::Player::ShowPauseIcon(state.state));
}

void MusicButton::setOverrideBg(std::optional<QColor> color) {
	if (_overrideBg == color) {
		return;
	}
	_overrideBg = color;
	_colors = ComputePillColors(color);
	update();
	_playback->update();
}

void MusicButton::setOpacity(float64 opacity) {
	if (_opacity != opacity) {
		_opacity = opacity;
		update();
		_playback->update();
	}
	const auto transparent = (_opacity <= 0.);
	if (testAttribute(Qt::WA_TransparentForMouseEvents) != transparent) {
		setAttribute(Qt::WA_TransparentForMouseEvents, transparent);
		SetTrackFocus(*this, !transparent);
		SetTrackFocus(*_playback, !transparent);
	}
}

bool MusicButton::actionable() const {
	return bool(_track) && !isHidden() && (_opacity > 0.);
}

bool MusicButton::hasCover() const {
	return _media && (_media->thumbnail() || _media->thumbnailInline());
}

int MusicButton::textLeft() const {
	const auto inset = style::ConvertScale(kInset);
	return inset + (hasCover() ? style::ConvertScale(kCover) + inset : 0);
}

void MusicButton::refreshText(int width) {
	const auto room = std::max(
		width - textLeft() - style::ConvertScale(kPlaybackTarget + kInset),
		0);
	_shownTitle = st::infoProfileBalancePillAmountFont->elided(_title, room);
	_shownPerformer = st::infoProfileBalancePillLabelFont->elided(
		_performer,
		room);
}

void MusicButton::paintEvent(QPaintEvent *e) {
	if (_opacity <= 0.) {
		return;
	}
	auto p = Painter(this);
	p.setOpacity(_opacity);
	auto colors = _colors;
	if (isOver() || isDown()) {
		colors.fill = Ui::BlendColors(
			colors.fill,
			colors.primary,
			isDown() ? 0.12 : 0.06);
	}
	PaintPillBackground(p, rect(), colors);
	if (hasFocus()) {
		auto hq = PainterHighQualityEnabler(p);
		PaintFocus(p, rect(), height() / 2, colors.primary);
	}
	if (hasCover()) {
		const auto inset = style::ConvertScale(kInset);
		const auto cover = style::ConvertScale(kCover);
		HistoryView::DrawThumbnailAsSongCover(
			p,
			st::songCoverOverlayFg,
			_media,
			myrtlrect(inset, (height() - cover) / 2, cover, cover));
	}
	const auto &titleFont = st::infoProfileBalancePillAmountFont;
	const auto &performerFont = st::infoProfileBalancePillLabelFont;
	const auto both = !_shownPerformer.isEmpty();
	const auto textHeight = titleFont->height + (both ? performerFont->height : 0);
	const auto left = textLeft();
	const auto top = (height() - textHeight) / 2;
	p.setPen(colors.primary);
	p.setFont(titleFont);
	p.drawTextLeft(left, top, width(), _shownTitle);
	if (both) {
		p.setPen(colors.secondary);
		p.setFont(performerFont);
		p.drawTextLeft(left, top + titleFont->height, width(), _shownPerformer);
	}
}

void MusicButton::focusInEvent(QFocusEvent *e) {
	AbstractButton::focusInEvent(e);
	update();
}

void MusicButton::focusOutEvent(QFocusEvent *e) {
	AbstractButton::focusOutEvent(e);
	clearState();
	update();
}

void MusicButton::onStateChanged(State was, StateChangeSource source) {
	update();
}

void MusicButton::resizeEvent(QResizeEvent *e) {
	AbstractButton::resizeEvent(e);
	const auto target = style::ConvertScale(kPlaybackTarget);
	_playback->setGeometryToRight(style::ConvertScale(4), 0, target, height());
	refreshText(width());
}

int MusicButton::resizeGetHeight(int newWidth) {
	return RowHeight();
}

} // namespace Info::Profile
