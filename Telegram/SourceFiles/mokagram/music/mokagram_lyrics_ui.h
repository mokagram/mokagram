// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#pragma once

#include "mokagram/music/mokagram_lyrics.h"
#include "ui/effects/animations.h"
#include "ui/rp_widget.h"

#include <QtCore/QCache>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QImage>

class QTextLayout;

namespace Mokagram::Music {

struct Track;

namespace details {

class TimedComments;

inline constexpr auto kSwitchDuration = crl::time(220);

[[nodiscard]] Track PlayingTrack();
void ResolveCommentsTrack(const Track &track, Fn<void(QString)> done);
void LoadPlaybackArtwork(const Track &track);

[[nodiscard]] QImage CoverBackgroundForPlayback();

class CoverBackground final {
public:
	void refresh(Fn<void()> repaint, bool visible);
	void paint(QPainter &p, QRect area, QRect canvas) const;

private:
	[[nodiscard]] QImage frame() const;

	QImage _from;
	QImage _to;
	Ui::Animations::Simple _animation;

};

class LyricsView final : public Ui::RpWidget {
public:
	explicit LyricsView(QWidget *parent);
	~LyricsView() override;

	void setBig(bool big);
	void setBackground(Fn<void(QPainter&, QRect)> paint);
	void setCoverTint(bool on);
	void setLoading(const QString &trackId);
	void setLyrics(Lyrics lyrics);
	void updateCover();
	void setCommentsTrack(const QString &id);
	void followPlayback();
	void setPosition(qint64 ms, bool playing, double speed);

	[[nodiscard]] rpl::producer<qint64> lineClicked() const;

protected:
	void showEvent(QShowEvent *e) override;
	void hideEvent(QHideEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;

private:
	[[nodiscard]] int lineAt(int y);
	[[nodiscard]] QPointF lineOrigin(double top) const;
	void loadTimingOffset(const QString &json);
	void updateTimer();
	void reset();
	void invalidateLayouts();
	void ensureLinePositions(int count);
	[[nodiscard]] double topOf(int index);
	[[nodiscard]] int lineAtOffset(double y);

	[[nodiscard]] qint64 livePosition() const;
	void tick();
	[[nodiscard]] double lineProgress(int index, qint64 at) const;
	[[nodiscard]] double anchorY() const;
	[[nodiscard]] int textWidth() const;
	[[nodiscard]] QFont lineFont() const;
	struct LineLayout {
		std::unique_ptr<QTextLayout> main;
		std::unique_ptr<QTextLayout> subtitle;
	};
	[[nodiscard]] const LineLayout &layoutOf(int index);
	[[nodiscard]] int heightOf(int index);
	void drawLayout(
			QPainter &p,
			const QTextLayout &layout,
			QPointF origin,
			double fill,
			double sung,
			double unsung);
	struct LineRaster {
		QImage image;
		QPointF offset;
	};
	[[nodiscard]] LineRaster rasterizeLayout(
		const QTextLayout &layout,
		QPointF origin,
		qreal ratio) const;
	bool drawCachedLayout(
		QPainter &p,
		const QTextLayout &layout,
		QPointF origin,
		double opacity);
	[[nodiscard]] double centerOf(int index);

	void paintEvent(QPaintEvent *e) override;

	int lyricsHeight() const;
	void layoutComments();

	rpl::event_stream<qint64> _lineClicked;
	const not_null<TimedComments*> _comments;
	Lyrics _lyrics;
	QString _status;
	QTimer _timer;
	QElapsedTimer _clock;
	std::vector<int> _heights;
	std::vector<double> _lineTops;
	std::vector<LineLayout> _layouts;
	QCache<const QTextLayout*, LineRaster> _lineRasters{ 4 * 1024 * 1024 };
	qreal _lineRasterRatio = 0.;
	int _subtitleMode = 0;
	QString _timingTrack;
	int _timingOffset = 0;
	CoverBackground _backdrop;
	Fn<void(QPainter&, QRect)> _background;
	std::vector<float> _weights;
	std::vector<int> _fadingWeights;
	double _fill = 0.;
	qint64 _position = 0;
	qint64 _positionAt = 0;
	double _speed = 1.;
	qint64 _last = 0;
	double _scroll = 0.;
	double _plainScroll = 0.;
	qint64 _manualUntil = 0;
	int _current = -1;
	bool _scrollReady = false;
	bool _coverTint = true;
	bool _playing = false;
	bool _big = false;

};

} // namespace details
} // namespace Mokagram::Music
