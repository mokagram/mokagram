/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "window/window_slide_animation.h"

#include "styles/style_basic.h"


namespace Window {

void SlideAnimation::paintContents(QPainter &p) const {
	const auto retina = style::DevicePixelRatio();

	const auto slideLeft = (_direction == SlideDirection::FromLeft);
	const auto progress = _animation.value(slideLeft ? 0. : 1.);
	if (_withFade) {
		const auto dt = slideLeft
			? (1. - progress)
			: progress;
		const auto easeOut = anim::easeOutCirc(1., dt);
		const auto easeIn = anim::easeInCirc(1., dt);
		const auto arrivingAlpha = easeIn;
		const auto departingAlpha = 1. - easeOut;
		const auto leftWidthFull = _cacheUnder.width() / retina;
		const auto rightWidthFull = _cacheOver.width() / retina;
		const auto leftCoord = slideLeft
			? anim::interpolate(-leftWidthFull, 0, easeOut)
			: anim::interpolate(0, -leftWidthFull, easeIn);
		const auto leftAlpha = (slideLeft ? arrivingAlpha : departingAlpha);
		const auto rightCoord = slideLeft
			? anim::interpolate(0, rightWidthFull, easeIn)
			: anim::interpolate(rightWidthFull, 0, easeOut);
		const auto rightAlpha = (slideLeft ? departingAlpha : arrivingAlpha);

		const auto leftWidth = (leftWidthFull + leftCoord);
		const auto rightWidth = rightWidthFull - rightCoord;

		if (!_mask.isNull()) {
			auto frame = QImage(
				_mask.size(),
				QImage::Format_ARGB32_Premultiplied);
			frame.setDevicePixelRatio(_mask.devicePixelRatio());
			frame.fill(Qt::transparent);
			QPainter q(&frame);

			if (leftWidth > 0) {
				q.setOpacity(leftAlpha);
				q.drawPixmap(
					0,
					0,
					_cacheUnder,
					_cacheUnder.width()
						- leftWidth * style::DevicePixelRatio(),
					0,
					leftWidth * style::DevicePixelRatio(),
					_topSkip * retina);
			}

			if (rightWidth > 0) {
				q.setOpacity(rightAlpha);
				q.drawPixmap(
					rightCoord,
					0,
					_cacheOver,
					0,
					0,
					rightWidth * style::DevicePixelRatio(),
					_topSkip * retina);
			}

			q.setOpacity(1.);
			q.setCompositionMode(QPainter::CompositionMode_DestinationIn);
			q.drawPixmap(0, 0, _mask);

			p.drawImage(0, 0, frame);
		}

		if (leftWidth > 0) {
			p.setOpacity(leftAlpha);
			p.drawPixmap(
				0,
				_topSkip,
				_cacheUnder,
				(_cacheUnder.width() - leftWidth * retina),
				_topSkip * retina,
				leftWidth * retina,
				_cacheUnder.height() - _topSkip * retina);
		}
		if (rightWidth > 0) {
			p.setOpacity(rightAlpha);
			p.drawPixmap(
				rightCoord,
				_topSkip,
				_cacheOver,
				0,
				_topSkip * retina,
				rightWidth * retina,
				_cacheOver.height() - _topSkip * retina);
		}
	} else if (_direction == SlideDirection::FromBottom) {
		const auto height = _cacheOver.height() / retina;
		const auto offset = anim::interpolate(0, height, progress);
		p.drawPixmap(
			QRect(QPoint(0, -offset), _cacheUnder.size() / retina),
			_cacheUnder,
			QRect(QPoint(), _cacheUnder.size()));
		p.drawPixmap(
			QRect(QPoint(0, height - offset), _cacheOver.size() / retina),
			_cacheOver,
			QRect(QPoint(), _cacheOver.size()));
	} else {
		// Mokagram: общие переходы не должны гонять весь экран из края в
		// край. Короткий параллакс и кросс-фейд сохраняют контекст чата,
		// настроек и инфо-панели, но ощущаются заметно легче.
		const auto width = _cacheOver.width() / retina;
		const auto travel = std::min(style::ConvertScale(56), width / 4);
		const auto coordUnder = anim::interpolate(0, -travel / 3, progress);
		const auto coordOver = anim::interpolate(travel, 0, progress);
		const auto height = _cacheOver.height() / retina;

		p.save();
		p.setOpacity(1. - (0.32 * progress));
		p.drawPixmap(QPoint(coordUnder, 0), _cacheUnder);
		p.restore();

		p.save();
		p.setOpacity(progress);
		p.drawPixmap(QPoint(coordOver, 0), _cacheOver);
		p.restore();

		if (progress > 0.) {
			p.setOpacity(progress * 0.72);
			st::slideShadow.fill(
				p,
				QRect(
					coordOver - st::slideShadow.width(),
					0,
					st::slideShadow.width(),
					height));
			p.setOpacity(1.);
		}
	}
}

float64 SlideAnimation::progress() const {
	const auto slideLeft = (_direction == SlideDirection::FromLeft);
	const auto progress = _animation.value(slideLeft ? 0. : 1.);
	return slideLeft ? (1. - progress) : progress;
}

void SlideAnimation::setDirection(SlideDirection direction) {
	_direction = direction;
}

void SlideAnimation::setPixmaps(
		const QPixmap &oldContentCache,
		const QPixmap &newContentCache) {
	_cacheUnder = oldContentCache;
	_cacheOver = newContentCache;
}

void SlideAnimation::setTopBarShadow(bool enabled) {
	_topBarShadowEnabled = enabled;
}

void SlideAnimation::setTopSkip(int skip) {
	_topSkip = skip;
}

void SlideAnimation::setWithFade(bool withFade) {
	_withFade = withFade;
}

void Window::SlideAnimation::setDuration(crl::time duration) {
	_duration = duration;
}

void SlideAnimation::setRepaintCallback(RepaintCallback &&callback) {
	_repaintCallback = std::move(callback);
}

void SlideAnimation::setFinishedCallback(FinishedCallback &&callback) {
	_finishedCallback = std::move(callback);
}

void SlideAnimation::setTopBarMask(const QPixmap &mask) {
	_mask = mask;
}

void SlideAnimation::start() {
	const auto fromLeft = (_direction == SlideDirection::FromLeft);
	if (fromLeft) {
		std::swap(_cacheUnder, _cacheOver);
	}
	const auto fromBottom = (_direction == SlideDirection::FromBottom);
	const auto curve = fromBottom ? anim::easeOutQuint : transition();
	const auto duration = fromBottom
		? (st::slideDuration * 2)
		: (_duration ? _duration : st::slideDuration);
	_animation.start(
		[this] { animationCallback(); },
		fromLeft ? 1. : 0.,
		fromLeft ? 0. : 1.,
		duration,
		curve);
	if (const auto onstack = _repaintCallback) {
		onstack();
	}
}

void SlideAnimation::animationCallback() {
	if (const auto onstack = _repaintCallback) {
		onstack();
	}
	if (!_animation.animating()) {
		if (const auto onstack = _finishedCallback) {
			onstack();
		}
	}
}

} // namespace Window
