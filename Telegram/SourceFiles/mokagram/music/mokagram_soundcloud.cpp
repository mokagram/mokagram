// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: cfb896c6
#include "mokagram/music/mokagram_soundcloud.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/music/mokagram_music_library.h"

#include "core/application.h"
#include "rpl/event_stream.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <map>
#include <memory>
#include <set>

namespace Mokagram::Music {
namespace {

constexpr auto kApi = "https://api-v2.soundcloud.com";
constexpr auto kHome = "https://soundcloud.com/";
constexpr auto kPageSize = 20;
constexpr auto kMaxAudioBytes = 200 * 1024 * 1024; // предел на размер трека
constexpr auto kMaxImageBytes = 32 * 1024 * 1024;  // предел на размер обложки
constexpr auto kMaxHlsSegments = 512;              // предел числа сегментов HLS
constexpr auto kUserAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0";
constexpr auto kMaxLikesPages = 50;   // предел страниц лайков при синхронизации
constexpr auto kMaxPushedLikes = 200; // за один проход отправляем не больше

QNetworkAccessManager &Net() {
	static auto manager = new QNetworkAccessManager();
	return *manager;
}

// Разрешаем только https на известных хостах: ответы API не должны уводить загрузку
// на file:// или чужой сервер. sndcdn.com и soundcloud.cloud — домены самого SoundCloud.
[[nodiscard]] bool SafeMediaUrl(const QUrl &url) {
	const auto host = url.host();
	return url.isValid()
		&& url.scheme() == u"https"_q
		&& url.userInfo().isEmpty()
		&& url.port(-1) == -1
		&& (host == u"api-v2.soundcloud.com"_q
			|| host == u"soundcloud.com"_q
			|| host.endsWith(u".sndcdn.com"_q)
			|| host.endsWith(u".soundcloud.cloud"_q));
}

// id трека — каноничное положительное число: всё остальное могло прийти из чужого ответа.
[[nodiscard]] bool ValidTrackId(const QString &id) {
	const auto number = id.toULongLong();
	return number != 0 && QString::number(number) == id;
}

[[nodiscard]] QString ClientIdFile() {
	return CacheDir() + u"/client_id.txt"_q;
}

QString &ClientId() {
	static auto id = [] {
		auto file = QFile(ClientIdFile());
		return file.open(QIODevice::ReadOnly)
			? QString::fromUtf8(file.readAll()).trimmed()
			: QString();
	}();
	return id;
}

[[nodiscard]] QString SessionFile() {
	return CacheDir() + u"/session.json"_q;
}

[[nodiscard]] bool ValidSessionToken(const QString &token) {
	// Настоящий oauth_token SoundCloud выглядит как "2-293746-185937845-AbCd".
	// Заголовок JWT (eyJ...) и прочие строки сюда не проходят: у них нет
	// ведущей группы цифр через дефис.
	return QRegularExpression(
		u"^[0-9][0-9]*-[0-9]+-[0-9]+-[A-Za-z0-9_-]{6,}$"_q).match(token).hasMatch()
		&& token.size() <= 256;
}

// Токен входа; живёт в файле рядом с client_id, как и он.
QString &AccountToken() {
	static auto token = [] {
		auto file = QFile(SessionFile());
		if (!file.open(QIODevice::ReadOnly)) {
			return QString();
		}
		const auto object = QJsonDocument::fromJson(file.readAll()).object();
		const auto result = object.value("token").toString();
		return ValidSessionToken(result) ? result : QString();
	}();
	return token;
}

void SaveSessionFile(const QString &token, const QJsonObject &user) {
	QDir().mkpath(CacheDir());
	auto file = QFile(SessionFile());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(QJsonDocument(QJsonObject{
		{ "token", token },
		{ "user", user },
	}).toJson(QJsonDocument::Compact));
}

void RemoveSessionFile() {
	QFile::remove(SessionFile());
}

QNetworkReply *Get(const QUrl &url, qint64 limit = 0) {
	auto request = QNetworkRequest(url);
	request.setRawHeader("User-Agent", kUserAgent);
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setTransferTimeout(20000);
	const auto reply = Net().get(request);
	if (limit > 0) {
		// Слишком большие ответы обрываем: readAll() не должен собирать гигабайты.
		reply->setReadBufferSize(limit + 1);
		QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
			[=](qint64 received, qint64 total) {
				if (received > limit || total > limit) {
					reply->abort();
				}
			});
	}
	return reply;
}

// Ищет client_id в скриптах главной страницы (в последних скриптах он обычно есть).
void FetchClientId(Fn<void(QString)> done) {
	const auto home = Get(QUrl(QString::fromLatin1(kHome)));
	QObject::connect(home, &QNetworkReply::finished, [=] {
		const auto html = QString::fromUtf8(home->readAll());
		home->deleteLater();
		auto scripts = QStringList();
		auto match = QRegularExpression(
			uR"(https://a-v2\.sndcdn\.com/assets/[^"']+\.js)"_q
		).globalMatch(html);
		while (match.hasNext()) {
			scripts.push_back(match.next().captured());
		}
		if (scripts.isEmpty()) {
			done({});
			return;
		}
		std::reverse(scripts.begin(), scripts.end());
		const auto next = std::make_shared<Fn<void(int)>>();
		*next = [=](int index) {
			if (index >= scripts.size()) {
				done({});
				return;
			}
			// Ссылки на скрипты вытащены из HTML главной: проверяем перед загрузкой.
			if (!SafeMediaUrl(QUrl(scripts[index]))) {
				(*next)(index + 1);
				return;
			}
			const auto reply = Get(QUrl(scripts[index]));
			QObject::connect(reply, &QNetworkReply::finished, [=] {
				const auto text = QString::fromUtf8(reply->readAll());
				reply->deleteLater();
				const auto found = QRegularExpression(
					u"client_id:\"([A-Za-z0-9]{32})\""_q).match(text);
				if (found.hasMatch()) {
					done(found.captured(1));
				} else {
					(*next)(index + 1);
				}
			});
		};
		(*next)(0);
	});
}

void EnsureClientId(Fn<void(bool)> done, bool force = false) {
	if (!force && !ClientId().isEmpty()) {
		done(true);
		return;
	}
	FetchClientId([=](QString id) {
		if (id.isEmpty()) {
			done(false);
			return;
		}
		ClientId() = id;
		QDir().mkpath(CacheDir());
		auto file = QFile(ClientIdFile());
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			file.write(id.toUtf8());
		}
		done(true);
	});
}

// client_id нужен всегда: по нему API отдаёт список треков и поток.
// oauth-токен аккаунта лишь добавляется к запросу, когда он есть.
void EnsureAnyClientId(Fn<void(bool)> done, bool force) {
	EnsureClientId(done, force);
}

// GET с автоматическим обновлением client_id при 401/403 (один повтор).
// client_id уходит всегда, вместе с ним, если есть, oauth-токен аккаунта.
void ApiGet(
		const QString &base,
		QUrlQuery query,
		Fn<void(QByteArray, QString)> done,
		bool retried = false) {
	EnsureAnyClientId([=](bool ok) {
		if (!ok) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("не удалось получить client_id SoundCloud")));
			return;
		}
		auto copy = query;
		if (!ClientId().isEmpty()) {
			copy.addQueryItem(u"client_id"_q, ClientId());
		}
		if (!AccountToken().isEmpty()) {
			copy.addQueryItem(u"oauth_token"_q, AccountToken());
		}
		auto url = QUrl(base);
		url.setQuery(copy);
		const auto reply = Get(url);
		QObject::connect(reply, &QNetworkReply::finished, [=] {
			const auto status = reply->attribute(
				QNetworkRequest::HttpStatusCodeAttribute).toInt();
			const auto data = reply->readAll();
			const auto error = reply->error();
			reply->deleteLater();
			if ((status == 401 || status == 403) && !retried) {
				EnsureAnyClientId([=](bool refreshed) {
					if (!refreshed) {
						done({}, QString::fromUtf8(Mokagram::Lang::Tr("client_id SoundCloud устарел")));
						return;
					}
					ApiGet(base, query, done, true);
				}, true);
				return;
			}
			if (error != QNetworkReply::NoError) {
				done({}, QString::fromUtf8(Mokagram::Lang::Tr("сеть: %1 (код %2)")).arg(
					reply->errorString()).arg(status));
				return;
			}
			done(data, QString());
		});
	}, false);
}

[[nodiscard]] Track ParseTrack(const QJsonObject &object) {
	auto result = Track();
	result.id = QString::number(object.value("id").toVariant().toLongLong());
	result.title = object.value("title").toString();
	const auto user = object.value("user").toObject();
	result.artist = object.value("publisher_metadata").toObject()
		.value("artist").toString().trimmed();
	if (result.artist.isEmpty()) {
		result.artist = user.value("username").toString();
	}
	result.artworkUrl = object.value("artwork_url").toString();
	if (result.artworkUrl.isEmpty()) {
		result.artworkUrl = user.value("avatar_url").toString();
	}
	result.permalink = object.value("permalink_url").toString();
	result.durationMs = object.value("duration").toInt();
	result.authorization = object.value("track_authorization").toString();
	result.artistId = QString::number(user.value("id").toVariant().toLongLong());
	result.artistArtworkUrl = user.value("avatar_url").toString();
	result.genre = object.value("genre").toString().trimmed().toLower();
	result.playbackCount = object.value("playback_count").toVariant().toLongLong();
	result.likesCount = object.value("likes_count").toVariant().toLongLong();
	result.createdAt = QDateTime::fromString(
		object.value("created_at").toString(),
		Qt::ISODate).toSecsSinceEpoch();
	{
		// tag_list: слова через пробел, составные теги в кавычках.
		const auto raw = object.value("tag_list").toString();
		auto match = QRegularExpression(u"\"([^\"]+)\"|(\\S+)"_q).globalMatch(raw);
		while (match.hasNext()) {
			const auto m = match.next();
			const auto tag = (m.captured(1).isEmpty() ? m.captured(2) : m.captured(1)).toLower();
			if (!tag.isEmpty() && result.tags.size() < 12) {
				result.tags.push_back(tag);
			}
		}
	}
	auto progressive = QString();
	auto hlsMp3 = QString();
	auto hlsAac = QString();
	for (const auto &value : object.value("media").toObject()
			.value("transcodings").toArray()) {
		const auto item = value.toObject();
		const auto format = item.value("format").toObject();
		const auto protocol = format.value("protocol").toString();
		const auto mime = format.value("mime_type").toString();
		const auto url = item.value("url").toString();
		if (item.value("snipped").toBool()) {
			continue; // Превью-обрезка.
		}
		if (protocol == u"progressive"_q && mime.contains(u"mpeg"_q)) {
			progressive = url;
		} else if (protocol == u"hls"_q && mime.contains(u"mpeg"_q)
			&& !mime.contains(u"mpegurl"_q)) {
			hlsMp3 = url;
		} else if (protocol == u"hls"_q && mime.contains(u"mp4"_q)) {
			hlsAac = url;
		}
	}
	if (!progressive.isEmpty()) {
		result.transcodingUrl = progressive;
		result.extension = u"mp3"_q;
	} else if (!hlsAac.isEmpty()) {
		result.transcodingUrl = hlsAac;
		result.extension = u"m4a"_q;
		result.hls = true;
	} else if (!hlsMp3.isEmpty()) {
		result.transcodingUrl = hlsMp3;
		result.extension = u"mp3"_q;
		result.hls = true;
	}
	return result;
}

[[nodiscard]] QString CachePath(const Track &track) {
	// id попадает в путь к файлу: нечисловой id не пропускаем.
	if (!ValidTrackId(track.id)) {
		return QString();
	}
	return CacheDir() + '/' + track.id + '.' + track.extension;
}

void FinishFile(
		const QString &path,
		const QByteArray &bytes,
		Fn<void(const QString &, const QString &)> done) {
	QDir().mkpath(CacheDir());
	auto file = QFile(path + u".part"_q);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
		|| file.write(bytes) != bytes.size()) {
		done({}, QString::fromUtf8(Mokagram::Lang::Tr("не удалось записать файл")));
		return;
	}
	file.close();
	QFile::remove(path);
	file.rename(path);
	done(path, QString());
}

// Скачивает HLS: init-сегмент (EXT-X-MAP) и сегменты по порядку, склеивает в один файл.
void DownloadHls(
		const QString &playlistUrl,
		const QString &path,
		Fn<void(const QString &, const QString &)> done) {
	const auto reply = Get(QUrl(playlistUrl), kMaxAudioBytes);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		const auto text = QString::fromUtf8(reply->readAll());
		const auto base = reply->url();
		const auto failed = (reply->error() != QNetworkReply::NoError);
		reply->deleteLater();
		if (failed) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("не загрузился плейлист")));
			return;
		}
		auto parts = QStringList();
		for (const auto &raw : text.split('\n')) {
			const auto line = raw.trimmed();
			if (line.startsWith(u"#EXT-X-MAP:"_q)) {
				const auto found = QRegularExpression(
					u"URI=\"([^\"]+)\""_q).match(line);
				if (found.hasMatch()) {
					parts.push_back(base.resolved(
						QUrl(found.captured(1))).toString());
				}
			} else if (!line.isEmpty() && !line.startsWith('#')) {
				parts.push_back(base.resolved(QUrl(line)).toString());
			}
		}
		if (parts.isEmpty()) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("пустой плейлист")));
			return;
		}
		// Ссылки в плейлисте могли указать куда угодно: проверяем каждую.
		for (const auto &part : parts) {
			if (!SafeMediaUrl(QUrl(part))) {
				done({}, QString::fromUtf8(Mokagram::Lang::Tr("плейлист ведёт на чужой сервер")));
				return;
			}
		}
		if (parts.size() > kMaxHlsSegments) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("слишком большой плейлист")));
			return;
		}
		const auto buffer = std::make_shared<QByteArray>();
		const auto next = std::make_shared<Fn<void(int)>>();
		*next = [=](int index) {
			if (index >= parts.size()) {
				FinishFile(path, *buffer, done);
				return;
			}
			const auto segment = Get(QUrl(parts[index]), kMaxAudioBytes);
			QObject::connect(segment, &QNetworkReply::finished, [=] {
				const auto ok = (segment->error() == QNetworkReply::NoError);
				if (ok) {
					buffer->append(segment->readAll());
				}
				segment->deleteLater();
				// Общий размер склейки ограничен так же, как обычная загрузка.
				if (!ok || buffer->size() > kMaxAudioBytes) {
					done({}, QString::fromUtf8(Mokagram::Lang::Tr("сбой загрузки сегмента")));
					return;
				}
				(*next)(index + 1);
			});
		};
		(*next)(0);
	});
}

} // namespace

void LoadImage(const QString &url, Fn<void(QImage)> done) {
	static auto cache = std::map<QString, QImage>();
	const auto i = cache.find(url);
	if (i != cache.end()) {
		done(i->second);
		return;
	}
	if (url.isEmpty()) {
		done(QImage());
		return;
	}
	// Ссылка пришла из ответа API: проверяем перед загрузкой.
	if (!SafeMediaUrl(QUrl(url))) {
		done(QImage());
		return;
	}
	const auto reply = Get(QUrl(url), kMaxImageBytes);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		auto image = QImage::fromData(reply->readAll());
		reply->deleteLater();
		if (cache.size() > 300) {
			cache.clear();
		}
		cache[url] = image;
		done(image);
	});
}

namespace {
std::map<uint64, QImage> &Covers() {
	static auto result = std::map<uint64, QImage>();
	return result;
}
} // namespace

QImage CoverForDocument(uint64 documentId) {
	const auto &covers = Covers();
	const auto i = covers.find(documentId);
	return (i != covers.end()) ? i->second : QImage();
}

namespace {
rpl::event_stream<> &CoversStream() {
	static auto result = rpl::event_stream<>();
	return result;
}
} // namespace

void SetCover(uint64 documentId, QImage image) {
	Covers()[documentId] = std::move(image);
	CoversStream().fire({});
}

rpl::producer<> CoversChanged() {
	return CoversStream().events();
}

uint64 DocumentIdForTrack(const QString &trackId) {
	const auto id = trackId.toULongLong();
	// Префикс занимает старшие биты: слишком длинный id переполнит сумму.
	if (id >= (1ULL << 56)) {
		return 0;
	}
	return 0x4D4F4B0000000000ULL + id;
}

namespace {

void CollectionRequest(
		const QString &path,
		QUrlQuery query,
		Fn<void(std::vector<Track>)> done) {
	ApiGet(
		QString::fromLatin1(kApi) + path,
		std::move(query),
		[=](QByteArray data, QString error) {
			auto result = std::vector<Track>();
			if (error.isEmpty()) {
				const auto root = QJsonDocument::fromJson(data).object();
				for (const auto &value : root.value("collection").toArray()) {
					auto track = ParseTrack(value.toObject());
					if (!track.transcodingUrl.isEmpty() && !track.id.isEmpty()) {
						result.push_back(std::move(track));
					}
				}
			}
			done(std::move(result));
		});
}

} // namespace

void Resolve(
		const Track &track,
		Fn<void(Track, const QString &error)> done) {
	// id попадает в URL запроса: нечисловой id мог бы подменить путь.
	if (!ValidTrackId(track.id)) {
		done(track, QString::fromUtf8(Mokagram::Lang::Tr("некорректный id трека")));
		return;
	}
	ApiGet(
		QString::fromLatin1(kApi) + u"/tracks/"_q + track.id,
		QUrlQuery(),
		[=](QByteArray data, QString error) {
			if (!error.isEmpty()) {
				done(track, error);
				return;
			}
			auto fresh = ParseTrack(QJsonDocument::fromJson(data).object());
			if (fresh.transcodingUrl.isEmpty()) {
				done(track, QString::fromUtf8(Mokagram::Lang::Tr("трек недоступен (закрыт или удалён)")));
				return;
			}
			done(std::move(fresh), QString());
		});
}

void Related(
		const QString &trackId,
		int limit,
		Fn<void(std::vector<Track>)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"limit"_q, QString::number(limit));
	CollectionRequest(u"/tracks/"_q + trackId + u"/related"_q, q, std::move(done));
}

void LoadRelatedPage(
		const QString &trackId,
		const QString &cursor,
		Fn<void(RelatedPage)> done) {
	const auto endpoint = QString::fromLatin1(kApi)
		+ u"/tracks/"_q + trackId + u"/related"_q;
	const auto url = QUrl(cursor.isEmpty() ? endpoint : cursor);
	if (!trackId.toULongLong()
		|| url.scheme() != u"https"_q
		|| url.host() != QUrl(endpoint).host()
		|| url.path() != QUrl(endpoint).path()) {
		done({ {}, {}, u"Invalid radio source"_q });
		return;
	}
	auto query = QUrlQuery(url);
	query.removeAllQueryItems(u"client_id"_q);
	if (cursor.isEmpty()) {
		query.addQueryItem(u"limit"_q, u"50"_q);
		query.addQueryItem(u"linked_partitioning"_q, u"1"_q);
	}
	ApiGet(endpoint, std::move(query), [=](QByteArray data, QString error) {
		auto result = RelatedPage();
		result.error = error;
		if (error.isEmpty()) {
			const auto document = QJsonDocument::fromJson(data);
			const auto root = document.object();
			if (!document.isObject() || !root.value("collection").isArray()) {
				result.error = u"Invalid radio response"_q;
			} else {
				result.next = root.value("next_href").toString();
				for (const auto &value : root.value("collection").toArray()) {
					auto track = ParseTrack(value.toObject());
					if (track.id.toULongLong() && !track.transcodingUrl.isEmpty()) {
						result.tracks.push_back(std::move(track));
					}
				}
			}
		}
		done(std::move(result));
	});
}

void UserTracks(
		const QString &userId,
		int limit,
		Fn<void(std::vector<Track>)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"limit"_q, QString::number(limit));
	CollectionRequest(u"/users/"_q + userId + u"/tracks"_q, q, std::move(done));
}

void SearchLimited(
		const QString &query,
		int limit,
		Fn<void(std::vector<Track>)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"q"_q, query);
	q.addQueryItem(u"limit"_q, QString::number(limit));
	CollectionRequest(u"/search/tracks"_q, q, std::move(done));
}

namespace {

[[nodiscard]] Album ParseAlbum(const QJsonObject &object) {
	auto result = Album();
	result.id = QString::number(object.value("id").toVariant().toLongLong());
	result.title = object.value("title").toString();
	const auto user = object.value("user").toObject();
	result.artist = user.value("username").toString();
	result.artworkUrl = object.value("artwork_url").toString();
	if (result.artworkUrl.isEmpty()) {
		const auto tracks = object.value("tracks").toArray();
		if (!tracks.isEmpty()) {
			result.artworkUrl = tracks.first().toObject()
				.value("artwork_url").toString();
		}
	}
	if (result.artworkUrl.isEmpty()) {
		result.artworkUrl = user.value("avatar_url").toString();
	}
	result.trackCount = object.value("track_count").toInt();
	return result;
}

void AlbumsRequest(
		const QString &path,
		QUrlQuery query,
		Fn<void(std::vector<Album>, QString)> done) {
	ApiGet(
		QString::fromLatin1(kApi) + path,
		std::move(query),
		[=](QByteArray data, QString error) {
			auto result = std::vector<Album>();
			if (error.isEmpty()) {
				const auto root = QJsonDocument::fromJson(data).object();
				for (const auto &value : root.value("collection").toArray()) {
					auto album = ParseAlbum(value.toObject());
					if (album.id != u"0"_q && album.trackCount > 0) {
						result.push_back(std::move(album));
					}
				}
			}
			done(std::move(result), error);
		});
}

// Подгружает треки по id пачками по 50 и дописывает в found.
void HydrateTracks(
		std::shared_ptr<std::map<QString, Track>> found,
		QStringList missing,
		Fn<void(QString)> done) {
	if (missing.isEmpty()) {
		done(QString());
		return;
	}
	const auto chunk = missing.mid(0, 50);
	missing = missing.mid(chunk.size());
	auto q = QUrlQuery();
	q.addQueryItem(u"ids"_q, chunk.join(','));
	ApiGet(
		QString::fromLatin1(kApi) + u"/tracks"_q,
		q,
		[=](QByteArray data, QString error) {
			if (!error.isEmpty()) {
				done(error);
				return;
			}
			auto parseError = QJsonParseError();
			const auto json = QJsonDocument::fromJson(data, &parseError);
			if (parseError.error != QJsonParseError::NoError || !json.isArray()) {
				done(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить треки альбома. Попробуйте снова.")));
				return;
			}
			for (const auto &value : json.array()) {
				auto track = ParseTrack(value.toObject());
				if (track.id.toULongLong() && !track.title.isEmpty()) {
					track.unavailable = track.transcodingUrl.isEmpty();
					(*found)[track.id] = std::move(track);
				}
			}
			HydrateTracks(found, missing, done);
		});
}

} // namespace

void SearchAlbums(
		const QString &query,
		int limit,
		Fn<void(std::vector<Album>, QString error)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"q"_q, query);
	q.addQueryItem(u"limit"_q, QString::number(limit));
	AlbumsRequest(u"/search/albums"_q, q, std::move(done));
}

void TrackAlbums(
		const QString &trackId,
		Fn<void(std::vector<Album>)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"limit"_q, u"10"_q);
	AlbumsRequest(
		u"/tracks/"_q + trackId + u"/albums"_q,
		q,
		[=](std::vector<Album> albums, QString) { done(std::move(albums)); });
}

void AlbumTracks(
		const QString &albumId,
		Fn<void(std::vector<Track>, QString error)> done) {
	ApiGet(
		QString::fromLatin1(kApi) + u"/playlists/"_q + albumId,
		QUrlQuery(),
		[=](QByteArray data, QString error) {
			if (!error.isEmpty()) {
				done({}, error);
				return;
			}
			auto parseError = QJsonParseError();
			const auto json = QJsonDocument::fromJson(data, &parseError);
			if (parseError.error != QJsonParseError::NoError
				|| !json.isObject()
				|| !json.object().value("tracks").isArray()) {
				done({}, QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить треки альбома. Попробуйте снова.")));
				return;
			}
			const auto root = json.object();
			auto order = QStringList();
			auto missing = QStringList();
			const auto found = std::make_shared<std::map<QString, Track>>();
			for (const auto &value : root.value("tracks").toArray()) {
				const auto object = value.toObject();
				const auto id = QString::number(
					object.value("id").toVariant().toLongLong());
				order.push_back(id);
				auto track = ParseTrack(object);
				if (id == u"0"_q) {
					continue;
				}
				if (track.transcodingUrl.isEmpty()) {
					missing.push_back(id);
				}
				if (!track.title.isEmpty()) {
					track.unavailable = track.transcodingUrl.isEmpty();
					(*found)[id] = std::move(track);
				}
			}
			HydrateTracks(found, missing, [=](QString error) {
				if (!error.isEmpty()) {
					done({}, error);
					return;
				}
				auto result = std::vector<Track>();
				for (const auto &id : order) {
					if (const auto i = found->find(id); i != found->end()) {
						result.push_back(i->second);
					}
				}
				done(std::move(result), QString());
			});
		});
}

QString CacheDir() {
	return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
		+ u"/mokagram_music"_q;
}

void Search(
		const QString &query,
		int offset,
		Fn<void(SearchResult)> done) {
	auto q = QUrlQuery();
	q.addQueryItem(u"q"_q, query);
	q.addQueryItem(u"limit"_q, QString::number(kPageSize));
	q.addQueryItem(u"offset"_q, QString::number(offset));
	ApiGet(
		QString::fromLatin1(kApi) + u"/search/tracks"_q,
		q,
		[=](QByteArray data, QString error) {
			auto result = SearchResult();
			if (!error.isEmpty()) {
				result.error = error;
				done(std::move(result));
				return;
			}
			const auto root = QJsonDocument::fromJson(data).object();
			for (const auto &value : root.value("collection").toArray()) {
				auto track = ParseTrack(value.toObject());
				if (!track.transcodingUrl.isEmpty()) {
					result.tracks.push_back(std::move(track));
				}
			}
			if (!root.value("next_href").toString().isEmpty()) {
				result.nextOffset = offset + kPageSize;
			}
			done(std::move(result));
		});
}

void Download(
		const Track &track,
		Fn<void(const QString &path, const QString &error)> done) {
	const auto path = CachePath(track);
	if (path.isEmpty()) {
		done({}, QString::fromUtf8(Mokagram::Lang::Tr("некорректный id трека")));
		return;
	}
	if (QFile::exists(path)) {
		done(path, QString());
		return;
	}
	auto q = QUrlQuery();
	if (!track.authorization.isEmpty()) {
		q.addQueryItem(u"track_authorization"_q, track.authorization);
	}
	// Ссылка на транскодинг пришла из ответа API: проверяем перед загрузкой.
	if (!SafeMediaUrl(QUrl(track.transcodingUrl))) {
		done({}, QString::fromUtf8(Mokagram::Lang::Tr("сервер вернул недопустимую ссылку")));
		return;
	}
	ApiGet(track.transcodingUrl, q, [=](QByteArray data, QString error) {
		if (!error.isEmpty()) {
			done({}, error);
			return;
		}
		const auto url = QJsonDocument::fromJson(data).object()
			.value("url").toString();
		if (url.isEmpty()) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("нет ссылки на поток")));
			return;
		}
		if (!SafeMediaUrl(QUrl(url))) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr("сервер вернул недопустимую ссылку")));
			return;
		}
		if (track.hls) {
			DownloadHls(url, path, done);
			return;
		}
		const auto reply = Get(QUrl(url), kMaxAudioBytes);
		QObject::connect(reply, &QNetworkReply::finished, [=] {
			const auto bytes = reply->readAll();
			const auto failed = (reply->error() != QNetworkReply::NoError);
			reply->deleteLater();
			if (failed) {
				done({}, QString::fromUtf8(Mokagram::Lang::Tr("сбой загрузки трека")));
				return;
			}
			FinishFile(path, bytes, done);
		});
	});
}

namespace Account {
namespace {

rpl::event_stream<> &ChangesStream() {
	static auto result = rpl::event_stream<>();
	return result;
}

QJsonObject &UserObject() {
	static auto result = []() -> QJsonObject {
		auto file = QFile(SessionFile());
		if (!file.open(QIODevice::ReadOnly)) {
			return {};
		}
		return QJsonDocument::fromJson(file.readAll()).object()
			.value("user").toObject();
	}();
	return result;
}

[[nodiscard]] Info ParseUser(const QJsonObject &object) {
	if (object.isEmpty()) {
		return {};
	}
	auto result = Info();
	result.id = QString::number(object.value("id").toVariant().toLongLong());
	result.username = object.value("username").toString();
	result.avatarUrl = object.value("avatar_url").toString();
	if (result.avatarUrl.isEmpty()) {
		result.avatarUrl = object.value("permalink_url").toString();
	}
	return result;
}

void FetchUser(Fn<void(bool)> done) {
	ApiGet(
		QString::fromLatin1(kApi) + u"/me"_q,
		QUrlQuery(),
		[=](QByteArray data, QString error) {
			if (!error.isEmpty() || data.isEmpty()) {
				done(false);
				return;
			}
			UserObject() = QJsonDocument::fromJson(data).object();
			SaveSessionFile(AccountToken(), UserObject());
			ChangesStream().fire({});
			done(true);
		});
}

[[nodiscard]] bool ValidApiUrl(const QString &url) {
	const auto parsed = QUrl(url);
	return parsed.isValid()
		&& parsed.scheme() == u"https"_q
		&& parsed.host() == u"api-v2.soundcloud.com"_q;
}

[[nodiscard]] QString LikesUrl(const QString &userId, const QString &next) {
	if (!next.isEmpty()) {
		return ValidApiUrl(next) ? next : QString();
	}
	return ValidApiUrl(QString::fromLatin1(kApi))
		? QString::fromLatin1(kApi) + u"/users/"_q + userId + u"/likes"_q
		: QString();
}

struct SyncState {
	std::vector<Track> remote;
	std::vector<QString> pushed;
	QString next;
	int pages = 0;
	QString error;
	Fn<void(int, int, QString)> done;
};

} // namespace

bool IsLoggedIn() {
	return !AccountToken().isEmpty();
}

Info User() {
	return ParseUser(UserObject());
}

void Login(const QString &accessToken) {
	const auto token = accessToken.trimmed();
	if (!ValidSessionToken(token)) {
		return;
	}
	if (AccountToken() != token) {
		AccountToken() = token;
		UserObject() = QJsonObject();
	}
	SaveSessionFile(AccountToken(), UserObject());
	ChangesStream().fire({});
	FetchUser([](bool) {});
	if (AccountToken().isEmpty()) {
		return;
	}
	// Сразу подтягиваем лайки, чтобы лента не ждала следующего запуска.
	SyncLikes(nullptr);
}

void Logout() {
	AccountToken().clear();
	UserObject() = QJsonObject();
	RemoveSessionFile();
	ChangesStream().fire({});
	::Mokagram::Music::ClearRemoteLikes();
}

rpl::producer<> Changes() {
	return ChangesStream().events();
}

void LoadLikes(const QString &nextHref, Fn<void(LikesPage)> done) {
	const auto info = User();
	if (!IsLoggedIn() || info.id.isEmpty()) {
		done({ .error = QString::fromUtf8(Mokagram::Lang::Tr("нет аккаунта SoundCloud")) });
		return;
	}
	const auto url = LikesUrl(info.id, nextHref);
	if (url.isEmpty()) {
		done({ .error = QString::fromUtf8(Mokagram::Lang::Tr("некорректная ссылка SoundCloud")) });
		return;
	}
	ApiGet(url, QUrlQuery(), [=](QByteArray data, QString error) {
		auto result = LikesPage();
		if (!error.isEmpty()) {
			result.error = error;
			done(std::move(result));
			return;
		}
		const auto root = QJsonDocument::fromJson(data).object();
		for (const auto &value : root.value("collection").toArray()) {
			// Лайк бывает и треком, и плейлистом: берём только треки.
			auto track = ParseTrack(value.toObject());
			if (!track.id.isEmpty() && !track.title.isEmpty()) {
				result.tracks.push_back(std::move(track));
			}
		}
		const auto next = root.value("next_href").toString();
		result.next = ValidApiUrl(next) ? next : QString();
		done(std::move(result));
	});
}

void SetTrackLiked(const QString &trackId, bool liked) {
	if (!IsLoggedIn() || !ValidTrackId(trackId)) {
		return;
	}
	const auto info = User();
	if (info.id.isEmpty()) {
		return;
	}
	const auto url = QString::fromLatin1(kApi)
		+ u"/users/"_q + info.id + u"/track-likes/"_q + trackId;
	auto request = QNetworkRequest(QUrl(url));
	request.setRawHeader("User-Agent", kUserAgent);
	request.setTransferTimeout(20000);
	// PUT / DELETE /users/{id}/track-likes/{track}: id ставит лайк, удаляет его.
	const auto reply = liked
		? Net().put(request, QByteArray())
		: Net().deleteResource(request);
	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
	});
}

void SyncLikes(Fn<void(int remote, int pushed, QString error)> done) {
	auto state = std::make_shared<SyncState>();
	state->done = std::move(done);
	if (!IsLoggedIn()) {
		if (state->done) {
			state->done(0, 0, QString());
		}
		return;
	}
	const auto load = std::make_shared<Fn<void()>>();
	*load = [=] {
		// Лимит страниц: применяем собранное, не ждём бесконечности.
		++state->pages;
		LoadLikes(state->next, [=](LikesPage page) {
			if (!page.error.isEmpty()) {
				state->error = page.error;
				state->next.clear();
			} else {
				for (auto &track : page.tracks) {
					state->remote.push_back(std::move(track));
				}
				state->next = page.next;
			}
			if (!state->next.isEmpty() && state->pages < kMaxLikesPages) {
				(*load)();
				return;
			}
			// 1) Лайки из аккаунта — в локальные.
			auto remoteIds = std::set<QString>();
			auto remoteTracks = std::vector<Track>();
			for (const auto &track : state->remote) {
				if (remoteIds.insert(track.id).second) {
					remoteTracks.push_back(track);
				}
			}
			if (!state->error.isEmpty()) {
				if (state->done) {
					state->done(0, 0, state->error);
				}
				return;
			}
			const auto imported = ::Mokagram::Music::MergeRemoteLikes(remoteTracks);
			NotifyLikesChanged();
			// 2) Локальные лайки, которых нет в аккаунте, — в аккаунт.
			auto pushed = 0;
			for (const auto &track : LikedTracks()) {
				if (pushed >= kMaxPushedLikes) {
					break;
				}
				if (track.id.isEmpty() || remoteIds.contains(track.id)) {
					continue;
				}
				SetTrackLiked(track.id, true);
				++pushed;
			}
			if (state->done) {
				state->done(imported, pushed, QString());
			}
		});
	};
	(*load)();
}

} // namespace Account

} // namespace Mokagram::Music
