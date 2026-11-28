// This file is part of mokaGram, https://github.com/mokagram/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#include "mokagram/music/mokagram_lyrics.h"

#include "base/timer.h"
#include "core/application.h"
#include "data/data_document.h"
#include "core/file_location.h"
#include "data/data_document_media.h"
#include "data/data_file_origin.h"
#include "main/main_account.h"
#include "main/main_session.h"
#include "media/player/media_player_instance.h"
#include "media/streaming/media_streaming_loader.h"
#include "mokagram/asr/mokagram_asr.h"
#include "mokagram/music/mokagram_soundcloud.h"

#include <map>

namespace Mokagram::Music {
namespace {

constexpr auto kSong = AudioMsgId::Type::Song;
constexpr auto kMaxAudioBytes = int64(64 * 1024 * 1024);
constexpr auto kPartTimeout = crl::time(30000);

[[nodiscard]] bool SameAudio(const AudioMsgId &a, const AudioMsgId &b) {
	return a.audio() == b.audio() && a.contextId() == b.contextId();
}

std::map<QString, Lyrics> &GeneratedCache() {
	static auto cache = std::map<QString, Lyrics>();
	return cache;
}

class Recognition;
std::unique_ptr<Recognition> &Active();

class Recognition final : public base::has_weak_ptr {
public:
	Recognition(QString key, const AudioMsgId &audio, Fn<void(Lyrics)> done)
	: _key(std::move(key))
	, _audio(audio)
	, _timeout([this] { finish({}); }) {
		join(std::move(done));
	}

	[[nodiscard]] bool matches(const AudioMsgId &audio) const {
		return SameAudio(_audio, audio);
	}

	void join(Fn<void(Lyrics)> done) {
		_waiting.push_back(std::move(done));
	}

	void start() {
		const auto document = _audio.audio();
		document->session().account().sessionChanges(
		) | rpl::on_next([this] { finish({}); }, _lifetime);
		Media::Player::instance()->trackChanged(
		) | rpl::on_next([this](AudioMsgId::Type type) {
			if (type == kSong && !matches(Media::Player::instance()->current(kSong))) {
				finish({});
			}
		}, _lifetime);

		if (const auto media = document->activeMediaView()) {
			auto bytes = media->bytes();
			if (!bytes.isEmpty() && !bytes.startsWith("partial:")) {
				transcribe(std::move(bytes), {});
				return;
			}
		}
		const auto &location = document->location(true);
		if (!location.isEmpty() && !location.inMediaCache()) {
			transcribe({}, location);
			return;
		}
		_loader = document->createStreamingLoader(_audio.contextId(), true);
		if (!_loader || _loader->size() <= 0 || _loader->size() > kMaxAudioBytes) {
			finish({});
			return;
		}
		_bytes.reserve(_loader->size());
		_loader->parts() | rpl::on_next([this](Media::Streaming::LoadedPart part) {
			const auto size = _loader->size();
			const auto offset = int64(_bytes.size());
			const auto expected = std::min(Media::Streaming::Loader::kPartSize, size - offset);
			if (part.offset != offset || part.bytes.size() != expected) {
				crl::on_main(this, [this] { finish({}); });
				return;
			}
			_bytes.append(part.bytes);
			if (_bytes.size() < size) {
				loadNext();
			} else {
				_timeout.cancel();
				// Отпускаем загрузчик после выхода из его обработчика части.
				crl::on_main(this, [this] {
					_loader.reset();
					transcribe(std::move(_bytes), {});
				});
			}
		}, _lifetime);
		loadNext();
	}

	void finish(Lyrics result) {
		if (result.found) {
			auto &cache = GeneratedCache();
			if (cache.size() >= 64) {
				cache.erase(cache.begin());
			}
			cache[_key] = result;
		}
		auto waiting = std::move(_waiting);
		Active().reset();
		crl::on_main([waiting = std::move(waiting), result = std::move(result)]() mutable {
			for (const auto &done : waiting) {
				done(result);
			}
		});
	}

private:
	void loadNext() {
		_timeout.callOnce(kPartTimeout);
		_loader->load(_bytes.size());
	}

	void transcribe(QByteArray bytes, Core::FileLocation location) {
		if (bytes.size() > kMaxAudioBytes || location.size > kMaxAudioBytes) {
			finish({});
			return;
		}
		Asr::Engine::Instance().transcribeRecording(
			std::move(bytes), std::move(location), _guard.make_guard(),
			[this](std::vector<Asr::Segment> segments) {
				auto result = Lyrics();
				result.lines.reserve(segments.size());
				for (auto &segment : segments) {
					result.lines.push_back({ segment.startMs, std::move(segment.text) });
				}
				result.found = result.synced = !result.lines.empty();
				finish(std::move(result));
			});
	}

	const QString _key;
	const AudioMsgId _audio;
	std::vector<Fn<void(Lyrics)>> _waiting;
	QByteArray _bytes;
	std::unique_ptr<Media::Streaming::Loader> _loader;
	base::Timer _timeout;
	base::binary_guard _guard;
	rpl::lifetime _lifetime;
};

std::unique_ptr<Recognition> &Active() {
	static auto active = std::unique_ptr<Recognition>();
	return active;
}

} // namespace

void FetchPlaybackLyrics(const Track &track, Fn<void(Lyrics)> done) {
	const auto audio = Media::Player::instance()->current(kSong);
	const auto session = audio.audio()
		? base::make_weak(&audio.audio()->session())
		: base::weak_ptr<Main::Session>();
	FetchLyrics(track.id, track.title, track.artist, track.durationMs,
		[key = track.id, duration = track.durationMs, audio, session,
		 done = std::move(done)](Lyrics result) {
			if (result.found || Core::Quitting() || !session.get()
				|| duration > Asr::kMaxRecordingDurationMs
				|| !SameAudio(audio, Media::Player::instance()->current(kSong))) {
				done(std::move(result));
				return;
			}
			const auto &cache = GeneratedCache();
			if (const auto i = cache.find(key); i != cache.end()) {
				done(i->second);
				return;
			}
			auto generate = [done, unavailable = result.unavailable](Lyrics lyrics) {
				lyrics.unavailable = !lyrics.found && unavailable;
				done(std::move(lyrics));
			};
			auto &active = Active();
			if (active && active->matches(audio)) {
				active->join(std::move(generate));
				return;
			}
			if (active) {
				active->finish({});
			}
			active = std::make_unique<Recognition>(key, audio, std::move(generate));
			active->start();
		});
}

} // namespace Mokagram::Music
