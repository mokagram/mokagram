// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 80ddf4c8
// Mokagram: лайки, история прослушиваний и персональная подборка «Для вас».
#pragma once

#include "mokagram/music/mokagram_soundcloud.h"

#include "rpl/producer.h"

#include <vector>

namespace Mokagram::Music {

// Лайки. Хранятся на диске (JSON), новые сверху.
[[nodiscard]] bool IsLiked(const QString &trackId);
void SetLiked(const Track &track, bool liked);
[[nodiscard]] std::vector<Track> LikedTracks();
// Лайки, пришедшие из аккаунта SoundCloud: их не отправляем обратно, а
// снятый в аккаунте лайк убирается и локально. Возвращает число новых треков.
// complete = false, если список аккаунта загружен не до конца: тогда локальные
// лайки не удаляем, чтобы не потерять то, что просто не попало в ответ.
[[nodiscard]] int MergeRemoteLikes(
	const std::vector<Track> &remote,
	bool complete = true);
void ClearRemoteLikes();
[[nodiscard]] rpl::producer<> LikesChanged();

struct Playlist {
	QString id;
	QString name;
	std::vector<Track> tracks;
	// Плейлист, импортированный из лайкнутых sets SoundCloud.
	bool remote = false;
	QString sourceId;
};

struct RemotePlaylist {
	QString id;
	QString name;
	std::vector<Track> tracks;
};

[[nodiscard]] std::vector<Playlist> Playlists();
[[nodiscard]] QString CreatePlaylist(const QString &name);
void RenamePlaylist(const QString &id, const QString &name);
void DeletePlaylist(const QString &id);
void AddPlaylistTrack(const QString &id, const Track &track);
void AddPlaylistTracks(const QString &id, const std::vector<Track> &tracks);
void RemovePlaylistTrack(const QString &id, const QString &trackId);
void MergeRemotePlaylists(const std::vector<RemotePlaylist> &playlists, bool complete);
void ClearRemotePlaylists();
[[nodiscard]] rpl::producer<> PlaylistsChanged();

struct RecentItem {
	Track track;
	Album album;
	bool isAlbum = false;
};

[[nodiscard]] std::vector<RecentItem> RecentItems();
void RecordRecentTrack(const Track &track);
void RecordRecentAlbum(const Album &album);
[[nodiscard]] rpl::producer<> RecentChanged();

// История: сколько реально прослушано. finished: трек доигран до конца.
// Пропуск (мало прослушано) считается сильным сигналом «не нравится».
void RecordListen(const Track &track, int listenedMs, bool finished);

// Есть ли данные для рекомендаций (лайки или история).
[[nodiscard]] bool HasTaste();

// Треки Telegram в очереди живут под id «telegram:...», в подборках не участвуют.
[[nodiscard]] bool IsTelegramId(const QString &trackId);

// Явная обратная связь из плеера. «Больше такого» усиливает вкус сильнее лайка,
// блокировка автора навсегда убирает его из подборок и потока.
void BoostTrack(const Track &track);
// Поднять событие лайков наружу (после синхронизации с аккаунтом).
void NotifyLikesChanged();
void BlockArtist(const Track &track);
void UnblockArtist(const QString &artistId);
[[nodiscard]] bool IsArtistBlocked(const Track &track);
[[nodiscard]] std::vector<QString> TrackIdentityKeys(const Track &track);

// Контекст бесконечного потока: что играет сейчас и что недавно было или уже в очереди.
struct StreamContext {
	Track current;
	std::vector<Track> avoid;
	bool avoidPreviousArtists = true;
};

// Пополнение потока: подборка с учётом сессии (последние дослушанные), плавных
// переходов, дедупликации и кулдаунов по истории на диске.
void BuildStream(
	StreamContext context,
	int count,
	Fn<void(std::vector<Track> tracks, QString note)> done);

// Похожие по звучанию: жанр и теги трека и слова из его названия (не ник
// автора), похожие по SoundCloud и немного вкуса слушателя. Автор трека и
// треки из avoid сюда не попадают: у автора есть отдельный ряд.
void BuildSimilar(
	Track seed,
	const std::vector<Track> &avoid,
	int count,
	Fn<void(std::vector<Track> tracks, QString note)> done);

// Собирает подборку. note: пояснение для интерфейса (например, «лайкни пару треков»).
void BuildForYou(
	int count,
	Fn<void(std::vector<Track> tracks, QString note)> done);

} // namespace Mokagram::Music
