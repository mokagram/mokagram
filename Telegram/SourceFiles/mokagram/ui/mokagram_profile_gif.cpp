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
#ifdef Q_OS_MAC
#include <QtGui/QUtiMimeConverter>
#endif
#include <QtWidgets/QWidget>

// msf_gif 2.4, e280bc2189ed32b743c7f634cd21c0eb5def7046, MIT.
#define MSF_GIF_IMPL
#include "../../../ThirdParty/msf_gif/msf_gif.h"

namespace Mokagram {
namespace {

constexpr auto kFrameDelay = crl::time(60);
constexpr auto kFrameCount = 40;
constexpr auto kGifMime = QStringView(u"image/gif");

size_t AppendGifData(const void *data, size_t size, size_t count, void *out) {
	if (!data) {
		return 0;
	}
	static_cast<QByteArray*>(out)->append(
		static_cast<const char*>(data), size * count);
	return count;
}

struct Recording {
	~Recording() {
		msf_gif_free(msf_gif_end(&encoder));
	}

	base::weak_qptr<QWidget> guard;
	Fn<bool(QImage&)> render;
	Fn<void(QByteArray)> done;
	int frameCount = 0;
	int frameIndex = 0;
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
	// Keep one reusable framebuffer in flight; never queue raw frames.
	crl::async([recording = std::move(recording), started] {
		auto &encoder = recording->encoder;
		auto &frame = recording->frame;
		const auto first = (recording->frameIndex == 0);
		const auto last = (++recording->frameIndex == recording->frameCount);
		const auto ok = (!first || msf_gif_begin_to_file(
			&encoder,
			frame.width(),
			frame.height(),
			AppendGifData,
			&recording->bytes))
			&& msf_gif_frame_to_file(
				&encoder,
				frame.bits(),
				kFrameDelay / 10,
				16,
				frame.bytesPerLine())
			&& (!last || msf_gif_end_to_file(&encoder));
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
	recording->frameCount = animated ? kFrameCount : 1;
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
