/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/message_bubble.h"

#include "ui/cached_round_corners.h"
#include "ui/image/image_prepare.h"
#include "ui/chat/chat_style.h"
#include "ui/chat/torn_edge.h"
#include "ui/painter.h"
#include "styles/style_chat.h"
#include "styles/style_chat_style.h"

namespace Ui {
namespace {

using Corner = BubbleCornerRounding;

[[nodiscard]] bool UsePatternBubble(const SimpleBubble &args) {
	return !args.selected
		&& args.outbg
		&& args.pattern
		&& !args.patternViewport.isEmpty()
		&& !args.pattern->pixmap.size().isEmpty();
}

[[nodiscard]] float64 PatternBubbleOpacity(
		const SimpleBubble &args,
		float64 wasOpacity) {
	return args.st->msgOutBg()->c.alphaF() * wasOpacity;
}

void PaintBubblePiece(
		QPainter &p,
		const SimpleBubble &args,
		QRect geometry,
		bool fromTop,
		bool tillBottom) {
	auto simple = args;
	simple.geometry = geometry;
	if (!fromTop) {
		simple.rounding.topLeft
			= simple.rounding.topRight
			= Corner::None;
	}
	if (!tillBottom) {
		simple.rounding.bottomLeft
			= simple.rounding.bottomRight
			= Corner::None;
		simple.shadowed = false;
	}
	PaintBubble(p, simple);
}

void PaintBubblePiece(
		QPainter &p,
		const SimpleBubble &args,
		QRect geometry,
		bool selected,
		bool fromTop,
		bool tillBottom) {
	auto simple = args;
	simple.selected = selected;
	PaintBubblePiece(p, simple, geometry, fromTop, tillBottom);
}

template <
	typename FillBg, // fillBg(QRect rect)
	typename FillSh, // fillSh(QRect rect)
	typename FillCorner, // fillCorner(int x, int y, int index, Corner size)
	typename PaintTail> // paintTail(QPoint bottomPosition) -> tailWidth
void PaintBubbleGeneric(
		const SimpleBubble &args,
		FillBg &&fillBg,
		FillSh &&fillSh,
		FillCorner &&fillCorner,
		PaintTail &&paintTail) {
	using namespace Images;

	const auto topLeft = args.rounding.topLeft;
	const auto topRight = args.rounding.topRight;
	const auto bottomWithTailLeft = args.rounding.bottomLeft;
	const auto bottomWithTailRight = args.rounding.bottomRight;
	if (topLeft == Corner::None
		&& topRight == Corner::None
		&& bottomWithTailLeft == Corner::None
		&& bottomWithTailRight == Corner::None) {
		fillBg(args.geometry);
		return;
	}
	const auto bottomLeft = (bottomWithTailLeft == Corner::Tail)
		? Corner::None
		: bottomWithTailLeft;
	const auto bottomRight = (bottomWithTailRight == Corner::Tail)
		? Corner::None
		: bottomWithTailRight;
	const auto rect = args.geometry;
	const auto small = BubbleRadiusSmall();
	const auto large = BubbleRadiusLarge();
	const auto cornerSize = [&](Corner corner) {
		return (corner == Corner::Large)
			? large
			: (corner == Corner::Small)
			? small
			: 0;
	};
	const auto verticalSkip = [&](Corner left, Corner right) {
		return std::max(cornerSize(left), cornerSize(right));
	};
	const auto top = verticalSkip(topLeft, topRight);
	const auto bottom = verticalSkip(bottomLeft, bottomRight);
	if (top) {
		const auto left = cornerSize(topLeft);
		const auto right = cornerSize(topRight);
		if (left) {
			fillCorner(rect.left(), rect.top(), kTopLeft, topLeft);
			if (const auto add = top - left) {
				fillBg({ rect.left(), rect.top() + left, left, add });
			}
		}
		if (const auto fill = rect.width() - left - right; fill > 0) {
			fillBg({ rect.left() + left, rect.top(), fill, top });
		}
		if (right) {
			fillCorner(
				rect.left() + rect.width() - right,
				rect.top(),
				kTopRight,
				topRight);
			if (const auto add = top - right) {
				fillBg({
					rect.left() + rect.width() - right,
					rect.top() + right,
					right,
					add,
				});
			}
		}
	}
	if (const auto fill = rect.height() - top - bottom; fill > 0) {
		fillBg({ rect.left(), rect.top() + top, rect.width(), fill });
	}
	if (bottom) {
		const auto left = cornerSize(bottomLeft);
		const auto right = cornerSize(bottomRight);
		if (left) {
			fillCorner(
				rect.left(),
				rect.top() + rect.height() - left,
				kBottomLeft,
				bottomLeft);
			if (const auto add = bottom - left) {
				fillBg({
					rect.left(),
					rect.top() + rect.height() - bottom,
					left,
					add,
				});
			}
		}
		if (const auto fill = rect.width() - left - right; fill > 0) {
			fillBg({
				rect.left() + left,
				rect.top() + rect.height() - bottom,
				fill,
				bottom,
			});
		}
		if (right) {
			fillCorner(
				rect.left() + rect.width() - right,
				rect.top() + rect.height() - right,
				kBottomRight,
				bottomRight);
			if (const auto add = bottom - right) {
				fillBg({
					rect.left() + rect.width() - right,
					rect.top() + rect.height() - bottom,
					right,
					add,
				});
			}
		}
	}
	const auto leftTail = (bottomWithTailLeft == Corner::Tail)
		? paintTail({ rect.x(), rect.y() + rect.height() })
		: 0;
	const auto rightTail = (bottomWithTailRight == Corner::Tail)
		? paintTail({ rect.x() + rect.width(), rect.y() + rect.height() })
		: 0;
	if (!args.shadowed) {
		return;
	}
	const auto shLeft = rect.x() + cornerSize(bottomLeft) - leftTail;
	const auto shWidth = rect.x()
		+ rect.width()
		- cornerSize(bottomRight)
		+ rightTail
		- shLeft;
	if (shWidth > 0) {
		fillSh({ shLeft, rect.y() + rect.height(), shWidth, st::msgShadow });
	}
}

// iOS-like continuous corners: a circular arc in the middle and two
// cubic transitions on each side, so curvature grows from zero instead
// of jumping at the seam between a straight side and the arc.
constexpr auto kBubbleCornerSmoothing = 0.6;

struct SquircleCorner {
	float64 a = 0.;
	float64 b = 0.;
	float64 c = 0.;
	float64 d = 0.;
	float64 p = 0.;
	float64 arc = 0.;
	float64 radius = 0.;
	float64 arcMeasure = 0.;
};

[[nodiscard]] float64 DegToRad(float64 degrees) {
	return degrees * M_PI / 180.;
}

[[nodiscard]] SquircleCorner ComputeSquircleCorner(
		float64 radius,
		float64 smoothing,
		float64 budget) {
	auto result = SquircleCorner();
	radius = std::min(radius, budget);
	if (radius <= 0.) {
		return result;
	}
	auto p = std::min((1. + smoothing) * radius, budget);
	const auto arcMeasure = 90. * (1. - smoothing);
	const auto arc = std::sin(DegToRad(arcMeasure / 2.))
		* radius
		* std::sqrt(2.);
	const auto alpha = (90. - arcMeasure) / 2.;
	const auto p3ToP4 = radius * std::tan(DegToRad(alpha / 2.));
	const auto beta = 45. * smoothing;
	const auto c = p3ToP4 * std::cos(DegToRad(beta));
	const auto d = c * std::tan(DegToRad(beta));
	auto b = (p - arc - c - d) / 3.;
	auto a = 2. * b;
	if ((1. + smoothing) * radius > budget) {
		const auto maxDistance = budget - d - arc - c;
		if (maxDistance <= 0.) {
			// No room for smoothing at all, plain circular corner.
			return ComputeSquircleCorner(radius, 0., budget);
		}
		b = std::min(b, maxDistance - maxDistance / 6.);
		a = maxDistance - b;
	}
	result.a = a;
	result.b = b;
	result.c = c;
	result.d = d;
	result.p = p;
	result.arc = arc;
	result.radius = radius;
	result.arcMeasure = arcMeasure;
	return result;
}

// Corner described in a local frame: x goes along the incoming side
// (e1) towards the tip, y goes along the outgoing side (e2).
void AddSquircleCorner(
		QPainterPath &path,
		QPointF tip,
		QPointF e1,
		QPointF e2,
		const SquircleCorner &k,
		bool first) {
	if (k.p <= 0.) {
		if (first) {
			path.moveTo(tip);
		} else {
			path.lineTo(tip);
		}
		return;
	}
	const auto map = [&](QPointF local) {
		return tip + e1 * (local.x() - k.p) + e2 * local.y();
	};
	const auto start = map({ 0., 0. });
	if (first) {
		path.moveTo(start);
	} else {
		path.lineTo(start);
	}
	const auto from = QPointF(k.a + k.b + k.c, k.d);
	path.cubicTo(map({ k.a, 0. }), map({ k.a + k.b, 0. }), map(from));

	const auto till = from + QPointF(k.arc, k.arc);
	const auto center = QPointF(k.p - k.radius, k.radius);
	const auto handle = 4. / 3. * std::tan(DegToRad(k.arcMeasure) / 4.);
	const auto rFrom = from - center;
	const auto rTill = till - center;
	const auto tFrom = QPointF(-rFrom.y(), rFrom.x()) * handle;
	const auto tTill = QPointF(-rTill.y(), rTill.x()) * handle;
	path.cubicTo(map(from + tFrom), map(till - tTill), map(till));

	path.cubicTo(
		map(till + QPointF(k.d, k.c)),
		map(till + QPointF(k.d, k.b + k.c)),
		map(till + QPointF(k.d, k.a + k.b + k.c)));
}

[[nodiscard]] QPainterPath BubblePath(
		const QRect &geometry,
		const BubbleRounding &rounding) {
	const auto rect = QRectF(geometry);
	const auto budget = std::min(rect.width(), rect.height()) / 2.;
	const auto corner = [&](Corner value) {
		return ComputeSquircleCorner(
			BubbleCornerRadius(value),
			kBubbleCornerSmoothing,
			budget);
	};
	auto result = QPainterPath();
	AddSquircleCorner(
		result,
		rect.topRight(),
		{ 1., 0. },
		{ 0., 1. },
		corner(rounding.topRight),
		true);
	AddSquircleCorner(
		result,
		rect.bottomRight(),
		{ 0., 1. },
		{ -1., 0. },
		corner(rounding.bottomRight),
		false);
	AddSquircleCorner(
		result,
		rect.bottomLeft(),
		{ -1., 0. },
		{ 0., -1. },
		corner(rounding.bottomLeft),
		false);
	AddSquircleCorner(
		result,
		rect.topLeft(),
		{ 0., -1. },
		{ 1., 0. },
		corner(rounding.topLeft),
		false);
	result.closeSubpath();
	return result;
}

// Under-tail part of the 1px bottom shadow, the path shadow ends at
// the square tail corner.
[[nodiscard]] QRect TailShadowRect(
		const SimpleBubble &args,
		int tailWidth) {
	const auto &rect = args.geometry;
	if (args.rounding.bottomLeft == Corner::Tail) {
		return { rect.x() - tailWidth, rect.y() + rect.height(), tailWidth, st::msgShadow };
	} else if (args.rounding.bottomRight == Corner::Tail) {
		return { rect.x() + rect.width(), rect.y() + rect.height(), tailWidth, st::msgShadow };
	}
	return {};
}

void PaintPatternBubble(QPainter &p, const SimpleBubble &args) {
	const auto wasOpacity = p.opacity();
	const auto opacity = PatternBubbleOpacity(args, wasOpacity);
	const auto shadowOpacity = opacity * args.st->msgOutShadow()->c.alphaF();
	const auto pattern = args.pattern;
	const auto &tail = (args.rounding.bottomRight == Corner::Tail)
		? pattern->tailRight
		: pattern->tailLeft;
	const auto tailShift = (args.rounding.bottomRight == Corner::Tail
		? QPoint(0, tail.height())
		: QPoint(tail.width(), tail.height())) / int(tail.devicePixelRatio());
	const auto fillBg = [&](const QRect &rect) {
		const auto fill = rect.intersected(args.patternViewport);
		if (!fill.isEmpty()) {
			PaintPatternBubblePart(
				p,
				args.patternViewport,
				pattern->pixmap,
				fill);
		}
	};
	const auto fillSh = [&](const QRect &rect) {
		p.setOpacity(shadowOpacity);
		fillBg(rect);
		p.setOpacity(opacity);
	};
	const auto fillPattern = [&](
			int x,
			int y,
			const QImage &mask,
			QImage &cache) {
		PaintPatternBubblePart(
			p,
			args.patternViewport,
			pattern->pixmap,
			QRect(QPoint(x, y), mask.size() / int(mask.devicePixelRatio())),
			mask,
			cache);
	};
	const auto paintTail = [&](QPoint bottomPosition) {
		const auto position = bottomPosition - tailShift;
		fillPattern(position.x(), position.y(), tail, pattern->tailCache);
		return tail.width() / int(tail.devicePixelRatio());
	};

	const auto &rect = args.geometry;
	const auto shadow = args.shadowed ? st::msgShadow : 0;
	const auto ratio = style::DevicePixelRatio();
	auto mask = QImage(
		QSize(rect.width(), rect.height() + shadow) * ratio,
		QImage::Format_ARGB32_Premultiplied);
	mask.setDevicePixelRatio(ratio);
	mask.fill(Qt::transparent);
	{
		const auto path = BubblePath(rect.translated(-rect.topLeft()), args.rounding);
		auto q = QPainter(&mask);
		PainterHighQualityEnabler hq(q);
		q.setPen(Qt::NoPen);
		q.setCompositionMode(QPainter::CompositionMode_Source);
		if (shadow) {
			auto color = QColor(255, 255, 255);
			color.setAlphaF(args.st->msgOutShadow()->c.alphaF());
			q.fillPath(path.translated(0, shadow), color);
		}
		q.fillPath(path, QColor(255, 255, 255));
	}
	auto cache = QImage();
	p.setOpacity(opacity);
	PaintPatternBubblePart(
		p,
		args.patternViewport,
		pattern->pixmap,
		QRect(rect.topLeft(), QSize(rect.width(), rect.height() + shadow)),
		mask,
		cache);
	if (args.rounding.bottomLeft == Corner::Tail
		|| args.rounding.bottomRight == Corner::Tail) {
		const auto width = paintTail(rect.topLeft() + QPoint(
			(args.rounding.bottomLeft == Corner::Tail) ? 0 : rect.width(),
			rect.height()));
		if (shadow) {
			fillSh(TailShadowRect(args, width));
		}
	}
	p.setOpacity(wasOpacity);
}

void PaintSolidBubble(QPainter &p, const SimpleBubble &args) {
	const auto &st = args.st->messageStyle(args.outbg, args.selected);
	const auto &bg = st.msgBg;
	const auto sh = (args.rounding.bottomRight == Corner::None)
		? nullptr
		: &st.msgShadow;
	const auto &tail = (args.rounding.bottomRight == Corner::Tail)
		? st.tailRight
		: st.tailLeft;
	const auto tailShift = (args.rounding.bottomRight == Corner::Tail)
		? QPoint(0, tail.height())
		: QPoint(tail.width(), tail.height());

	const auto path = BubblePath(args.geometry, args.rounding);
	{
		PainterHighQualityEnabler hq(p);
		if (args.shadowed && sh) {
			const auto shadow = path.translated(0, st::msgShadow);
			p.fillPath(
				(bg->c.alpha() < 255) ? shadow.subtracted(path) : shadow,
				(*sh)->b);
		}
		p.fillPath(path, bg->b);
	}
	const auto &rect = args.geometry;
	if (args.rounding.bottomLeft == Corner::Tail
		|| args.rounding.bottomRight == Corner::Tail) {
		const auto bottom = rect.topLeft() + QPoint(
			(args.rounding.bottomLeft == Corner::Tail) ? 0 : rect.width(),
			rect.height());
		tail.paint(p, bottom - tailShift, args.outerWidth);
		if (args.shadowed && sh) {
			p.fillRect(TailShadowRect(args, tail.width()), *sh);
		}
	}
}

} // namespace

std::unique_ptr<BubblePattern> PrepareBubblePattern(
		not_null<const style::palette*> st) {
	auto result = std::make_unique<Ui::BubblePattern>();
	result->cornersSmall = Images::CornersMask(BubbleRadiusSmall());
	result->cornersLarge = Images::CornersMask(BubbleRadiusLarge());
	const auto addShadow = [&](QImage &bottomCorner) {
		auto result = QImage(
			bottomCorner.width(),
			(bottomCorner.height()
				+ st::msgShadow * int(bottomCorner.devicePixelRatio())),
			QImage::Format_ARGB32_Premultiplied);
		result.fill(Qt::transparent);
		result.setDevicePixelRatio(bottomCorner.devicePixelRatio());
		auto p = QPainter(&result);
		p.setOpacity(st->msgInShadow()->c.alphaF());
		p.drawImage(0, st::msgShadow, bottomCorner);
		p.setOpacity(1.);
		p.drawImage(0, 0, bottomCorner);
		p.end();

		bottomCorner = std::move(result);
	};
	addShadow(result->cornersSmall[2]);
	addShadow(result->cornersSmall[3]);
	result->cornerTopSmallCache = QImage(
		result->cornersSmall[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerTopLargeCache = QImage(
		result->cornersLarge[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomSmallCache = QImage(
		result->cornersSmall[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomLargeCache = QImage(
		result->cornersLarge[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	return result;
}

void FinishBubblePatternOnMain(not_null<BubblePattern*> pattern) {
	pattern->tailLeft = st::historyBubbleTailOutLeft.instance(Qt::white);
	pattern->tailRight = st::historyBubbleTailOutRight.instance(Qt::white);
	pattern->tailCache = QImage(
		pattern->tailLeft.size(),
		QImage::Format_ARGB32_Premultiplied);
}

QPainterPath SquirclePath(
		QRectF rect,
		float64 radius,
		float64 smoothing) {
	const auto budget = std::min(rect.width(), rect.height()) / 2.;
	const auto corner = ComputeSquircleCorner(radius, smoothing, budget);
	auto result = QPainterPath();
	AddSquircleCorner(result, rect.topRight(), { 1., 0. }, { 0., 1. }, corner, true);
	AddSquircleCorner(result, rect.bottomRight(), { 0., 1. }, { -1., 0. }, corner, false);
	AddSquircleCorner(result, rect.bottomLeft(), { -1., 0. }, { 0., -1. }, corner, false);
	AddSquircleCorner(result, rect.topLeft(), { 0., -1. }, { 1., 0. }, corner, false);
	result.closeSubpath();
	return result;
}

void PaintBubble(QPainter &p, const SimpleBubble &args) {
	if (UsePatternBubble(args)) {
		PaintPatternBubble(p, args);
	} else {
		PaintSolidBubble(p, args);
	}
}

void PaintBubble(QPainter &p, const ComplexBubble &args) {
	if (args.selection.empty()) {
		PaintBubble(p, args.simple);
		return;
	}
	const auto rect = args.simple.geometry;
	const auto left = rect.x();
	const auto width = rect.width();
	const auto top = rect.y();
	const auto bottom = top + rect.height();
	auto from = top;
	for (const auto &selected : args.selection) {
		if (selected.top > from) {
			PaintBubblePiece(
				p,
				args.simple,
				QRect(left, from, width, selected.top - from),
				false,
				(from <= top),
				false);
		}
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, selected.top, width, selected.height),
			true,
			(selected.top <= top),
			(selected.top + selected.height >= bottom));
		from = selected.top + selected.height;
	}
	if (from < bottom) {
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, from, width, bottom - from),
			false,
			false,
			true);
	}
}

void PaintBubble(QPainter &p, const BubbleWithGaps &args) {
	if (args.gaps.empty()) {
		PaintBubble(p, args.simple);
		return;
	}
	const auto rect = args.simple.geometry;
	const auto left = rect.x();
	const auto width = rect.width();
	const auto top = rect.y();
	const auto bottom = top + rect.height();
	const auto pattern = UsePatternBubble(args.simple);
	const auto paintStrip = [&](
			const QImage &mask,
			QImage &patternCache,
			QImage &solidCache,
			QColor &solidColor,
			int y) {
		const auto target = QRect(
			left,
			y,
			width,
			mask.height() / int(mask.devicePixelRatio()));
		if (pattern) {
			const auto wasOpacity = p.opacity();
			p.setOpacity(PatternBubbleOpacity(args.simple, wasOpacity));
			PaintPatternBubblePart(
				p,
				args.simple.patternViewport,
				args.simple.pattern->pixmap,
				target,
				mask,
				patternCache);
			p.setOpacity(wasOpacity);
		} else {
			const auto &st = args.simple.st->messageStyle(
				args.simple.outbg,
				args.simple.selected);
			const auto color = st.msgBg->c;
			if (solidCache.size() != mask.size() || solidColor != color) {
				if (solidCache.size() != mask.size()) {
					solidCache = QImage(
						mask.size(),
						QImage::Format_ARGB32_Premultiplied);
					solidCache.setDevicePixelRatio(mask.devicePixelRatio());
				}
				style::colorizeImage(mask, color, &solidCache);
				solidColor = color;
			}
			p.drawImage(target, solidCache);
		}
	};
	const auto torn = args.torn;
	const auto stripHeight = torn->maskTop.height()
		/ int(torn->maskTop.devicePixelRatio());
	auto from = top;
	for (const auto &gap : args.gaps) {
		const auto gapTop = gap.top;
		const auto gapBottom = gap.top + gap.height;
		if (gapTop > from) {
			PaintBubblePiece(
				p,
				args.simple,
				QRect(left, from, width, gapTop - from),
				(from <= top),
				false);
			paintStrip(
				torn->maskBottom,
				torn->patternCacheBottom,
				torn->solidCacheBottom,
				torn->solidColorBottom,
				gapTop);
		}
		if (gapBottom < bottom) {
			paintStrip(
				torn->maskTop,
				torn->patternCacheTop,
				torn->solidCacheTop,
				torn->solidColorTop,
				gapBottom - stripHeight);
		}
		from = gapBottom;
	}
	if (from < bottom) {
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, from, width, bottom - from),
			false,
			true);
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target) {
	const auto factor = pixmap.devicePixelRatio();
	if (viewport.size() * factor == pixmap.size()) {
		const auto fill = target.intersected(viewport);
		if (fill.isEmpty()) {
			return;
		}
		p.drawPixmap(fill, pixmap, QRect(
			(fill.topLeft() - viewport.topLeft()) * factor,
			fill.size() * factor));
	} else {
		const auto to = viewport;
		const auto from = QRect(QPoint(), pixmap.size());
		const auto deviceRect = QRect(
			QPoint(),
			QSize(p.device()->width(), p.device()->height()));
		const auto clip = (target != deviceRect);
		if (clip) {
			p.setClipRect(target);
		}
		p.drawPixmap(to, pixmap, from);
		if (clip) {
			p.setClipping(false);
		}
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		const QImage &mask,
		QImage &cache) {
	Expects(mask.bytesPerLine() == mask.width() * 4);
	Expects(mask.format() == QImage::Format_ARGB32_Premultiplied);

	if (cache.size() != mask.size()) {
		cache = QImage(
			mask.size(),
			QImage::Format_ARGB32_Premultiplied);
	}
	cache.setDevicePixelRatio(mask.devicePixelRatio());
	Assert(cache.bytesPerLine() == cache.width() * 4);
	memcpy(cache.bits(), mask.constBits(), mask.sizeInBytes());

	auto q = QPainter(&cache);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-target.topLeft()),
		pixmap,
		QRect(QPoint(), cache.size() / int(cache.devicePixelRatio())));
	q.end();

	p.drawImage(target, cache);
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		Fn<void(QPainter&)> paintContent,
		QImage &cache) {
	Expects(paintContent != nullptr);

	const auto targetOrigin = target.topLeft();
	const auto targetSize = target.size();
	if (cache.size() != targetSize * style::DevicePixelRatio()) {
		cache = QImage(
			target.size() * style::DevicePixelRatio(),
			QImage::Format_ARGB32_Premultiplied);
		cache.setDevicePixelRatio(style::DevicePixelRatio());
	}
	cache.fill(Qt::transparent);
	auto q = QPainter(&cache);
	q.translate(-targetOrigin);
	paintContent(q);
	q.translate(targetOrigin);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-targetOrigin),
		pixmap,
		QRect(QPoint(), targetSize));
	q.end();

	p.drawImage(target, cache);
}

} // namespace Ui
