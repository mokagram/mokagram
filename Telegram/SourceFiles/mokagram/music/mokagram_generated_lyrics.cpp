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
#include "mokagram/music/mokagram_lyrics_alignment.h"
#include "mokagram/music/mokagram_soundcloud.h"
#include "mokagram/settings/mokagram_settings.h"

#include <crl/crl.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>

#include <map>
#include <tuple>

namespace Mokagram::Music {
namespace {

constexpr auto kSong = AudioMsgId::Type::Song;
constexpr auto kMaxAudioBytes = int64(64 * 1024 * 1024);
constexpr auto kPartTimeout = crl::time(30000);
constexpr auto kMaxTranscriptBytes = 1024 * 1024;
constexpr auto kMaxDiskTranscripts = 96;

[[nodiscard]] const char *SongLanguages(const QString &title, const QString &artist) {
	const auto clean = CleanTitle(title, artist);
	auto name = QStringView(clean);
	if (const auto bracket = name.indexOf(u'('); bracket > 0) {
		name = name.left(bracket);
	}
	if (const auto bracket = name.indexOf(u'['); bracket > 0) {
		name = name.left(bracket);
	}
	auto russian = false;
	auto ukrainian = false;
	auto cyrillic = false;
	for (const auto ch : name) {
		const auto lower = ch.toLower();
		russian |= QStringView(u"ыэъё").contains(lower);
		ukrainian |= QStringView(u"іїєґ").contains(lower);
		cyrillic |= ch.script() == QChar::Script_Cyrillic;
	}
	return (russian != ukrainian)
		? (russian ? "ru" : "uk")
		: (cyrillic ? "ru,uk" : nullptr);
}

[[nodiscard]] Lyrics LyricsFromWords(const std::vector<Asr::TimedWord> &words) {
	auto result = Lyrics();
	for (const auto &word : words) {
		const auto startLine = [&] {
			if (result.lines.empty()) {
				return true;
			} else if (word.join) {
				return false;
			}
			const auto &line = result.lines.back();
			const auto gap = word.startMs - line.words.back().endMs;
			const auto phraseEnd = QStringView(u".?!…;:").contains(line.text.back());
			return gap >= 550
				|| phraseEnd
				|| line.words.size() >= 9
				|| line.text.size() + 1 + word.text.size() > 48
				|| (line.words.size() >= 3 && (gap >= 280 || line.text.back() == u','));
		}();
		if (startLine) {
			result.lines.push_back({ word.startMs });
		}
		auto &line = result.lines.back();
		if (!line.text.isEmpty() && !word.join) {
			line.text.append(u' ');
		}
		const auto from = int(line.text.size());
		line.text += word.text;
		line.words.push_back({ word.startMs, word.endMs, from, int(line.text.size()) });
	}
	result.found = result.synced = !result.lines.empty();
	return result;
}

[[nodiscard]] bool SameAudio(const AudioMsgId &a, const AudioMsgId &b) {
	return a.audio() == b.audio() && a.contextId() == b.contextId();
}

using AlignmentKey = std::vector<std::tuple<QString, QString, QString>>;

[[nodiscard]] AlignmentKey SourceText(const Lyrics &source) {
	auto result = AlignmentKey();
	result.reserve(source.lines.size());
	for (const auto &line : source.lines) {
		result.emplace_back(line.text, line.translation, line.romanization);
	}
	return result;
}

struct Alignment {
	std::optional<Lyrics> result;
	std::vector<Fn<void(Lyrics)>> waiting;
};

struct Transcript {
	const std::vector<Asr::TimedWord> words;
	Lyrics generated;
	std::map<AlignmentKey, Alignment> aligned;
};

using TranscriptPtr = std::shared_ptr<Transcript>;

[[nodiscard]] QString TranscriptKey(
		uint64 accountId,
		const QString &trackId,
		int durationMs,
		const char *languages) {
	const auto key = QJsonDocument(QJsonArray{
		QString::number(accountId),
		trackId,
		durationMs,
		QString::fromLatin1(languages),
	}).toJson(QJsonDocument::Compact);
	return QString::fromLatin1(
		QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex());
}

[[nodiscard]] QString TranscriptPath(const QString &key) {
	const auto base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	return base.isEmpty()
		? QString()
		: base + u"/mokagram_music/asr_lyrics_v1/"_q + key + u".json"_q;
}

crl::queue &TranscriptDiskQueue() {
	static const auto result = new crl::queue();
	return *result;
}

[[nodiscard]] TranscriptPtr ReadTranscript(const QString &path) {
	const auto info = QFileInfo(path);
	auto file = QFile(path);
	if (!info.isFile() || info.isSymLink()
		|| !file.open(QIODevice::ReadOnly) || file.size() > kMaxTranscriptBytes) {
		return {};
	}
	const auto bytes = file.read(kMaxTranscriptBytes + 1);
	if (bytes.size() > kMaxTranscriptBytes) {
		return {};
	}
	const auto rows = QJsonDocument::fromJson(bytes).array();
	if (rows.isEmpty()) {
		return {};
	}
	auto words = std::vector<Asr::TimedWord>();
	words.reserve(rows.size());
	auto previousEnd = qint64(0);
	for (const auto &row : rows) {
		const auto fields = row.toArray();
		if (fields.size() != 4 || !fields.at(3).isBool()) {
			return {};
		}
		const auto start = fields.at(0).toInteger(-1);
		const auto end = fields.at(1).toInteger(-1);
		auto text = fields.at(2).toString();
		if (start < previousEnd || end <= start || end > Asr::kMaxRecordingDurationMs
			|| text.trimmed().isEmpty()) {
			return {};
		}
		words.push_back({ start, end, std::move(text), fields.at(3).toBool() });
		previousEnd = end;
	}
	return std::make_shared<Transcript>(std::move(words));
}

void SaveTranscript(const QString &path, TranscriptPtr transcript) {
	if (path.isEmpty()) {
		return;
	}
	TranscriptDiskQueue().async([path, transcript = std::move(transcript)] {
		auto rows = QJsonArray();
		for (const auto &word : transcript->words) {
			rows.push_back(QJsonArray{ word.startMs, word.endMs, word.text, word.join });
		}
		const auto bytes = QJsonDocument(rows).toJson(QJsonDocument::Compact);
		if (bytes.size() > kMaxTranscriptBytes) {
			return;
		}
		const auto directory = QFileInfo(path).absoluteDir();
		if (!directory.mkpath(u"."_q)) {
			return;
		}
		auto file = QSaveFile(path);
		if (!file.open(QIODevice::WriteOnly)
			|| !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)
			|| file.write(bytes) != bytes.size()
			|| !file.commit()) {
			return;
		}
		const auto entries = directory.entryInfoList(
			{ u"*.json"_q }, QDir::Files | QDir::NoSymLinks, QDir::Time);
		for (auto i = kMaxDiskTranscripts; i < entries.size(); ++i) {
			QFile::remove(entries[i].absoluteFilePath());
		}
	});
}

std::map<QString, TranscriptPtr> &TranscriptCache() {
	static auto cache = std::map<QString, TranscriptPtr>();
	return cache;
}

void SyncPlainLyrics(
		TranscriptPtr transcript,
		AlignmentKey key,
		Lyrics source,
		Fn<void(Lyrics)> done) {
	const auto entry = &transcript->aligned[std::move(key)];
	if (entry->result) {
		done(*entry->result);
		return;
	}
	const auto start = entry->waiting.empty();
	entry->waiting.push_back(std::move(done));
	if (start) {
		crl::async([transcript = std::move(transcript), entry,
				source = std::move(source)]() mutable {
			auto aligned = AlignLyrics(std::move(source), transcript->words);
			crl::on_main([transcript = std::move(transcript), entry,
					aligned = std::move(aligned)]() mutable {
				(void)transcript; // Keeps the map entry alive.
				entry->result = aligned.synced ? std::move(aligned) : Lyrics();
				const auto waiting = std::exchange(entry->waiting, {});
				for (const auto &done : waiting) {
					done(*entry->result);
				}
			});
		});
	}
}

class Recognition;
std::unique_ptr<Recognition> &Active();

class Recognition final : public base::has_weak_ptr {
public:
	Recognition(
		QString key,
		const AudioMsgId &audio,
		const char *languages,
		Fn<void(TranscriptPtr)> done)
	: _key(std::move(key))
	, _path(TranscriptPath(_key))
	, _audio(audio)
	, _languages(languages)
	, _timeout([this] { finish({}); }) {
		join(std::move(done));
	}

	[[nodiscard]] bool matches(const AudioMsgId &audio) const {
		return SameAudio(_audio, audio);
	}

	void join(Fn<void(TranscriptPtr)> done) {
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

		if (_path.isEmpty()) {
			startAudio();
			return;
		}
		const auto loaded = crl::guard(this, [this](TranscriptPtr transcript) {
			if (transcript) {
				finish(std::move(transcript));
			} else {
				startAudio();
			}
		});
		TranscriptDiskQueue().async([path = _path, loaded] {
			crl::on_main([loaded, transcript = ReadTranscript(path)]() mutable {
				loaded(std::move(transcript));
			});
		});
	}

private:
	void startAudio() {
		const auto document = _audio.audio();

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

public:
	void finish(TranscriptPtr result) {
		if (result) {
			auto &cache = TranscriptCache();
			if (cache.size() >= 64) {
				cache.erase(cache.begin());
			}
			cache[_key] = result;
		}
		auto waiting = std::move(_waiting);
		Active().reset();
		crl::on_main([waiting = std::move(waiting), result = std::move(result)] {
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
		Asr::Engine::Instance().transcribeSong(
			std::move(bytes), std::move(location), _languages, _guard.make_guard(),
			[this](std::vector<Asr::TimedWord> words) {
				if (words.empty()) {
					finish({});
					return;
				}
				auto transcript = std::make_shared<Transcript>(std::move(words));
				SaveTranscript(_path, transcript);
				finish(std::move(transcript));
			});
	}

	const QString _key;
	const QString _path;
	const AudioMsgId _audio;
	const char *const _languages;
	std::vector<Fn<void(TranscriptPtr)>> _waiting;
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

void FetchPlaybackLyrics(
		const Track &track,
		Fn<void(Lyrics)> done,
		bool syncPlain,
		Fn<void(bool)> syncing) {
	const auto audio = Media::Player::instance()->current(kSong);
	const auto provider = Settings::Int(u"lyrics.provider"_q);
	const auto session = audio.audio()
		? base::make_weak(&audio.audio()->session())
		: base::weak_ptr<Main::Session>();
	const auto current = [=] {
		return !Core::Quitting()
			&& session.get()
			&& provider == Settings::Int(u"lyrics.provider"_q)
			&& SameAudio(audio, Media::Player::instance()->current(kSong));
	};
	FetchLyrics(track.id, track.title, track.artist, track.durationMs,
		[id = track.id, duration = track.durationMs, audio, current, syncPlain,
		 languages = SongLanguages(track.title, track.artist),
		 done = std::move(done), syncing = std::move(syncing)](Lyrics source) {
			if (source.synced || (source.found && !syncPlain)
				|| duration > Asr::kMaxRecordingDurationMs || !current()) {
				done(std::move(source));
				return;
			}
			const auto key = TranscriptKey(
				audio.audio()->session().uniqueId(), id, duration, languages);
			const auto &cache = TranscriptCache();
			const auto i = cache.find(key);
			const auto cached = (i != cache.end()) ? i->second : nullptr;
			auto alignmentKey = AlignmentKey();
			if (source.found) {
				alignmentKey = SourceText(source);
				if (cached) {
					const auto entry = cached->aligned.find(alignmentKey);
					if (entry != cached->aligned.end() && entry->second.result) {
						const auto &result = *entry->second.result;
						done(result.synced ? result : std::move(source));
						return;
					}
				}
				done(source);
				if (!current()) {
					return;
				}
			}
			if (syncing) {
				syncing(true);
			}
			auto complete = [done, current, syncing](std::optional<Lyrics> lyrics) {
				if (!current()) {
					return;
				}
				if (lyrics) {
					done(std::move(*lyrics));
				}
				if (syncing) {
					syncing(false);
				}
			};
			auto useTranscript = [current, complete = std::move(complete),
					alignmentKey = std::move(alignmentKey), source = std::move(source)](
					TranscriptPtr transcript) mutable {
				if (!current()) {
					return;
				}
				if (source.found) {
					if (transcript) {
						SyncPlainLyrics(
							std::move(transcript), std::move(alignmentKey), std::move(source),
							[complete](Lyrics aligned) {
								complete(aligned.synced
									? std::optional(std::move(aligned))
									: std::nullopt);
							});
					} else {
						complete(std::nullopt);
					}
				} else if (transcript) {
					if (!transcript->generated.found) {
						transcript->generated = LyricsFromWords(transcript->words);
					}
					complete(transcript->generated);
				} else {
					complete(std::move(source));
				}
			};
			if (cached) {
				useTranscript(cached);
				return;
			}
			auto &active = Active();
			if (active && active->matches(audio)) {
				active->join(std::move(useTranscript));
				return;
			}
			if (active) {
				active->finish({});
			}
			active = std::make_unique<Recognition>(key, audio, languages, std::move(useTranscript));
			active->start();
		});
}

} // namespace Mokagram::Music
