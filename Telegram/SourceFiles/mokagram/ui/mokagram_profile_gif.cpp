// This file is part of mokaGram. See LICENSE and MOKAGRAM_NOTICE.md.
#include "mokagram/ui/mokagram_profile_gif.h"

#include "base/call_delayed.h"
#include "base/weak_qptr.h"
#include <crl/crl.h>

#include <QtCore/QMimeData>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include <array>
#include <cmath>
#include <vector>
#ifdef Q_OS_MAC
#include <QtGui/QUtiMimeConverter>
#endif
#include <QtWidgets/QWidget>

// msf_gif 2.4, e280bc2189ed32b743c7f634cd21c0eb5def7046, MIT.
#define MSF_GIF_IMPL
#include "../../../ThirdParty/msf_gif/msf_gif.h"

namespace Mokagram {
namespace {

constexpr auto kFrameDelay = crl::time(40);
constexpr auto kFrameCount = 120;
constexpr auto kLoopBlendFrames = 12;
constexpr auto kPaletteColors = 255;
constexpr auto kHistogramSize = 1 << 18;
constexpr auto kGifMime = QStringView(u"image/gif");

size_t AppendGifData(const void *data, size_t size, size_t count, void *out) {
	if (!data) {
		return 0;
	}
	static_cast<QByteArray*>(out)->append(
		static_cast<const char*>(data), size * count);
	return count;
}

using Rgb = std::array<int, 3>;

[[nodiscard]] int ColorBucket(const uchar *rgb) {
	return ((rgb[0] >> 2) << 12) | ((rgb[1] >> 2) << 6) | (rgb[2] >> 2);
}

struct ColorSample {
	int count = 0;
	Rgb sums = {};
	Rgb color = {};
};

struct ColorBox {
	int from = 0;
	int to = 0;
	int count = 0;
	int axis = 0;
	int range = 0;
	Rgb sums = {};
};

[[nodiscard]] ColorBox MeasureBox(
		const std::vector<ColorSample> &samples,
		int from,
		int to) {
	auto box = ColorBox{ .from = from, .to = to };
	auto low = Rgb{ 255, 255, 255 };
	auto high = Rgb{};
	for (auto i = from; i != to; ++i) {
		const auto &sample = samples[i];
		box.count += sample.count;
		for (auto c = 0; c != 3; ++c) {
			low[c] = std::min(low[c], sample.color[c]);
			high[c] = std::max(high[c], sample.color[c]);
			box.sums[c] += sample.sums[c];
		}
	}
	for (auto c = 0; c != 3; ++c) {
		if (high[c] - low[c] > box.range) {
			box.range = high[c] - low[c];
			box.axis = c;
		}
	}
	return box;
}

class ProfilePalette {
public:
	void build(const std::vector<QImage> &intro, const QImage &frame) {
		auto histogram = std::vector<ColorSample>(kHistogramSize);
		const auto sample = [&](const QImage &image) {
			for (auto y = 0; y < image.height(); y += 2) {
				const auto row = image.constScanLine(y);
				for (auto x = 0; x < image.width(); x += 2) {
					const auto rgb = row + 4 * x;
					auto &entry = histogram[ColorBucket(rgb)];
					++entry.count;
					for (auto c = 0; c != 3; ++c) entry.sums[c] += rgb[c];
				}
			}
		};
		for (const auto &image : intro) sample(image);
		sample(frame);
		auto samples = std::vector<ColorSample>();
		for (auto &entry : histogram) {
			if (!entry.count) continue;
			for (auto c = 0; c != 3; ++c) entry.color[c] = entry.sums[c] / entry.count;
			samples.push_back(entry);
		}

		// Split occupied colors at their weighted median, preserving detail
		// in the portrait and the much smaller colored decorations.
		auto boxes = std::vector<ColorBox>{ MeasureBox(samples, 0, int(samples.size())) };
		const auto priority = [](const ColorBox &box) {
			return double(box.range) * std::sqrt(double(box.count));
		};
		while (boxes.size() < kPaletteColors) {
			const auto largest = std::max_element(boxes.begin(), boxes.end(),
				[&](const ColorBox &a, const ColorBox &b) { return priority(a) < priority(b); });
			if (largest->to - largest->from < 2 || !largest->range) break;
			const auto axis = largest->axis;
			std::sort(samples.begin() + largest->from, samples.begin() + largest->to,
				[=](const ColorSample &a, const ColorSample &b) { return a.color[axis] < b.color[axis]; });
			auto split = largest->from + 1;
			auto count = samples[largest->from].count;
			while (split < largest->to - 1 && count < largest->count / 2) {
				count += samples[split++].count;
			}
			const auto right = MeasureBox(samples, split, largest->to);
			*largest = MeasureBox(samples, largest->from, split);
			boxes.push_back(right);
		}
		_size = int(boxes.size());
		for (auto i = 0; i != kPaletteColors; ++i) {
			const auto &box = boxes[std::min(i, _size - 1)];
			for (auto c = 0; c != 3; ++c) {
				_colors[3 * i + c] = uchar((box.sums[c] + box.count / 2) / box.count);
			}
		}
		_indices.assign(kHistogramSize, -1);
	}

	[[nodiscard]] bool append(MsfGifState &encoder, const QImage &frame) {
		auto output = encoder.currentFrame.pixels;
		for (auto y = 0; y != frame.height(); ++y) {
			const auto row = frame.constScanLine(y);
			for (auto x = 0; x != frame.width(); ++x) {
				const auto bucket = ColorBucket(row + 4 * x);
				auto &index = _indices[bucket];
				if (index < 0) {
					index = nearest({
						((bucket >> 12) & 63) * 4 + 2,
						((bucket >> 6) & 63) * 4 + 2,
						(bucket & 63) * 4 + 2,
					});
				}
				*output++ = index;
			}
		}
		return msf_gif_indexed_frame_to_file(&encoder, _colors.data(), kFrameDelay / 10);
	}

private:
	[[nodiscard]] int nearest(Rgb rgb) const {
		auto best = 0;
		auto distance = 1 << 20;
		for (auto i = 0; i != _size; ++i) {
			const auto r = rgb[0] - _colors[3 * i];
			const auto g = rgb[1] - _colors[3 * i + 1];
			const auto b = rgb[2] - _colors[3 * i + 2];
			const auto delta = 2 * r * r + 4 * g * g + b * b;
			if (delta < distance) {
				best = i;
				distance = delta;
			}
		}
		return best;
	}

	int _size = 0;
	std::array<uchar, kPaletteColors * 3> _colors = {};
	std::vector<int> _indices;
};

struct Recording {
	~Recording() {
		msf_gif_free(msf_gif_end(&encoder));
	}

	base::weak_qptr<QWidget> guard;
	Fn<bool(QImage&)> render;
	Fn<void(QByteArray)> done;
	int frameCount = 0;
	int frameIndex = 0;
	int blendFrames = 0;
	std::vector<QImage> intro;
	ProfilePalette palette;
	MsfGifState encoder = {};
	QImage frame;
	QByteArray bytes;
};

void CaptureFrame(std::shared_ptr<Recording> recording) {
	const auto started = crl::now();
	if (!recording->render(recording->frame)) {
		recording->done({});
		return;
	}
	// Retain only the short overlap used to join the end to the beginning.
	crl::async([recording = std::move(recording), started] {
		auto &encoder = recording->encoder;
		auto &frame = recording->frame;
		const auto index = recording->frameIndex++;
		const auto first = (index == recording->blendFrames);
		const auto last = (recording->frameIndex == recording->frameCount);
		const auto overlap = index - (recording->frameCount - recording->blendFrames);
		auto ok = true;
		if (index < recording->blendFrames) {
			recording->intro.push_back(frame);
		} else {
			if (first) {
				recording->palette.build(recording->intro, frame);
				ok = msf_gif_begin_to_file(&encoder, frame.width(), frame.height(),
					AppendGifData, &recording->bytes);
			}
			if (overlap >= 0) {
				const auto t = (overlap + 1.) / recording->blendFrames;
				auto painter = QPainter(&frame);
				painter.setOpacity(t * t * (3. - 2. * t));
				painter.drawImage(QPoint(), recording->intro[overlap]);
			}
			ok = ok && recording->palette.append(encoder, frame)
				&& (!last || msf_gif_end_to_file(&encoder));
		}
		const auto guard = recording->guard;
		crl::on_main(guard, [recording = std::move(recording), started, ok, last] {
			if (!ok || last) {
				recording->done(ok ? std::move(recording->bytes) : QByteArray());
			} else {
				const auto delay = std::max(crl::time(1), started + kFrameDelay - crl::now());
				base::call_delayed(delay, recording->guard, [recording] {
					CaptureFrame(recording);
				});
			}
		});
	});
}

#ifdef Q_OS_MAC
class GifMimeConverter final : public QUtiMimeConverter {
public:
	QString utiForMime(const QString &mime) const override {
		return (mime == kGifMime) ? u"com.compuserve.gif"_q : QString();
	}
	QString mimeForUti(const QString &uti) const override {
		return (uti == u"com.compuserve.gif"_q) ? kGifMime.toString() : QString();
	}
	QList<QByteArray> convertFromMime(
			const QString &, const QVariant &data, const QString &) const override {
		return { data.toByteArray() };
	}
	QVariant convertToMime(
			const QString &, const QList<QByteArray> &data, const QString &) const override {
		return data.value(0);
	}
};
#endif // Q_OS_MAC

} // namespace

void RecordProfileGif(
		not_null<QWidget*> guard,
		Fn<bool(QImage&)> render,
		bool animated,
		Fn<void(QByteArray)> done) {
	auto recording = std::make_shared<Recording>();
	recording->guard = base::make_weak(guard.get());
	recording->render = std::move(render);
	recording->done = std::move(done);
	recording->blendFrames = animated ? kLoopBlendFrames : 0;
	recording->frameCount = animated ? kFrameCount + kLoopBlendFrames : 1;
	CaptureFrame(std::move(recording));
}

bool CopyProfileGif(const QByteArray &gif) {
	// File paste preserves the animation in clients that only accept attachments.
	static const auto directory = QTemporaryDir();
	if (!directory.isValid()) {
		return false;
	}
	auto file = QTemporaryFile(directory.filePath(u"mokagram_profile_XXXXXX.gif"_q));
	if (!file.open() || file.write(gif) != gif.size() || !file.flush()) {
		return false;
	}
	file.close();
	file.setAutoRemove(false);
	const auto mime = new QMimeData();
	mime->setData(kGifMime.toString(), gif);
	mime->setUrls({ QUrl::fromLocalFile(file.fileName()) });
#ifdef Q_OS_MAC
	// The Cocoa integration owns and destroys registered converters.
	[[maybe_unused]] static const auto converter = new GifMimeConverter();
#elif defined Q_OS_WIN
	mime->setData(u"application/x-qt-windows-mime;value=\"GIF\""_q, gif);
#endif // Q_OS_MAC || Q_OS_WIN
	QGuiApplication::clipboard()->setMimeData(mime);
	return true;
}

} // namespace Mokagram
