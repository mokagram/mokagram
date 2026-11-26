/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/abstract_button.h"
#include "data/data_audio_msg_id.h"

namespace Data {
class DocumentMedia;
} // namespace Data

namespace Media::Player {
struct TrackState;
} // namespace Media::Player

namespace Info::Profile {

struct PillColors {
	QColor fill;
	QColor primary;
	QColor secondary;
};

[[nodiscard]] PillColors ComputePillColors(std::optional<QColor> edge);
void PaintPillBackground(
	QPainter &p,
	const QRect &rect,
	const PillColors &colors);
[[nodiscard]] int PillTextBaseline();
[[nodiscard]] int PillArrowTop();

class MusicButton final : public Ui::AbstractButton {
public:
	explicit MusicButton(QWidget *parent);
	~MusicButton();

	// Mokagram: the row opens the music list; the trailing button plays.
	[[nodiscard]] static int RowHeight();
	void setTrack(const AudioMsgId &track);
	void setOverrideBg(std::optional<QColor> color);
	void setOpacity(float64 opacity);
	[[nodiscard]] bool actionable() const;

private:
	void paintEvent(QPaintEvent *e) override;
	void focusInEvent(QFocusEvent *e) override;
	void focusOutEvent(QFocusEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;
	void resizeEvent(QResizeEvent *e) override;
	int resizeGetHeight(int newWidth) override;

	class PlaybackButton;
	void updatePlayback(const ::Media::Player::TrackState &state);
	void refreshText(int width);
	[[nodiscard]] bool hasCover() const;
	[[nodiscard]] int textLeft() const;

	const std::unique_ptr<PlaybackButton> _playback;
	AudioMsgId _track;
	std::shared_ptr<Data::DocumentMedia> _media;
	rpl::lifetime _coverLifetime;
	QString _title;
	QString _performer;
	QString _shownTitle;
	QString _shownPerformer;
	std::optional<QColor> _overrideBg;
	PillColors _colors;
	float64 _opacity = 0.;

};

} // namespace Info::Profile
