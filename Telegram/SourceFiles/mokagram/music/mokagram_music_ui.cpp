// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 7b8dec14
#include "mokagram/music/mokagram_music_ui.h"
#include "media/audio/media_audio_ffmpeg_loader.h"
#include "mokagram/music/mokagram_tyanka_hikka.h"
#include "mokagram/music/mokagram_music_presence.h"
#include "mokagram/ui/mokagram_box_parts.h"
#include "mokagram/lang/mokagram_lang.h"
#include "ui/widgets/rp_window.h"
#include "ui/boxes/confirm_box.h"
#include "mokagram/misc/mokagram_misc.h"

#include "mokagram/music/mokagram_soundcloud.h"
#include "mokagram/music/mokagram_soundcloud_login.h"
#include "mokagram/music/mokagram_youtube_music.h"
#include "mokagram/music/mokagram_music_canvas.h"
#include "mokagram/music/mokagram_music_library.h"
#include "mokagram/music/mokagram_lyrics.h"
#include "mokagram/music/mokagram_lyrics_ui.h"
#include "mokagram/music/mokagram_player_style.h"
#include "mokagram/music/mokagram_motion.h"
#include "mokagram/perf/mokagram_perf.h"
#include "media/clip/media_clip_reader.h"
#include "styles/style_mokagram_motion.h"
#include "mokagram/settings/mokagram_settings.h"
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include "mokagram/settings/mokagram_settings_box.h"

#include "base/invoke_queued.h"
#include "base/timer.h"
#include "base/unixtime.h"
#include "base/options.h"
#include "mainwindow.h"
#include "base/random.h"
#include <set>
#include "core/application.h"
#include "core/core_settings.h"
#include "window/window_controller.h"
#include "api/api_common.h"
#include "apiwrap.h"
#include "data/data_media_types.h"
#include "data/data_saved_music.h"
#include "data/data_search_controller.h"
#include "data/data_thread.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "lang/lang_tag.h"
#include "mokagram/plugins/mokagram_plugin_install.h"
#include "storage/storage_media_prepare.h"
#include "ui/chat/attach/attach_prepare.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_peer_menu.h"
#include "boxes/peer_list_controllers.h"
#include "styles/style_menu_icons.h"
#include "storage/localimageloader.h"
#include "storage/serialize_document.h"
#include "chat_helpers/stickers_lottie.h"
#include "chat_helpers/tabbed_panel.h"
#include "chat_helpers/tabbed_selector.h"
#include "core/version.h"
#include "history/view/media/history_view_media_common.h"
#include "history/view/media/history_view_sticker_player.h"
#include "lottie/lottie_single_player.h"
#include "styles/style_chat_helpers.h"

#include "core/file_location.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_session.h"
#include "main/main_session.h"
#include "media/audio/media_audio.h"
#include "media/player/media_player_instance.h"
#include "ui/effects/animations.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsEffect>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <array>
#include <span>
#include "ui/image/image.h"
#include "ui/image/image_prepare.h"
#include "ui/layers/generic_box.h"
#include "ui/color_contrast.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "ui/power_saving.h"
#include "ui/text/format_song_document_name.h"
#include "ui/text/format_values.h"
#include "ui/toast/toast.h"
#include "ui/vertical_list.h"
#include "styles/style_chat.h"
#include "styles/style_window.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/vertical_layout_reorder.h"
#include "ui/widgets/scroll_area.h"
#include "media/player/media_player_volume_controller.h"
#include "window/window_session_controller.h"
#include "mainwidget.h"
#include "window/window_peer_menu.h"
#include "boxes/peer_list_controllers.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_media_player.h"
#include "styles/style_widgets.h"

#include <map>
#include <QtSvg/QSvgRenderer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QStandardPaths>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QVariantAnimation>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QLinearGradient>
#include <QtGui/QGuiApplication>
#include "ui/chat/message_bubble.h"
#include "mokagram/music/mokagram_island_glyphs.h"
#include <QtSvg/QSvgRenderer>
#include <QtCore/QPointer>
#include <array>
#include <cmath>
#include <tuple>

namespace Mokagram::Music {
namespace {

using details::CoverBackground;
using details::CoverBackgroundForPlayback;
using details::kSwitchDuration;
using details::LyricsView;
using details::PlayingTrack;
using details::ResolveCommentsTrack;

class MusicSourceController final : public ChatsListBoxController {
public:
	MusicSourceController(
		not_null<Main::Session*> session,
		Fn<void(not_null<Data::Thread*>)> chosen)
	: ChatsListBoxController(session)
	, _session(session)
	, _chosen(std::move(chosen))
	, _key(u"music.pinned_sources.%1"_q.arg(session->uniqueId())) {
		Settings::Register({
			.key = _key,
			.kind = Settings::Kind::String,
			.defaultValue = QString(),
			.hidden = true,
		});
		_pins = Settings::String(_key).split(u',', Qt::SkipEmptyParts);
	}

	Main::Session &session() const override { return *_session; }

	void rowClicked(not_null<PeerListRow*> row) override {
		_chosen(_session->data().history(row->peer()));
	}

	base::unique_qptr<Ui::PopupMenu> rowContextMenu(
			QWidget *parent,
			not_null<PeerListRow*> row) override {
		auto menu = base::make_unique_q<Ui::PopupMenu>(parent, st::popupMenuWithIcons);
		const auto id = QString::number(row->peer()->id.value);
		const auto pinned = _pins.contains(id);
		menu->addAction(QString::fromUtf8(Lang::Tr(pinned
			? "Открепить источник" : "Закрепить источник")), [=] {
			if (pinned) {
				_pins.removeAll(id);
			} else {
				_pins.prepend(id);
				if (!delegate()->peerListFindRow(row->id())) {
					delegate()->peerListPrependRow(createRow(
						_session->data().history(row->peer())));
				}
			}
			Settings::SetString(_key, _pins.join(u','));
			_sortPinsQueued.call();
		});
		return menu;
	}

protected:
	void prepareViewHook() override {
		delegate()->peerListSetTitle(rpl::single(
			QString::fromUtf8(Lang::Tr("Источник музыки"))));
		for (const auto &id : _pins) {
			if (const auto peer = _session->data().peerLoaded(PeerId(id.toULongLong()))) {
				if (auto row = createRow(_session->data().history(peer))) {
					delegate()->peerListAppendRow(std::move(row));
				}
			}
		}
	}

	std::unique_ptr<Row> createRow(not_null<History*> history) override {
		const auto peer = history->peer;
		if (peer->isRepliesChat() || peer->isVerifyCodes()) return nullptr;
		if (!_pins.isEmpty()) {
			_sortPinsQueued.call();
		}
		return std::make_unique<Row>(history);
	}

private:
	void sortPins() {
		if (!_pins.isEmpty()) {
			auto ranks = base::flat_map<uint64, qsizetype>();
			for (auto i = qsizetype(0); i != _pins.size(); ++i) {
				ranks.try_emplace(_pins[i].toULongLong(), i);
			}
			const auto rank = [&](const PeerListRow &row) {
				const auto i = ranks.find(row.peer()->id.value);
				return i == ranks.end() ? _pins.size() : i->second;
			};
			delegate()->peerListSortRows([&](const PeerListRow &a, const PeerListRow &b) {
				return rank(a) < rank(b);
			});
		}
		delegate()->peerListRefreshRows();
	}

	const not_null<Main::Session*> _session;
	Fn<void(not_null<Data::Thread*>)> _chosen;
	const QString _key;
	QStringList _pins;
	SingleQueuedInvokation _sortPinsQueued{ [this] { sortPins(); } };
};

void ShowMusicSourceBox(
		not_null<Window::SessionController*> controller,
		Fn<bool(not_null<Data::Thread*>)> chosen) {
	const auto weak = std::make_shared<base::weak_qptr<Ui::BoxContent>>();
	*weak = controller->show(Box<PeerListBox>(
		std::make_unique<MusicSourceController>(&controller->session(),
			[=](not_null<Data::Thread*> thread) {
				if (chosen(thread) && *weak) (*weak)->closeBox();
			}),
		[](not_null<PeerListBox*> box) {
			box->addButton(rpl::single(QString::fromUtf8(Lang::Tr("Отмена"))),
				[=] { box->closeBox(); });
		}));
}

constexpr auto kRowHeight = 76;
constexpr auto kGroupedTrackHeight = 56;
constexpr auto kGroupedTrackLeadWidth = 40;
constexpr auto kGroupedTrackLeadingInset = 16;
constexpr auto kGroupedTrackTextGap = 12;
constexpr auto kGroupedTrackDurationInset = 24;
constexpr auto kGroupedTrackDurationColumn = 44;
constexpr auto kGroupedTrackActionSize = 40;
constexpr auto kCoverFadeDuration = crl::time(320);
constexpr auto kTypingDelay = crl::time(400);
constexpr auto kArtSize = 56;
constexpr auto kGroupedTrackMenuInset = 56;
constexpr auto kMusicListMaxWidth = 1000;
constexpr auto kMusicListGap = 8;
constexpr auto kFeaturedArtistHeight = 96;
constexpr auto kFeaturedArtistArtSize = 64;
constexpr auto kFeaturedArtistButtonWidth = 112;
constexpr auto kArtistTrackPageSize = 12;

struct Radio {
	Track seed;
	QString sourceId;
	std::set<QString> seen;
	std::set<QString> cursors;
	QString next;
	bool exhausted = false;
	bool loading = false;
};

struct QueueAlbum {
	QString id;
	std::vector<QString> trackIds;
};

struct Queue {
	std::shared_ptr<Radio> radio;
	std::shared_ptr<Radio> pendingRadio;
	base::flat_map<QString, FullMsgId> messages;
	base::weak_ptr<Main::Session> session;
	std::vector<Track> tracks;       // очередь воспроизведения
	QueueAlbum album;
	std::vector<Track> results;      // то, что сейчас показано в списке
	std::vector<Track> searchResults;
	std::vector<Track> forYou;
	QString forYouNote;
	int tab = 0;                     // 0 поиск, 1 лайкнутые, 2 для вас
	Track current;                   // играющий трек (с разобранными ссылками)
	bool recorded = true;            // прослушивание уже записано в историю
	QString lastQuery;
	bool custom = false;             // очередь собрана вручную
	int index = -1;
	QString currentId;
	int pendingToken = 0;
	rpl::lifetime lifetime;
	rpl::event_stream<> changes;
	base::weak_ptr<Window::SessionController> controller;
	bool subscribed = false;
	bool telegramSearch = false;
	PeerId selectedChat;
	// Умный поток: очередь сама пополняется, пока играет.
	bool stream = false;
	bool refilling = false;
	bool waiting = false;            // очередь кончилась, ждём пополнения
	int streamEpoch = 0;
	std::set<QString> autoIds;       // добавлено подборщиком, не вручную
};

Queue &QueueState() {
	static auto queue = Queue();
	return queue;
}

[[nodiscard]] bool IsTelegramTrack(const Track &track) {
	return track.id.startsWith(u"telegram:"_q);
}

[[nodiscard]] bool CanPlayTrack(const Track &track) {
	return !track.unavailable || HasYouTubeMusicMetadata(track);
}

[[nodiscard]] HistoryItem *TrackMessage(const Track &track) {
	const auto &queue = QueueState();
	const auto session = queue.session.get();
	const auto i = queue.messages.find(track.id);
	return (session && i != queue.messages.end())
		? session->data().message(i->second)
		: nullptr;
}

[[nodiscard]] bool CanForwardTrack(const Track &track) {
	if (!IsTelegramTrack(track)) {
		return true;
	}
	const auto item = TrackMessage(track);
	return item && item->allowsForward();
}

void BindController(not_null<Window::SessionController*> controller) {
	auto &queue = QueueState();
	if (queue.session.get() != &controller->session()) {
		++queue.pendingToken;
		queue.radio.reset();
		queue.pendingRadio.reset();
		queue.messages.clear();
		queue.selectedChat = {};
		queue.tracks.clear();
		queue.album = {};
		queue.results.clear();
		queue.searchResults.clear();
		queue.current = {};
		queue.currentId.clear();
		queue.lastQuery.clear();
		queue.index = -1;
		queue.custom = false;
		queue.recorded = true;
		queue.stream = false;
		queue.refilling = false;
		queue.waiting = false;
		++queue.streamEpoch;
		queue.autoIds.clear();
		queue.session = base::make_weak(&controller->session());
	}
	queue.controller = base::make_weak(controller);
}

[[nodiscard]] Track TrackFromMessage(not_null<HistoryItem*> item) {
	const auto document = item->media() ? item->media()->document() : nullptr;
	if (!document || !document->isSong()) {
		return {};
	}
	const auto name = Ui::Text::FormatSongNameFor(document).composedName();
	auto track = Track();
	const auto id = item->fullId();
	track.id = u"telegram:%1:%2"_q.arg(id.peer.value).arg(id.msg.bare);
	track.title = name.title;
	track.artist = name.performer;
	track.durationMs = document->duration();
	QueueState().messages.emplace(track.id, id);
	return track;
}

class ChatMusicSearch final {
public:
	ChatMusicSearch(
		not_null<Main::Session*> session,
		Fn<void(std::vector<Track>, QString)> done,
		PeerId peerId = {});
	~ChatMusicSearch();
	void search(const QString &query);
	void searchMore();
	void cancel();
	[[nodiscard]] bool hasMore() const;

private:
	base::weak_ptr<Main::Session> _session;
	Fn<void(std::vector<Track>, QString)> _done;
	PeerId _peerId;
	QString _query;
	std::vector<Track> _tracks;
	Data::MessagePosition _offset;
	mtpRequestId _requestId = 0;
	int _rate = 0;
	int _loaded = 0;
	int _generation = 0;
	bool _more = false;

};

ChatMusicSearch::ChatMusicSearch(
	not_null<Main::Session*> session,
		Fn<void(std::vector<Track>, QString)> done,
		PeerId peerId)
: _session(base::make_weak(session))
, _done(std::move(done))
, _peerId(peerId) {
}

ChatMusicSearch::~ChatMusicSearch() {
	cancel();
}

void ChatMusicSearch::cancel() {
	++_generation;
	if (const auto session = _session.get()) {
		session->api().request(base::take(_requestId)).cancel();
	}
	_more = false;
}

void ChatMusicSearch::search(const QString &query) {
	cancel();
	_query = query;
	_tracks.clear();
	_offset = {};
	_rate = 0;
	_loaded = 0;
	_more = true;
	searchMore();
}

bool ChatMusicSearch::hasMore() const {
	return _more && !_requestId;
}

void ChatMusicSearch::searchMore() {
	const auto session = _session.get();
	if (!session || _requestId || !_more) {
		return;
	}
	const auto generation = _generation;
	const auto received = [=](const MTPmessages_Messages &result) {
		if (generation != _generation
			|| QueueState().session.get() != session) {
			return;
		}
		_requestId = 0;
		if (result.type() == mtpc_messages_messagesNotModified) {
			_more = false;
			_done(_tracks, QString());
			return;
		}
		const auto parsed = Api::ParseGlobalMediaResult(session, result, false);
		_loaded += int(parsed.messageIds.size());
		_more = !parsed.messageIds.empty()
			&& _loaded < parsed.fullCount
			&& (parsed.offsetPosition != _offset || parsed.offsetRate != _rate);
		_offset = parsed.offsetPosition;
		_rate = parsed.offsetRate;
		for (const auto &position : parsed.messageIds) {
			if (const auto item = session->data().message(position.fullId)) {
				auto track = TrackFromMessage(item);
				if (!track.id.isEmpty()
					&& !ranges::contains(_tracks, track.id, &Track::id)) {
					_tracks.push_back(std::move(track));
				}
			}
		}
		_done(_tracks, QString());
	};
	const auto failed = [=](const MTP::Error &error) {
		if (generation != _generation
			|| QueueState().session.get() != session) {
			return;
		}
		_requestId = 0;
		_done(_tracks, MTP::IgnoreError(error) ? QString() : error.type());
	};
	if (_peerId) {
		_requestId = session->api().request(MTPmessages_Search(
			MTP_flags(0),
			session->data().peer(_peerId)->input(),
			MTP_string(_query),
			MTP_inputPeerEmpty(),
			MTP_inputPeerEmpty(),
			MTP_vector<MTPReaction>(),
			MTP_int(0),
			MTP_inputMessagesFilterMusic(),
			MTP_int(0),
			MTP_int(0),
			MTP_int(_offset.fullId.msg),
			MTP_int(0),
			MTP_int(50),
			MTP_int(0),
			MTP_int(0),
			MTP_long(0)
		)).done(received).fail(failed).send();
	} else {
		_requestId = session->api().request(MTPmessages_SearchGlobal(
			MTP_flags(0),
			MTP_int(0),
			MTPInputChannel(),
			MTP_string(_query),
			MTP_inputMessagesFilterMusic(),
			MTP_int(0),
			MTP_int(0),
			MTP_int(_rate),
			(_offset.fullId.peer
				? session->data().peer(_offset.fullId.peer)->input()
				: MTP_inputPeerEmpty()),
			MTP_int(_offset.fullId.msg),
			MTP_int(50)
		)).done(received).fail(failed).send();
	}
}

[[nodiscard]] QString FormatDuration(int ms) {
	const auto seconds = ms / 1000;
	return u"%1:%2"_q.arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
}

[[nodiscard]] QString ArtUrl(const QString &url, const QString &size) {
	auto result = url;
	result.replace(u"-large."_q, u"-"_q + size + u"."_q);
	return result;
}

void LoadQueuePlaybackCover(uint64 documentId, const Track &track) {
	const auto updateCover = [=](QImage image) {
		if (image.isNull()) {
			return;
		}
		const auto previous = CoverForDocument(documentId);
		if (std::min(image.width(), image.height())
			<= std::min(previous.width(), previous.height())) {
			return;
		}
		SetCover(documentId, std::move(image));
		QueueState().changes.fire({});
	};
	const auto preview = ArtUrl(track.artworkUrl, u"t500x500"_q);
	LoadImage(preview, updateCover);

	auto url = QUrl(preview);
	if (!url.host().endsWith(u".googleusercontent.com"_q)) {
		return;
	}
	static const auto dimensions = QRegularExpression(
		u"=(?:w[0-9]+-h[0-9]+|s[0-9]+)(?=-|$)"_q);
	auto path = url.path();
	if (!path.contains(dimensions)) {
		return;
	}
	path.replace(dimensions, u"=s1024"_q);
	url.setPath(path);
	const auto large = url.toString(QUrl::FullyEncoded);
	if (large != preview) {
		LoadImage(large, updateCover);
	}
}

// Mokagram: один поиск SoundCloud-двойника локальной песни, без точного совпадения пустой Track.
void ResolveLocalSoundCloud(const Track &track, Fn<void(Track)> done) {
	using Key = std::pair<QString, QString>;
	static auto found = std::map<Key, Track>();
	static auto pending = std::map<Key, std::vector<Fn<void(Track)>>>();
	const auto key = Key(track.title.trimmed(), track.artist.trimmed());
	if (key.first.isEmpty()) {
		done(Track());
		return;
	}
	if (const auto i = found.find(key); i != found.end()) {
		done(i->second);
		return;
	}
	const auto active = pending.contains(key);
	pending[key].push_back(std::move(done));
	if (active) {
		return;
	}
	Search(key.first, 0, [=](SearchResult result) {
		LOG(("[moka-dbg] sc search '%1' error '%2' n %3").arg(key.first).arg(result.error).arg(result.tracks.size()));
		for (auto i = 0; i < std::min(8, int(result.tracks.size())); ++i) LOG(("[moka-dbg]   cand '%1' / '%2' id %3").arg(result.tracks[i].title).arg(result.tracks[i].artist).arg(result.tracks[i].id));
		auto matches = std::vector<Track>();
		for (const auto &candidate : result.tracks) {
			if (candidate.title.trimmed() == key.first
				&& candidate.id.toULongLong()) {
				matches.push_back(candidate);
			}
		}
		if (matches.size() > 1) {
			std::erase_if(matches, [&](const Track &candidate) {
				return key.second.isEmpty()
					|| candidate.artist.trimmed() != key.second;
			});
		}
		const auto match = result.error.isEmpty() && matches.size() == 1
			? matches.front()
			: Track();
		if (result.error.isEmpty()) {
			if (found.size() >= 256) found.erase(found.begin());
			found[key] = match;
		}
		auto callbacks = std::move(pending[key]);
		pending.erase(key);
		for (auto &callback : callbacks) {
			callback(match);
		}
	});
}

void LoadLocalArtwork(const Track &track, Fn<void(QImage)> done) {
	ResolveLocalSoundCloud(track, [done = std::move(done)](Track match) mutable {
		LoadImage(
			match.id.isEmpty()
				? QString()
				: ArtUrl(match.artworkUrl, u"t500x500"_q),
			std::move(done));
	});
}

[[nodiscard]] not_null<DocumentData*> MakeDocument(
		not_null<Main::Session*> session,
		const Track &track,
		const QString &path) {
	const auto file = QFileInfo(path);
	const auto extension = file.suffix();
	auto attributes = QVector<MTPDocumentAttribute>();
	attributes.push_back(MTP_documentAttributeAudio(
		MTP_flags(MTPDdocumentAttributeAudio::Flag::f_title
			| MTPDdocumentAttributeAudio::Flag::f_performer),
		MTP_int(track.durationMs / 1000),
		MTP_string(track.title),
		MTP_string(track.artist),
		MTPstring()));
	attributes.push_back(MTP_documentAttributeFilename(
		MTP_string(track.title + '.' + extension)));
	// Тот же маппинг, что и в DocumentIdForTrack: слишком длинный id не переполнит префикс.
	const auto id = DocumentId(DocumentIdForTrack(track.id));
	const auto document = session->data().document(
		id,
		0,
		QByteArray(),
		base::unixtime::now(),
		attributes,
		extension == u"mp3"_q
			? u"audio/mpeg"_q
			: (extension == u"webm"_q ? u"audio/webm"_q : u"audio/mp4"_q),
		InlineImageLocation(),
		ImageWithLocation(),
		ImageWithLocation(),
		false,
		0,
		file.size());
	document->setLocation(Core::FileLocation(path));
	return document;
}

void PlayIndex(int index, bool skipUnavailable = false);
void RefillStream();
void Subscribe();
void PrefetchUpcoming();

void ResolveThen(const Track &track, Fn<void(Track, QString)> done) {
	if (!track.transcodingUrl.isEmpty()) {
		done(track, QString());
		return;
	}
	Resolve(track, std::move(done));
}

// Записывает в историю то, что успели послушать, до перехода на другой трек.
// Возвращает true, если трек пропущен в первые секунды.
bool RecordCurrentListen(bool finished) {
	auto &queue = QueueState();
	if (queue.recorded || queue.current.id.isEmpty()) {
		return false;
	}
	queue.recorded = true;
	auto listened = queue.current.durationMs;
	if (!finished) {
		const auto state = Media::Player::instance()->getState(AudioMsgId::Type::Song);
		const auto frequency = std::max<int64>(state.frequency, 1);
		listened = int(state.position * 1000 / frequency);
	}
	RecordListen(queue.current, listened, finished);
	return !finished && listened < 30 * 1000;
}

bool StreamEnabled() {
	return Mokagram::Settings::Bool(u"music.smart_stream"_q);
}

// Убирает из хвоста очереди то, что добавил подборщик (после позиции from).
void DropUpcoming(Fn<bool(const Track &)> predicate, int from, bool onlyAuto) {
	auto &queue = QueueState();
	for (auto i = int(queue.tracks.size()) - 1; i > from; --i) {
		const auto &t = queue.tracks[i];
		if ((!onlyAuto || queue.autoIds.contains(t.id)) && predicate(t)) {
			queue.autoIds.erase(t.id);
			queue.tracks.erase(queue.tracks.begin() + i);
		}
	}
	queue.changes.fire({});
	PrefetchUpcoming();
}

void RememberRadioTrack(Radio &radio, const Track &track) {
	for (const auto &key : TrackIdentityKeys(track)) {
		radio.seen.insert(key);
	}
}

[[nodiscard]] bool RadioAccepts(const Radio &radio, const Track &track) {
	if (!track.id.toULongLong() || IsArtistBlocked(track)) {
		return false;
	}
	for (const auto &key : TrackIdentityKeys(track)) {
		if (radio.seen.contains(key)) {
			return false;
		}
	}
	return true;
}

void FetchRadioBatch(
		std::shared_ptr<Radio> radio,
		Fn<bool()> valid,
		Fn<void(std::vector<Track>, QString)> done,
		int pagesLeft = 4) {
	const auto cursor = radio->next;
	LoadRelatedPage(radio->sourceId, cursor, [=](RelatedPage page) {
		if (!valid()) {
			return;
		}
		if (!page.error.isEmpty()) {
			done({}, QString::fromUtf8(Mokagram::Lang::Tr(
				"Не удалось загрузить радио. Попробуйте ещё раз.")));
			return;
		}
		radio->cursors.insert(cursor);
		radio->next = page.next;
		radio->exhausted = page.next.isEmpty()
			|| radio->cursors.contains(page.next);
		auto tracks = std::vector<Track>();
		for (auto &track : page.tracks) {
			if (RadioAccepts(*radio, track)) {
				RememberRadioTrack(*radio, track);
				tracks.push_back(std::move(track));
			}
		}
		if (tracks.empty() && !radio->exhausted && pagesLeft > 1) {
			FetchRadioBatch(radio, valid, done, pagesLeft - 1);
		} else {
			done(std::move(tracks), QString());
		}
	});
}

void RefillRadio() {
	auto &queue = QueueState();
	const auto radio = queue.radio;
	if (!radio || radio->loading) {
		return;
	}
	if (radio->exhausted) {
		if (!base::take(queue.waiting)) {
			return;
		}
		if (const auto controller = queue.controller.get()) {
			controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
				"Радио закончилось. Запустите радио от другого трека.")));
		}
		return;
	}
	for (const auto &track : queue.tracks) {
		RememberRadioTrack(*radio, track);
	}
	radio->loading = true;
	const auto session = queue.session;
	const auto valid = [=] {
		return session && QueueState().session.get() == session.get()
			&& QueueState().radio == radio;
	};
	FetchRadioBatch(radio, valid, [=](std::vector<Track> tracks, QString error) {
		auto &queue = QueueState();
		radio->loading = false;
		auto queued = Radio();
		for (const auto &track : queue.tracks) {
			RememberRadioTrack(queued, track);
		}
		for (auto &track : tracks) {
			if (RadioAccepts(queued, track)) {
				RememberRadioTrack(queued, track);
				queue.tracks.push_back(std::move(track));
			}
		}
		const auto waiting = base::take(queue.waiting);
		queue.changes.fire({});
		PrefetchUpcoming();
		if (waiting && queue.index + 1 < int(queue.tracks.size())) {
			PlayIndex(queue.index + 1);
		} else if (waiting || !error.isEmpty()) {
			if (const auto controller = queue.controller.get()) {
				controller->showToast(error.isEmpty()
					? QString::fromUtf8(Mokagram::Lang::Tr(
						"Нет новых подходящих треков для радио."))
					: error);
			}
		}
	});
}

void CommitRadio(
		std::shared_ptr<Radio> radio,
		std::vector<Track> tracks,
		const Track &seed,
		const QString &path) {
	auto &queue = QueueState();
	const auto controller = queue.controller.get();
	queue.pendingRadio.reset();
	if (!controller) {
		return;
	}
	if (IsArtistBlocked(seed)) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
			"Радио недоступно: автор скрыт")));
		return;
	}
	tracks.erase(std::remove_if(tracks.begin(), tracks.end(), [](const Track &track) {
		return IsArtistBlocked(track);
	}), tracks.end());
	if (tracks.empty()) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
			"Нет новых подходящих треков для радио.")));
		return;
	}
	const auto keepPlaying = PlayingTrack().id == seed.id;
	if (!keepPlaying) {
		RecordCurrentListen(false);
	}
	++queue.pendingToken;
	++queue.streamEpoch;
	queue.pendingRadio.reset();
	queue.radio = radio;
	queue.stream = false;
	queue.refilling = false;
	queue.waiting = false;
	queue.custom = false;
	queue.autoIds.clear();
	queue.tracks = { seed };
	queue.album = {};
	queue.tracks.insert(queue.tracks.end(), tracks.begin(), tracks.end());
	queue.index = 0;
	queue.currentId = seed.id;
	if (!keepPlaying && IsTelegramTrack(seed)) {
		PlayIndex(0);
	} else if (!keepPlaying) {
		queue.current = seed;
		queue.recorded = false;
		const auto document = MakeDocument(&controller->session(), seed, path);
		LoadQueuePlaybackCover(document->id, seed);
		Media::Player::instance()->play(
			AudioMsgId(document, FullMsgId()),
			std::nullopt);
		RecordRecentTrack(seed);
	}
	Subscribe();
	queue.changes.fire({});
	PrefetchUpcoming();
	controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
		"Радио по треку: ")) + seed.title);
	if (queue.tracks.size() <= 4 && !radio->exhausted) {
		RefillRadio();
	}
}

void FailRadio() {
	QueueState().pendingRadio.reset();
	if (const auto controller = QueueState().controller.get()) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
			"Не удалось загрузить радио. Попробуйте ещё раз.")));
	}
}

void PrepareRadioSeed(
		std::shared_ptr<Radio> radio,
		std::vector<Track> tracks,
		Fn<bool()> valid) {
	const auto seed = radio->seed;
	if (IsTelegramTrack(seed)
		&& PlayingTrack().id != seed.id
		&& !TrackMessage(seed)) {
		FailRadio();
		return;
	}
	if (PlayingTrack().id == seed.id || IsTelegramTrack(seed)) {
		CommitRadio(radio, tracks, seed, {});
		return;
	}
	Resolve(seed, [=](Track resolved, QString error) {
		if (!valid()) {
			return;
		}
		if (!error.isEmpty()) {
			FailRadio();
			return;
		}
		Download(resolved, [=](const QString &path, const QString &error) {
			if (!valid()) {
				return;
			}
			if (!error.isEmpty()) {
				FailRadio();
				return;
			}
			CommitRadio(radio, tracks, resolved, path);
		});
	});
}

void StartRadio(
		not_null<Window::SessionController*> controller,
		Track seed) {
	BindController(controller);
	if ((!IsTelegramTrack(seed) && !seed.id.toULongLong())
		|| IsArtistBlocked(seed)) {
		return;
	}
	auto &queue = QueueState();
	const auto radio = std::make_shared<Radio>();
	radio->seed = seed;
	radio->sourceId = IsTelegramTrack(seed) ? QString() : seed.id;
	RememberRadioTrack(*radio, seed);
	queue.pendingRadio = radio;
	const auto session = queue.session;
	const auto token = queue.pendingToken;
	const auto playback = Media::Player::instance()->current(AudioMsgId::Type::Song);
	const auto valid = [=] {
		return session && QueueState().session.get() == session.get()
			&& QueueState().pendingRadio == radio
			&& QueueState().pendingToken == token
			&& Media::Player::instance()->current(AudioMsgId::Type::Song) == playback;
	};
	controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
		"Загружаю радио по треку: ")) + seed.title);
	const auto fetch = [=] {
		FetchRadioBatch(radio, valid, [=](std::vector<Track> tracks, QString error) {
			const auto controller = QueueState().controller.get();
			if (!controller) {
				return;
			}
			if (!error.isEmpty() || tracks.empty()) {
				QueueState().pendingRadio.reset();
				controller->showToast(error.isEmpty()
					? QString::fromUtf8(Mokagram::Lang::Tr(
						"Нет новых подходящих треков для радио."))
					: error);
				return;
			}
			PrepareRadioSeed(radio, std::move(tracks), valid);
		});
	};
	if (!IsTelegramTrack(seed)) {
		fetch();
		return;
	}
	const auto metadata = MotionProtocol::Metadata(seed.title, seed.artist, false);
	Search(metadata.artist + u" "_q + metadata.title, 0, [=](SearchResult result) {
		if (!valid()) {
			return;
		}
		for (const auto &candidate : result.tracks) {
			const auto match = MotionProtocol::Metadata(
				candidate.title, candidate.artist, true);
			const auto hasDuration = seed.durationMs > 0 && candidate.durationMs > 0;
			if (candidate.id.toULongLong() && !IsArtistBlocked(candidate)
				&& MotionProtocol::MatchesTrack(
					metadata.title, metadata.artist, hasDuration ? seed.durationMs : 1,
					match.title, match.artist, hasDuration ? candidate.durationMs : 1)) {
				radio->sourceId = candidate.id;
				RememberRadioTrack(*radio, candidate);
				fetch();
				return;
			}
		}
		QueueState().pendingRadio.reset();
		if (const auto controller = QueueState().controller.get()) {
			controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr(
				result.error.isEmpty()
					? Mokagram::Lang::Tr("Не найден подходящий трек для радио.")
					: Mokagram::Lang::Tr("Не удалось загрузить радио. Попробуйте ещё раз."))));
		}
	});
}

void AddRadioAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		const Track &track) {
	const auto supported = IsTelegramTrack(track)
		? (!track.title.trimmed().isEmpty() && !track.artist.trimmed().isEmpty())
		: (track.id.toULongLong() != 0);
	const auto text = !supported
		? Mokagram::Lang::Tr("Радио недоступно для треков Telegram")
		: IsArtistBlocked(track)
		? Mokagram::Lang::Tr("Радио недоступно: автор скрыт")
		: Mokagram::Lang::Tr("Радио по треку");
	const auto weak = base::make_weak(controller);
	menu->addAction(QString::fromUtf8(text), [=] {
		if (const auto controller = weak.get()) {
			StartRadio(controller, track);
		}
	}, &st::menuIconSoundOn)->setEnabled(supported && !IsArtistBlocked(track));
}

// Дозагрузка очереди с учётом того, что играет сейчас и что недавно было.
void RefillStream() {
	auto &queue = QueueState();
	if (queue.radio) {
		RefillRadio();
		return;
	}
	if (!queue.stream || queue.refilling || !StreamEnabled()) {
		return;
	}
	queue.refilling = true;
	const auto epoch = queue.streamEpoch;
	const auto videoId = queue.current.videoId;
	const auto finish = [=](std::vector<Track> tracks, QString note) {
		auto &queue = QueueState();
		if (epoch != queue.streamEpoch) {
			return;
		}
		queue.refilling = false;
		for (auto &t : tracks) {
			queue.autoIds.insert(t.id);
			queue.tracks.push_back(std::move(t));
		}
		queue.changes.fire({});
		PrefetchUpcoming();
		if (base::take(queue.waiting)
			&& (videoId.isEmpty() || UsesQueue(Media::Player::instance()->current(
				AudioMsgId::Type::Song)))) {
			if (queue.index + 1 < int(queue.tracks.size())) {
				PlayIndex(queue.index + 1, !videoId.isEmpty());
			} else if (const auto controller = queue.controller.get(); controller && !note.isEmpty()) {
				controller->showToast(note);
			}
		}
	};
	if (!videoId.isEmpty()) {
		LoadYouTubeMusicRecommendations(videoId, [=](std::vector<Track> tracks) {
			auto &queue = QueueState();
			if (epoch != queue.streamEpoch) {
				return;
			}
			if (!StreamEnabled()) {
				queue.refilling = false;
				queue.waiting = false;
				return;
			}
			auto seen = std::set<QString>();
			for (const auto &track : queue.tracks) {
				const auto keys = TrackIdentityKeys(track);
				seen.insert(keys.begin(), keys.end());
			}
			auto picked = std::vector<Track>();
			for (auto &track : tracks) {
				const auto keys = TrackIdentityKeys(track);
				if (track.videoId == videoId || IsArtistBlocked(track)
					|| std::ranges::any_of(keys, [&](const QString &key) {
						return seen.contains(key);
					})) {
					continue;
				}
				seen.insert(keys.begin(), keys.end());
				picked.push_back(std::move(track));
				if (picked.size() == 12) {
					break;
				}
			}
			finish(std::move(picked), QString::fromUtf8(Mokagram::Lang::Tr(
				"Не нашлось подходящих треков. Попробуйте позже.")));
		});
		return;
	}
	auto context = StreamContext();
	context.current = queue.current;
	const auto from = std::max(0, queue.index - 4);
	for (auto i = from; i < int(queue.tracks.size()); ++i) {
		if (i != queue.index) {
			context.avoid.push_back(queue.tracks[i]);
		}
	}
	BuildStream(std::move(context), 12, finish);
}

// Три следующих трека готовим заранее: переход на них будет без загрузки.
// Telegram-треки стримит сам плеер, их не трогаем.
void PrefetchUpcoming() {
	const auto &queue = QueueState();
	const auto token = queue.pendingToken;
	for (auto i = queue.index + 1, left = 3
		; left > 0 && i < int(queue.tracks.size())
		; ++i) {
		const auto &track = queue.tracks[i];
		if (!CanPlayTrack(track) || IsTelegramTrack(track)) {
			continue;
		}
		if (track.unavailable) {
			// Феллбек из YouTube Music кэшируется так же, как треки SoundCloud.
			DownloadYouTubeMusic(track, [](const QString &, const QString &) {});
			--left;
			continue;
		}
		if (!track.id.toULongLong()) {
			continue;
		}
		--left;
		ResolveThen(track, [=](Track next, QString error) {
			if (error.isEmpty() && token == QueueState().pendingToken) {
				Download(next, [](const QString &, const QString &) {});
			}
		});
	}
}

void Subscribe() {
	auto &queue = QueueState();
	if (queue.subscribed) {
		return;
	}
	queue.subscribed = true;
	Presence::Publish([] {
		const auto &queue = QueueState();
		const auto state = Media::Player::instance()->getState(
			AudioMsgId::Type::Song);
		if (state.state != Media::Player::State::Playing
			|| queue.current.id != queue.currentId
			|| !UsesQueue(state.id)
			|| !CanForwardTrack(queue.current)) {
			return Presence::Playback();
		}
		return Presence::Playback{
			.session = queue.session.get(),
			.title = queue.current.title,
			.artist = queue.current.artist,
			.soundcloudId = queue.current.id.toULongLong(),
		};
	}, queue.lifetime);
	Media::Player::instance()->updatedNotifier(
	) | rpl::on_next([=](const Media::Player::TrackState &state) {
		auto &queue = QueueState();
		if (queue.currentId.isEmpty() || !UsesQueue(state.id)) {
			return;
		}
		if (Media::Player::IsStoppedAtEnd(state.state)) {
			RecordCurrentListen(true);
			const auto count = int(queue.tracks.size());
			const auto &settings = Core::App().settings();
			if (settings.playerRepeatMode() == Media::RepeatMode::One) {
				PlayIndex(queue.index);
			} else if (base::options::value<bool>(
					Media::Player::kOptionDisableAutoplayNext)
				|| !Mokagram::Settings::Bool(u"player.autoplay_next"_q)) {
				return;
			} else if (queue.radio || queue.stream) {
				if (queue.index + 1 < count) {
					PlayIndex(queue.index + 1, true);
				} else {
					queue.waiting = true;
					RefillStream();
				}
			} else if (settings.playerOrderMode() == Media::OrderMode::Shuffle
				&& count > 1) {
				auto next = queue.index;
				while (next == queue.index) {
					next = base::RandomIndex(count);
				}
				PlayIndex(next);
			} else if (queue.index + 1 < count) {
				PlayIndex(queue.index + 1, true);
			} else if (settings.playerRepeatMode() == Media::RepeatMode::All
				&& count > 0) {
				PlayIndex(0);
			} else if (StreamEnabled()) {
				// Вечная очередь продолжает даже один трек из EP или локальный трек.
				queue.stream = true;
				queue.waiting = true;
				RefillStream();
			}
		}
	}, queue.lifetime);
}

void PlayIndex(int index, bool skipUnavailable) {
	auto &queue = QueueState();
	const auto controller = queue.controller.get();
	if (!controller || index < 0 || index >= int(queue.tracks.size())) {
		return;
	}
	Subscribe();
	const auto previousArtist = queue.current.artistId;
	const auto skippedEarly = RecordCurrentListen(false);
	if (skippedEarly && queue.stream && !previousArtist.isEmpty()) {
		// Пропуск в первые секунды: убираем автора из предстоящих авто-треков.
		DropUpcoming([=](const Track &t) {
			return t.artistId == previousArtist;
		}, std::max(queue.index, index), true);
	}
	const auto track = queue.tracks[index];
	if (track.videoId.isEmpty() != queue.current.videoId.isEmpty()) {
		++queue.streamEpoch;
		queue.refilling = false;
	}
	queue.waiting = false;
	const auto token = ++queue.pendingToken;
	queue.index = index;
	queue.currentId = track.id;
	queue.changes.fire({});
	if (IsTelegramTrack(track)) {
		const auto item = TrackMessage(track);
		const auto document = item && item->media()
			? item->media()->document()
			: nullptr;
		if (!document) {
			controller->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Трек больше недоступен")));
			return;
		}
		queue.current = track;
		queue.recorded = true;
		Media::Player::instance()->play(
			AudioMsgId(document, item->fullId()),
			std::nullopt);
		PrefetchUpcoming();
		return;
	}
	const auto playDocument = [](not_null<DocumentData*> document, const Track &resolved) {
		auto &queue = QueueState();
		LoadQueuePlaybackCover(document->id, resolved);
		queue.current = resolved;
		queue.recorded = false;
		Media::Player::instance()->play(AudioMsgId(document, FullMsgId()), std::nullopt);
		RecordRecentTrack(resolved);
		if ((queue.radio || queue.stream)
			&& int(queue.tracks.size()) - queue.index - 1 <= 3) {
			RefillStream();
		}
		PrefetchUpcoming();
	};
	if (track.unavailable) {
		const auto playback = Media::Player::instance()->current(AudioMsgId::Type::Song);
		const auto valid = [=] {
			const auto &queue = QueueState();
			return queue.controller && queue.session && token == queue.pendingToken
				&& queue.index >= 0 && queue.index < int(queue.tracks.size())
				&& queue.tracks[queue.index].id == track.id
				&& Media::Player::instance()->current(AudioMsgId::Type::Song) == playback;
		};
		DownloadYouTubeMusic(track, [=](const QString &path, const QString &error) {
			if (!valid()) return;
			auto &queue = QueueState();
			const auto controller = queue.controller.get();
			if (!path.isEmpty()) {
				playDocument(MakeDocument(&controller->session(), track, path), track);
			} else if (skipUnavailable && queue.index + 1 < int(queue.tracks.size())) {
				crl::on_main(controller, [=] {
					if (valid()) PlayIndex(QueueState().index + 1, true);
				});
			} else {
				controller->showToast(error);
			}
		});
		return;
	}
	ResolveThen(track, [=](Track resolved, QString error) {
		auto &queue = QueueState();
		const auto controller = queue.controller.get();
		if (!controller || token != queue.pendingToken
			|| (queue.radio && IsArtistBlocked(track))) {
			return;
		}
		if (!error.isEmpty()) {
			controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить: ")) + error);
			return;
		}
		if (index < int(queue.tracks.size()) && queue.tracks[index].id == resolved.id) {
			queue.tracks[index] = resolved;
		}
		Download(resolved, [=](const QString &path, const QString &error) {
			auto &queue = QueueState();
			const auto controller = queue.controller.get();
			if (!controller || token != queue.pendingToken
				|| (queue.radio && IsArtistBlocked(resolved))) {
				return;
			}
			if (!error.isEmpty()) {
				controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить: ")) + error);
				return;
			}
			playDocument(MakeDocument(&controller->session(), resolved, path), resolved);
		});
	});
}


[[nodiscard]] QString SafeName(const Track &track) {
	auto name = track.artist + u" - "_q + track.title;
	name.replace(QRegularExpression(u"[\\\\/:*?\"<>|]"_q), u"_"_q);
	return name.left(120).trimmed();
}

// Копия трека с понятным именем: оно уйдёт в Telegram как название файла.
[[nodiscard]] QString NamedCopy(const Track &track, const QString &path) {
	const auto dir = CacheDir() + u"/send"_q;
	QDir().mkpath(dir);
	const auto named = dir + '/' + SafeName(track) + '.' + track.extension;
	QFile::remove(named);
	QFile::copy(path, named);
	return named;
}

void EnsureFile(
		const Track &track,
		Fn<void(const QString &, const Track &)> done) {
	const auto controller = QueueState().controller.get();
	ResolveThen(track, [=](Track resolved, QString error) {
		if (!error.isEmpty()) {
			if (const auto controller = QueueState().controller.get()) {
				controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить: ")) + error);
			}
			return;
		}
		Download(resolved, [=](const QString &path, const QString &error) {
			if (!error.isEmpty()) {
				if (const auto controller = QueueState().controller.get()) {
					controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось загрузить: ")) + error);
				}
				return;
			}
			auto saved = resolved;
			saved.extension = QFileInfo(path).suffix();
			done(path, saved);
		});
	});
	if (controller) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Загружаю: ")) + track.title);
	}
}

void UploadTrack(
		not_null<Data::Thread*> thread,
		const Track &track,
		const QString &path,
		const QImage &cover) {
	const auto session = &thread->owningHistory()->session();
	auto list = Storage::PrepareMediaList(
		QStringList{ NamedCopy(track, path) },
		st::sendMediaPreviewSize,
		session->user()->isPremium());
	if (list.error != Ui::PreparedList::Error::None || list.files.empty()) {
		return;
	}
	auto &file = list.files.front();
	auto &information = *file.information;
	auto song = std::get_if<Ui::PreparedFileInformation::Song>(
		&information.media);
	if (!song) {
		information.media = Ui::PreparedFileInformation::Song();
		song = std::get_if<Ui::PreparedFileInformation::Song>(
			&information.media);
	}
	if (song->duration <= 0) {
		song->duration = track.durationMs;
	}
	if (!track.title.isEmpty()) {
		song->title = track.title;
	}
	if (!track.artist.isEmpty()) {
		song->performer = track.artist;
	}
	if (!cover.isNull()) {
		song->cover = cover;
	}
	file.type = Ui::PreparedFile::Type::Music;
	auto action = Api::SendAction(thread);
	action.clearDraft = false;
	session->api().sendFiles(
		std::move(list),
		SendMediaType::File,
		nullptr,
		std::move(action));
}

void SendTrack(
		not_null<Data::Thread*> thread,
		const Track &track,
		const QString &path) {
	const auto cover = CoverForDocument(DocumentIdForTrack(track.id));
	if (!cover.isNull() || track.artworkUrl.isEmpty()) {
		UploadTrack(thread, track, path, cover);
		return;
	}
	const auto weak = base::make_weak(thread);
	LoadImage(ArtUrl(track.artworkUrl, u"t500x500"_q), [=](QImage image) {
		if (const auto thread = weak.get()) {
			UploadTrack(thread, track, path, image);
		}
	});
}

void SaveToDisk(const Track &track) {
	EnsureFile(track, [=](const QString &path, const Track &track) {
		auto dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
		if (dir.isEmpty()) {
			dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		}
		QDir().mkpath(dir);
		const auto target = dir + '/' + SafeName(track) + '.' + track.extension;
		QFile::remove(target);
		const auto controller = QueueState().controller.get();
		if (QFile::copy(path, target)) {
			if (controller) {
				controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Сохранено: ")) + target, 4000);
			}
		} else if (controller) {
			controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Не удалось сохранить файл")));
		}
	});
}

void ForwardTrack(
		not_null<Window::SessionController*> controller,
		const Track &track) {
	if (IsTelegramTrack(track)) {
		const auto item = TrackMessage(track);
		if (item && item->allowsForward()) {
			Window::ShowForwardMessagesBox(
				controller,
				MessageIdsList{ item->fullId() });
		}
		return;
	}
	Window::ShowChooseRecipientBox(
		controller,
		[=](not_null<Data::Thread*> thread) {
			const auto weak = base::make_weak(thread);
			EnsureFile(track, [=](const QString &path, const Track &track) {
				if (const auto thread = weak.get()) {
					SendTrack(thread, track, path);
				}
			});
			return true;
		},
		rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Переслать трек"))));
}

// Профильная музыка Telegram хранится в документах: заливаем файл в «Избранное»
// и, когда он загрузился, сохраняем в профиль.
void AddToProfile(
		not_null<Window::SessionController*> controller,
		const Track &track) {
	const auto session = &controller->session();
	const auto weak = base::make_weak(session);
	EnsureFile(track, [=](const QString &path, const Track &track) {
		const auto strong = weak.get();
		if (!strong) {
			return;
		}
		const auto history = strong->data().history(strong->user());
		SendTrack(history, track, path);
		const auto name = SafeName(track) + '.' + track.extension;
		const auto timer = new QTimer();
		const auto tries = std::make_shared<int>(0);
		timer->setInterval(1500);
		QObject::connect(timer, &QTimer::timeout, [=] {
			const auto session = weak.get();
			const auto finish = [&] {
				timer->stop();
				timer->deleteLater();
			};
			if (!session || ++*tries > 80) {
				finish();
				return;
			}
			const auto history = session->data().history(session->user());
			for (const auto &block : history->blocks) {
				for (const auto &view : block->messages) {
					const auto item = view->data();
					const auto document = item->media()
						? item->media()->document()
						: nullptr;
					if (!document
						|| item->isSending()
						|| item->hasFailed()
						|| document->filename() != name) {
						continue;
					}
					session->data().savedMusic().save(
						document,
						Data::FileOrigin(item->fullId()));
					if (const auto controller = QueueState().controller.get()) {
						controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Добавлено в профиль: ")) + track.title);
					}
					finish();
					return;
				}
			}
		});
		timer->start();
	});
}


// Клик по треку из результатов поиска.
[[nodiscard]] std::vector<Track> &ContextTracks() {
	static auto result = std::vector<Track>();
	return result;
}

void PlayFromResults(int resultIndex);

// Трек из списка чата, откуда играет Telegram: запускаем его уже внутри очереди плеера.
void PlayContext(int index) {
	auto &queue = QueueState();
	if (index < 0 || index >= int(ContextTracks().size())) {
		return;
	}
	queue.results = ContextTracks();
	PlayFromResults(index);
}

void PlayFromResults(int resultIndex) {
	auto &queue = QueueState();
	if (resultIndex < 0 || resultIndex >= int(queue.results.size())) {
		return;
	}
	const auto track = queue.results[resultIndex];
	if (!queue.custom) {
		queue.radio.reset();
		queue.pendingRadio.reset();
		queue.tracks = queue.results;
		queue.album = {};
		queue.stream = StreamEnabled();
		++queue.streamEpoch;
		queue.refilling = false;
		queue.waiting = false;
		queue.autoIds.clear();
		if (queue.stream) {
			for (const auto &t : queue.tracks) {
				queue.autoIds.insert(t.id);
			}
		}
		PlayIndex(resultIndex);
		return;
	}
	for (auto i = 0; i != int(queue.tracks.size()); ++i) {
		if (queue.tracks[i].id == track.id) {
			PlayIndex(i);
			return;
		}
	}
	const auto at = std::min<int>(queue.index + 1, queue.tracks.size());
	queue.tracks.insert(queue.tracks.begin() + at, track);
	PlayIndex(at);
}

void EnqueueNext(const Track &track) {
	auto &queue = QueueState();
	queue.custom = true;
	const auto at = std::min<int>(queue.index + 1, queue.tracks.size());
	queue.tracks.insert(queue.tracks.begin() + at, track);
	queue.changes.fire({});
}

void EnqueueLast(const Track &track) {
	auto &queue = QueueState();
	queue.custom = true;
	queue.tracks.push_back(track);
	queue.changes.fire({});
}

void QueueMove(int index, int delta) {
	auto &queue = QueueState();
	const auto target = index + delta;
	if (index < 0 || index >= int(queue.tracks.size())
		|| target < 0 || target >= int(queue.tracks.size())) {
		return;
	}
	base::reorder(queue.tracks, index, target);
	if (queue.index == index) {
		queue.index = target;
	} else if (index < queue.index && queue.index <= target) {
		--queue.index;
	} else if (target <= queue.index && queue.index < index) {
		++queue.index;
	}
	queue.custom = true;
	queue.changes.fire({});
}

void QueueRemove(int index) {
	auto &queue = QueueState();
	if (index < 0 || index >= int(queue.tracks.size())) {
		return;
	}
	queue.tracks.erase(queue.tracks.begin() + index);
	if (queue.index > index) {
		--queue.index;
	} else if (queue.index == index) {
		queue.index = -1;
	}
	queue.custom = true;
	queue.changes.fire({});
}

// Сердце из Ionicons (MIT): ion:heart и ion:heart-outline, viewBox 512x512.
void DrawHeart(QPainter &p, const QRectF &r, bool filled, const QColor &color) {
	static auto cache = std::map<std::pair<bool, QRgb>, std::unique_ptr<QSvgRenderer>>();
	auto &renderer = cache[{ filled, color.rgba() }];
	if (!renderer) {
		const auto paint = color.name(QColor::HexRgb).toLatin1();
		const auto opacity = QByteArray::number(color.alphaF(), 'f', 3);
		const auto shape = filled
			? QByteArray("<path fill=\"") + paint + "\" fill-opacity=\"" + opacity
				+ "\" d=\"M256 448a32 32 0 0 1-18-5.57c-78.59-53.35-112.62-89.93-131.39-112.8c-40-48.75-59.15-98.8-58.61-153C48.63 114.52 98.46 64 159.08 64c44.08 0 74.61 24.83 92.39 45.51a6 6 0 0 0 9.06 0C278.31 88.81 308.84 64 352.92 64c60.62 0 110.45 50.52 111.08 112.64c.54 54.21-18.63 104.26-58.61 153c-18.77 22.87-52.8 59.45-131.39 112.8a32 32 0 0 1-18 5.56\"/>"
			: QByteArray("<path fill=\"none\" stroke=\"") + paint + "\" stroke-opacity=\"" + opacity
				+ "\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"32\" d=\"M352.92 80C288 80 256 144 256 144s-32-64-96.92-64c-52.76 0-94.54 44.14-95.08 96.81c-1.1 109.33 86.73 187.08 183 252.42a16 16 0 0 0 18 0c96.26-65.34 184.09-143.09 183-252.42c-.54-52.67-42.32-96.81-95.08-96.81\"/>";
		renderer = std::make_unique<QSvgRenderer>(
			QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 512 512\">")
			+ shape + "</svg>");
	}
	const auto side = std::min(r.width(), r.height());
	renderer->render(&p, QRectF(
		r.center().x() - side / 2,
		r.center().y() - side / 2,
		side,
		side));
}

[[nodiscard]] const QColor &HeartColor() {
	static const auto color = QColor(255, 77, 109);
	return color;
}

constexpr auto kLikeDuration = crl::time(650);

[[nodiscard]] bool LikeAnimationsEnabled() {
	return !PowerSaving::On(PowerSaving::kAnimations);
}

// Масштаб сердца по ходу анимации лайка: затухающая пружина к 1.
[[nodiscard]] float64 LikeScale(float64 progress, bool liked) {
	const auto from = liked ? 0.4 : 0.75;
	return 1.
		- (1. - from) * std::exp(-7. * progress) * std::cos(14. * progress);
}

// Вспышка вокруг сердца при лайке: расходящееся кольцо и искры.
void DrawLikeBurst(
		QPainter &p,
		QPointF center,
		float64 radius,
		float64 progress,
		const QColor &color) {
	const auto easeOut = [](float64 t) {
		return 1. - std::pow(1. - t, 3);
	};
	p.save();
	if (progress < 0.45) {
		const auto t = progress / 0.45;
		auto ring = color;
		ring.setAlphaF(0.9 * (1. - t));
		const auto width = std::max(radius * 0.3 * (1. - t), 0.5);
		const auto r = radius * (0.35 + 0.6 * easeOut(t)) - width / 2.;
		p.setPen(QPen(ring, width));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(center, r, r);
	}
	const auto t = std::clamp((progress - 0.12) / 0.6, 0., 1.);
	if (t > 0. && t < 1.) {
		static const auto spark = QColor(255, 179, 71);
		const auto distance = radius * (0.55 + 0.35 * easeOut(t));
		const auto size = radius * 0.12 * (1. - t);
		constexpr auto kCount = 7;
		p.setPen(Qt::NoPen);
		for (auto i = 0; i != kCount; ++i) {
			const auto angle = -M_PI / 2. + i * 2. * M_PI / kCount;
			const auto big = center
				+ QPointF(std::cos(angle), std::sin(angle)) * distance;
			const auto side = angle + 0.35;
			const auto small = center
				+ QPointF(std::cos(side), std::sin(side)) * distance * 0.8;
			p.setBrush(color);
			p.drawEllipse(big, size, size);
			p.setBrush(spark);
			p.drawEllipse(small, size * 0.7, size * 0.7);
		}
	}
	p.restore();
}

class TabPill final : public Ui::AbstractButton {
public:
	TabPill(QWidget *parent, const QString &text)
	: AbstractButton(parent)
	, _text(text) {
		resize(width(), 40);
		setPointerCursor(true);
		paintRequest(
		) | rpl::on_next([=](QRect) {
			// Плашка и дорожка рисуются родителем и едут между вкладками,
			// здесь только подпись.
			auto p = QPainter(this);
			const auto progress = _fade.value(_active ? 1. : 0.);
			p.setFont(st::semiboldFont);
			p.setPen(anim::color(
				isOver() ? st::windowFg->c : st::windowSubTextFg->c,
				st::windowFg->c,
				progress));
			p.drawText(rect(), Qt::AlignCenter, _text);
		}, lifetime());
	}
	void setActive(bool active) {
		if (_active != active) {
			_active = active;
			_fade.start(
				[=] { update(); },
				_active ? 0. : 1.,
				_active ? 1. : 0.,
				kSwitchDuration,
				anim::easeOutCubic);
		}
	}

protected:
	void onStateChanged(State was, StateChangeSource source) override {
		AbstractButton::onStateChanged(was, source);
		update();
	}

private:
	QString _text;
	Ui::Animations::Simple _fade;
	bool _active = false;

};

class SourceSwitch final : public Ui::AbstractButton {
public:
	SourceSwitch(QWidget *parent, QString first, QString second)
	: AbstractButton(parent)
	, _labels{ std::move(first), std::move(second) } {
	resize(width(), 40);
	setPointerCursor(true);
	setClickedCallback([=] {
		const auto chats = mapFromGlobal(QCursor::pos()).x() >= width() / 2;
		if (_chats != chats && _changed) {
			_changed(chats);
		}
	});
	paintRequest(
	) | rpl::on_next([=](QRect) {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(st::windowBgOver);
		p.drawRoundedRect(rect(), height() / 2., height() / 2.);
		const auto margin = 4;
		const auto half = (width() - 2 * margin) / 2;
		const auto progress = _animation.value(_chats ? 1. : 0.);
		// Плашка едет к выбранной половине и слегка вытягивается на ходу.
		const auto stretch = int(base::SafeRound(
			half * 0.18 * (1. - std::abs(2. * progress - 1.))));
		const auto left = margin + int(base::SafeRound(half * progress));
		const auto selected = QRect(
			left - stretch / 2,
			margin,
			half + stretch,
			height() - 2 * margin);
		p.setBrush(st::windowActiveTextFg->c);
		p.drawRoundedRect(selected, selected.height() / 2., selected.height() / 2.);
		p.setFont(st::semiboldFont);
		for (auto i = 0; i != 2; ++i) {
			const auto active = (i == 1) ? progress : (1. - progress);
			p.setPen(anim::color(st::windowSubTextFg, st::windowFg, active));
			p.drawText(QRect(i * width() / 2, 0, width() / 2, height()), Qt::AlignCenter, _labels[i]);
		}
	}, lifetime());
	}
	void setChats(bool chats) {
		if (_chats != chats) {
			_chats = chats;
			_animation.start(
				[=] { update(); },
				_chats ? 0. : 1.,
				_chats ? 1. : 0.,
				kSwitchDuration,
				anim::easeOutCubic);
		}
	}
	void setChangedCallback(Fn<void(bool)> callback) {
		_changed = std::move(callback);
	}
	void finishAnimating() {
		_animation.stop();
		update();
	}

private:
	const std::array<QString, 2> _labels;
	Ui::Animations::Simple _animation;
	Fn<void(bool)> _changed;
	bool _chats = false;

};

class PicksBar final : public Ui::AbstractButton {
public:
	PicksBar(QWidget *parent, Fn<void()> play, Fn<void()> refresh)
	: AbstractButton(parent) {
		resize(width(), 36);
		setPointerCursor(true);
		setClickedCallback([=] {
			if (refreshRect().contains(mapFromGlobal(QCursor::pos()))) {
				refresh();
			} else {
				play();
			}
		});
		paintRequest(
		) | rpl::on_next([=](QRect) {
			auto p = QPainter(this);
			// Только текст, без значков и плашек.
			const auto onRefresh = refreshRect().contains(
				mapFromGlobal(QCursor::pos()));
			p.setFont(st::semiboldFont);
			p.setPen(onRefresh || !isOver()
				? st::windowActiveTextFg
				: st::windowFg);
			p.drawText(
				QRect(8, 0, width() - 8 - 110, height()),
				Qt::AlignVCenter | Qt::AlignLeft,
				QString::fromUtf8(Mokagram::Lang::Tr("Слушать подборку")));
			p.setFont(st::normalFont);
			p.setPen(onRefresh ? st::windowFg : st::windowSubTextFg);
			p.drawText(
				refreshRect(),
				Qt::AlignVCenter | Qt::AlignRight,
				QString::fromUtf8(Mokagram::Lang::Tr("Обновить")));
		}, lifetime());
	}

private:
	[[nodiscard]] QRect refreshRect() const {
		return QRect(width() - 110, 0, 110 - 8, height());
	}

};

[[nodiscard]] bool IsLightMusicTheme() {
	return Ui::IsLightBackground(st::boxBg->c);
}

[[nodiscard]] QColor MusicSecondaryColor() {
	return IsLightMusicTheme()
		? anim::color(st::windowSubTextFg->c, st::windowFg->c, .45)
		: st::windowSubTextFg->c;
}

[[nodiscard]] QColor MusicSelectionColor() {
	return anim::color(st::windowBgOver->c, st::windowBgActive->c, .30);
}

[[nodiscard]] QColor MusicCardColor() {
	return IsLightMusicTheme()
		? anim::color(st::windowBg->c, st::windowBgOver->c, .65)
		: st::windowBg->c;
}

// Карточка SoundCloud: вход и обновление лайков или плейлистов.
class SoundCloudAccountBar final : public Ui::AbstractButton {
public:
	SoundCloudAccountBar(
			QWidget *parent,
			Fn<void()> login,
			Fn<void()> logout,
			bool playlists = false)
	: AbstractButton(parent)
	, _login(std::move(login))
	, _logout(std::move(logout))
	, _playlists(playlists) {
		// Высоту задаёт раскладка: resize() в конструкторе дал бы нулевую
		// высоту и кнопка не ловила бы клики.
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		_refreshButton = Ui::CreateChild<Ui::RoundButton>(
			this,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Обновить"))),
			actionStyle());
		_disconnectButton = Ui::CreateChild<Ui::RoundButton>(
			this,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Отключить"))),
			actionStyle());
		_disconnectButton->setClickedCallback(_logout);
		_refreshButton->setClickedCallback([=] {
			if (_syncing || !Mokagram::Music::Account::IsLoggedIn()) {
				return;
			}
			_syncing = true;
			_syncNote = QString::fromUtf8(Mokagram::Lang::Tr("Обновляем…"));
			_refreshButton->setDisabled(true);
			refresh();
			const auto finish = crl::guard(this, [=](int, int, QString error) {
					_syncing = false;
					_syncNote = std::move(error);
					_refreshButton->setDisabled(false);
					refresh();
				});
			if (_playlists) {
				Mokagram::Music::Account::SyncPlaylists(finish);
			} else {
				Mokagram::Music::Account::SyncLikes(finish);
			}
		});
		refresh();
		setClickedCallback([=] {
			if (!Mokagram::Music::Account::IsLoggedIn()) {
				_login();
			}
		});
		Mokagram::Music::Account::Changes() | rpl::on_next([=] {
			refresh();
		}, lifetime());
		if (_playlists) {
			PlaylistsChanged() | rpl::on_next([=] {
				refresh();
			}, lifetime());
		} else {
			LikesChanged() | rpl::on_next([=] {
				refresh();
			}, lifetime());
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto buttonWidth = (newWidth - s(36)) / 2;
		_refreshButton->setFullWidth(buttonWidth);
		_disconnectButton->setFullWidth(buttonWidth);
		_refreshButton->move(s(14), s(68));
		_disconnectButton->move(newWidth - s(14) - buttonWidth, s(68));
		return s(_logged ? 122 : 72);
	}

	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto r = rect();
		p.setPen(Qt::NoPen);
		p.setBrush(_logged || !isOver() ? MusicCardColor() : st::windowBgOver->c);
		p.drawRoundedRect(r, s(22), s(22));
		if (hasFocus()) {
			p.setPen(QPen(st::windowActiveTextFg->c, s(2)));
			p.setBrush(Qt::NoBrush);
			p.drawRoundedRect(r.adjusted(s(2), s(2), -s(2), -s(2)), s(20), s(20));
		}
		const auto badge = QRect(
			s(14),
			s(14),
			s(44),
			s(44));
		if (_avatar.isNull()) {
			Mokagram::Music::PaintSoundCloudBadge(p, badge);
		} else {
			// Аватарка профиля кругом, фирменный значок маленьким в углу.
			p.save();
			auto clip = QPainterPath();
			clip.addEllipse(badge);
			p.setClipPath(clip);
			p.drawImage(badge, _avatar);
			p.restore();
			const auto mark = s(20);
			const auto markRect = QRect(
				badge.right() - mark + s(4),
				badge.bottom() - mark + s(4),
				mark,
				mark);
			p.setPen(Qt::NoPen);
			p.setBrush(MusicCardColor());
			p.drawEllipse(markRect.adjusted(-s(2), -s(2), s(2), s(2)));
			Mokagram::Music::PaintSoundCloudBadge(p, markRect);
		}

		auto titleFont = st::semiboldFont->f;
		titleFont.setPixelSize(s(15));
		p.setFont(titleFont);
		p.setPen(st::windowFg);
		const auto textLeft = badge.right() + s(14);
		const auto textRight = width() - s(16);
		p.drawText(
			QRect(textLeft, s(15), textRight - textLeft, s(20)),
			Qt::AlignVCenter | Qt::AlignLeft,
			_logged
				? QString::fromUtf8(Mokagram::Lang::Tr("SoundCloud"))
				: QString::fromUtf8(Mokagram::Lang::Tr("Подключить SoundCloud")));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		const auto noteFont = QFontMetrics(st::normalFont->f);
		p.drawText(
			QRect(textLeft, s(37), textRight - textLeft, s(18)),
			Qt::AlignVCenter | Qt::AlignLeft,
			noteFont.elidedText(
				_note,
				Qt::ElideRight,
				textRight - textLeft));
	}

private:
	static const style::RoundButton &actionStyle() {
		static const auto result = [] {
			auto result = Mokagram::BoxParts::PillLight();
			result.textBg = st::windowBgOver;
			result.textBgOver = st::windowBgRipple;
			result.textFg = st::windowFg;
			result.textFgOver = st::windowFg;
			return result;
		}();
		return result;
	}

	void refresh() {
		_logged = Mokagram::Music::Account::IsLoggedIn();
		const auto info = Mokagram::Music::Account::User();
		const auto playlists = Playlists();
		const auto count = _playlists
			? int(std::count_if(playlists.begin(), playlists.end(), [](const Playlist &p) {
				return p.remote;
			}))
			: int(LikedTracks().size());
		if (!_logged) {
			_note = QString::fromUtf8(Mokagram::Lang::Tr(
				_playlists
					? "Плейлисты из аккаунта SoundCloud"
					: "Лайки и рекомендации из аккаунта"));
		} else {
			const auto who = info.username.isEmpty()
				? QString::fromUtf8(Mokagram::Lang::Tr("Аккаунт подключён"))
				: (u"@"_q + info.username);
			const auto countText = _playlists
				? QString::fromUtf8(Mokagram::Lang::Tr("плейлистов: %1"))
				: QString::fromUtf8(Mokagram::Lang::Tr("лайков: %1"));
			_note = (count > 0)
				? (who + u" · "_q + countText.arg(count))
				: who;
		}
		if (_logged
			&& info.avatarUrl.contains(u"sndcdn.com"_q)
			&& info.avatarUrl != _avatarUrl) {
			_avatarUrl = info.avatarUrl;
			LoadImage(ArtUrl(_avatarUrl, u"t200x200"_q), crl::guard(this, [=](QImage image) {
				_avatar = std::move(image);
				update();
			}));
		} else if (!_logged) {
			_avatarUrl = QString();
			_avatar = QImage();
		}
		if (!_syncNote.isEmpty() && _logged) {
			_note = _syncNote;
		}
		_refreshButton->setVisible(_logged);
		_disconnectButton->setVisible(_logged);
		resizeToWidth(width());
		update();
	}

	Ui::RoundButton *_refreshButton = nullptr;
	Ui::RoundButton *_disconnectButton = nullptr;
	QString _syncNote;
	bool _syncing = false;
	Fn<void()> _login;
	Fn<void()> _logout;
	bool _playlists = false;
	QString _note;
	QImage _avatar;
	QString _avatarUrl;
	bool _logged = false;

};

void PlayAll(
		std::vector<Track> tracks,
		bool skipUnavailable = false,
		QString albumId = {}) {
	auto &queue = QueueState();
	if (tracks.empty()) {
		return;
	}
	queue.custom = false;
	queue.radio.reset();
	queue.pendingRadio.reset();
	queue.tracks = std::move(tracks);
	queue.album = { .id = std::move(albumId) };
	if (!queue.album.id.isEmpty()) {
		auto &ids = queue.album.trackIds;
		ids.reserve(queue.tracks.size());
		for (const auto &track : queue.tracks) {
			ids.push_back(track.id);
		}
		std::ranges::sort(ids);
	}
	queue.stream = StreamEnabled();
	queue.refilling = false;
	queue.waiting = false;
	++queue.streamEpoch;
	queue.autoIds.clear();
	if (queue.stream) {
		for (const auto &t : queue.tracks) {
			queue.autoIds.insert(t.id);
		}
	}
	PlayIndex(0, skipUnavailable);
}

struct HomeTrackPlaybackState {
	bool current = false;
	bool playing = false;

	bool operator==(const HomeTrackPlaybackState &) const = default;
};

[[nodiscard]] HomeTrackPlaybackState HomePlaybackState(
		Media::Player::State state) {
	return {
		!Media::Player::IsStoppedOrStopping(state),
		state == Media::Player::State::Playing
			|| state == Media::Player::State::Starting
			|| state == Media::Player::State::Resuming,
	};
}

[[nodiscard]] HomeTrackPlaybackState HomeTrackPlayback(const QString &id) {
	if (PlayingTrack().id != id) {
		return {};
	}
	return HomePlaybackState(
		Media::Player::instance()->getState(AudioMsgId::Type::Song).state);
}

[[nodiscard]] HomeTrackPlaybackState HomeAlbumPlayback(const QString &id) {
	const auto &queue = QueueState();
	if (id.isEmpty() || queue.album.id != id
		|| !std::ranges::binary_search(queue.album.trackIds, queue.currentId)) {
		return {};
	}
	const auto state = Media::Player::instance()->getState(AudioMsgId::Type::Song);
	return UsesQueue(state.id) ? HomePlaybackState(state.state) : HomeTrackPlaybackState();
}

[[nodiscard]] auto HomePlaybackChanges() {
	return Media::Player::instance()->updatedNotifier(
	) | rpl::filter([](const Media::Player::TrackState &state) {
		return state.id.type() == AudioMsgId::Type::Song;
	}) | rpl::map([](const Media::Player::TrackState &state) {
		return std::pair(state.id, state.state);
	}) | rpl::distinct_until_changed();
}

void PlayHomeAlbum(
		not_null<Window::SessionController*> controller,
		const Album &album,
		Fn<void()> done = nullptr) {
	BindController(controller);
	if (HomeAlbumPlayback(album.id).current) {
		Media::Player::instance()->playPause(AudioMsgId::Type::Song);
		if (done) {
			done();
		}
		return;
	}
	const auto token = ++QueueState().pendingToken;
	AlbumTracks(album.id, crl::guard(controller, [=](std::vector<Track> tracks, QString error) {
		if (done) {
			done();
		}
		if (token != QueueState().pendingToken) {
			return;
		}
		if (!error.isEmpty()) {
			controller->showToast(error);
			return;
		}
		std::erase_if(tracks, [](const Track &track) { return !CanPlayTrack(track); });
		if (tracks.empty()) {
			controller->showToast(QString::fromUtf8(Lang::Tr("В альбоме нет доступных треков")));
			return;
		}
		for (auto &track : tracks) {
			if (track.artworkUrl.isEmpty()) {
				track.artworkUrl = album.artworkUrl;
			}
		}
		RecordRecentAlbum(album);
		PlayAll(std::move(tracks), true, album.id);
	}));
}

bool ToggleTrackLike(
	Track track,
	not_null<Window::SessionController*> controller,
	QWidget *source = nullptr);

class TrackLikeButton final : public Ui::AbstractButton {
public:
	TrackLikeButton(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Track track,
		bool roomy = false)
	: AbstractButton(parent)
	, _track(std::move(track))
	, _roomy(roomy) {
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		const auto refresh = [=] {
			const auto text = IsLiked(_track.id)
				? "Убрать из избранного"
				: "В избранное";
			setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr(text)) + u": "_q + _track.title);
			update();
		};
		refresh();
		LikesChanged() | rpl::on_next(refresh, lifetime());
		setClickedCallback([=] { ToggleTrackLike(_track, controller, this); });
		paintRequest() | rpl::on_next([=](QRect) {
			auto p = QPainter(this);
			auto hq = PainterHighQualityEnabler(p);
			const auto liked = IsLiked(_track.id);
			auto icon = QRectF(rect()).adjusted(5, 6, -5, -5);
			if (_roomy) {
				const auto side = style::ConvertScale(22);
				icon = QRectF(QPointF(), QSizeF(side, side));
				icon.moveCenter(QRectF(rect()).center());
				if (isOver() || isDown()) {
					const auto inset = style::ConvertScale(3);
					p.setPen(Qt::NoPen);
					p.setBrush(isDown() ? st::windowBgRipple : st::windowBgOver);
					p.drawEllipse(rect().adjusted(inset, inset, -inset, -inset));
				}
			}
			const auto color = !liked
				? MusicSecondaryColor()
				: IsLightMusicTheme()
				? anim::color(HeartColor(), st::windowFg->c, .24)
				: HeartColor();
			DrawHeart(p, icon, liked, color);
			if (hasFocus()) {
				const auto line = _roomy ? style::ConvertScale(2) : 2;
				p.setPen(QPen(st::windowActiveTextFg->c, line));
				p.setBrush(Qt::NoBrush);
				if (_roomy) {
					p.drawEllipse(rect().adjusted(line, line, -line, -line));
				} else {
					p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 5, 5);
				}
			}
		}, lifetime());
	}

protected:
	void onStateChanged(State, StateChangeSource) override {
		if (_roomy) {
			update();
		}
	}

private:
	Track _track;
	const bool _roomy;

};

// Плавное наведение с разворотом от текущего кадра.
struct HoverFade {
	Ui::Animations::Simple animation;
	bool on = false;

	void set(bool now, Fn<void()> repaint) {
		if (on == now) {
			return;
		}
		const auto from = value();
		on = now;
		if (!enabled()) {
			animation.stop();
			repaint();
			return;
		}
		animation.start(
			std::move(repaint),
			from,
			now ? 1. : 0.,
			now ? 160 : 120,
			anim::easeOutCubic);
	}
	[[nodiscard]] double value() const {
		const auto target = on ? 1. : 0.;
		return enabled() ? animation.value(target) : target;
	}

private:
	[[nodiscard]] static bool enabled() {
		return !anim::Disabled() && !PowerSaving::On(PowerSaving::kAnimations);
	}
};

constexpr auto kHomeCardInset = 10;
constexpr auto kHomeCardGap = 8;
constexpr auto kHomeCardDetailsHeight = 56;

[[nodiscard]] int HomeShelfCardWidth(int width) {
	return std::clamp(
		width - 2 * style::ConvertScale(BoxParts::kOuterSide),
		1,
		style::ConvertScale(200));
}

[[nodiscard]] QRect HomeCardArtRect(int width) {
	const auto inset = style::ConvertScale(kHomeCardInset);
	const auto side = std::max(width - 2 * inset, 0);
	return QRect(inset, inset, side, side);
}

[[nodiscard]] QColor HomeSurfaceColor(
		double hover,
		bool pressed,
		bool selected,
		QColor selectedTint = st::windowBgActive->c) {
	const auto surface = MusicCardColor();
	const auto base = selected
		? anim::color(surface, selectedTint, .16)
		: surface;
	return anim::color(base, st::windowFg->c, pressed ? .12 : .08 * hover);
}

void PaintHomeSurface(
		QPainter &p,
		QRect rect,
		double hover,
		bool pressed,
		bool focused,
		bool selected = false,
		QColor selectedTint = st::windowFg->c) {
	const auto radius = std::min(
		style::ConvertScale(BoxParts::kCardRadius),
		std::min(rect.width(), rect.height()) / 2);
	p.setPen(Qt::NoPen);
	p.setBrush(HomeSurfaceColor(hover, pressed, selected, selectedTint));
	p.drawRoundedRect(rect, radius, radius);
	if (focused) {
		const auto inset = style::ConvertScale(1);
		p.setPen(QPen(st::windowActiveTextFg->c, style::ConvertScale(2)));
		p.setBrush(Qt::NoBrush);
		const auto innerRadius = std::max(radius - inset, 0);
		p.drawRoundedRect(rect.adjusted(inset, inset, -inset, -inset),
			innerRadius, innerRadius);
		p.setPen(Qt::NoPen);
	}
}

void PaintMusicCard(QPainter &p, QRect rect) {
	auto hq = PainterHighQualityEnabler(p);
	PaintHomeSurface(p, rect, 0., false, false);
}

class MusicTrackList final : public Ui::VerticalLayout {
public:
	explicit MusicTrackList(QWidget *parent) : VerticalLayout(parent) {
		paintOn([this](QPainter &p) { PaintMusicCard(p, rect()); });
	}
};

[[nodiscard]] MusicTrackList *AddMusicTrackList(Ui::VerticalLayout *parent, int top = 0) {
	const auto side = style::ConvertScale(BoxParts::kOuterSide);
	return parent->add(object_ptr<MusicTrackList>(parent), QMargins(side, top, side, 0));
}

class CompactQueueList final : public Ui::VerticalLayout {
public:
	using VerticalLayout::VerticalLayout;
};

void PaintTrackListState(QPainter &p, const Ui::AbstractButton &row, bool selected) {
	const auto over = row.isOver();
	const auto down = row.isDown();
	const auto focused = row.hasFocus();
	if (row.isDisabled() || !(selected || over || down || focused)) {
		return;
	}
	const auto panelHeight = row.parentWidget()->height();
	const auto first = (row.y() == 0);
	const auto last = (row.y() + row.height() == panelHeight);
	const auto radius = std::min(style::ConvertScale(BoxParts::kCardRadius), panelHeight / 2);
	if (selected || over || down) {
		const auto fill = HomeSurfaceColor(over ? 1. : 0., down, selected);
		if (first || last) {
			// The row clips the extended ends, leaving only the panel corners rounded.
			const auto surface = row.rect().adjusted(0, first ? 0 : -radius, 0, last ? 0 : radius);
			p.setPen(Qt::NoPen);
			p.setBrush(fill);
			p.drawRoundedRect(surface, radius, radius);
		} else {
			p.fillRect(row.rect(), fill);
		}
	}
	if (focused) {
		const auto inset = style::ConvertScale(4);
		const auto corner = (first || last) ? std::max(radius - inset, 0) : style::ConvertScale(8);
		p.setPen(QPen(st::windowActiveTextFg->c, style::ConvertScale(2)));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(row.rect().adjusted(inset, inset, -inset, -inset), corner, corner);
	}
}

void PaintHomeCover(QPainter &p, const QRect &rect, const QImage &art) {
	const auto radius = style::ConvertScale(BoxParts::kCardRadius - kHomeCardInset);
	auto clip = QPainterPath();
	clip.addRoundedRect(rect, radius, radius);
	p.save();
	p.setClipPath(clip);
	if (art.isNull()) {
		p.fillRect(rect, st::windowBgOver->c);
	} else {
		p.drawImage(rect, art);
	}
	p.restore();
}

[[nodiscard]] QRect HomeActionRect(const QRect &art) {
	const auto s = [](int v) { return style::ConvertScale(v); };
	const auto size = std::min(s(38), std::max(art.width() - s(16), 0));
	return QRect(art.right() - s(8) - size + 1,
		art.bottom() - s(8) - size + 1, size, size);
}

void PaintIslandGlyph(QPainter &p, const char *svg, QPointF center, int size, QColor color);
[[nodiscard]] QColor ContrastingForeground(const QColor &background);

[[nodiscard]] QColor PlaybackSurfaceColor() {
	auto base = st::windowBg->c;
	auto ink = st::windowFg->c;
	base.setAlpha(255);
	ink.setAlpha(255);
	return anim::color(base, ink, .04);
}

void PaintHomeCardAction(
		QPainter &p,
		const QRect &button,
		double shown = 1.,
		bool playing = false) {
	const auto s = [](int v) { return style::ConvertScale(v); };
	if (button.width() < s(24)) {
		return;
	}
	shown = std::max(shown, playing ? 1. : 0.);
	if (shown <= 0.) {
		return;
	}
	p.save();
	p.setOpacity(p.opacity() * shown);
	p.setPen(Qt::NoPen);
	const auto surface = PlaybackSurfaceColor();
	p.setBrush(surface);
	p.drawEllipse(button);
	const auto ink = st::windowFg->c;
	const auto glyph = Ui::CountContrast(surface, ink) >= 4.5 ? ink : ContrastingForeground(surface);
	using namespace Mokagram::Music::IslandGlyphs;
	PaintIslandGlyph(p, playing ? kGlyphPause : kGlyphPlay, QRectF(button).center(), s(16), glyph);
	p.restore();
}

class TrackMenuButton final : public QToolButton {
public:
	explicit TrackMenuButton(QWidget *parent) : QToolButton(parent) {
		setCursor(Qt::PointingHandCursor);
		setAutoRaise(true);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		p.setRenderHint(QPainter::Antialiasing);
		if (hasFocus()) {
			p.setPen(QPen(st::windowActiveTextFg->c, 1.5));
			p.setBrush(Qt::NoBrush);
			p.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 7, 7);
		}
		p.setPen(Qt::NoPen);
		p.setBrush((underMouse() || hasFocus()) ? st::windowFg->c : MusicSecondaryColor());
		const auto center = QPointF(width() / 2., height() / 2.);
		for (auto i = -1; i <= 1; ++i) {
			p.drawEllipse(center + QPointF(i * 4.5, 0), 1.5, 1.5);
		}
	}
};

[[nodiscard]] QString PlaylistText(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

void PlaylistNameBox(
		not_null<Window::SessionController*> controller,
		const QString &title,
		const QString &initial,
		Fn<void(QString)> done) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		Mokagram::Plugins::RoundedKeepButtons(box, st::defaultBox);
		box->setTitle(rpl::single(title));
		const auto input = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			rpl::single(PlaylistText("Название плейлиста"))));
		input->setText(initial);
		box->setFocusCallback([=] { input->setFocusFast(); });
		const auto submit = [=] {
			const auto name = input->getLastText().trimmed();
			if (name.isEmpty()) {
				input->showError();
				return;
			}
			done(name);
			box->closeBox();
		};
		input->submits() | rpl::on_next(submit, input->lifetime());
		box->addButton(rpl::single(PlaylistText("Сохранить")), submit);
		box->addButton(rpl::single(PlaylistText("Отмена")), [=] { box->closeBox(); });
	}));
}

void AddPlaylistDestinationMenu(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		Fn<void(QString)> add) {
	auto submenu = std::make_unique<Ui::PopupMenu>(menu.get(), st::popupMenuWithIcons);
	submenu->addAction(PlaylistText("Создать плейлист…"), [=] {
		PlaylistNameBox(controller, PlaylistText("Создать плейлист"), {}, [=](QString name) {
			const auto id = CreatePlaylist(name);
			if (!id.isEmpty()) add(id);
		});
	}, &st::menuIconAdd);
	const auto playlists = Playlists();
	if (!playlists.empty()) submenu->addSeparator();
	for (const auto &playlist : playlists) {
		const auto id = playlist.id;
		submenu->addAction(playlist.name, [=] { add(id); }, nullptr);
	}
	menu->addAction(PlaylistText("Добавить в плейлист"), std::move(submenu), &st::menuIconAdd);
}

void AddToPlaylistMenu(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		const Track &track) {
	if (track.id.isEmpty() || IsTelegramTrack(track)) return;
	AddPlaylistDestinationMenu(menu, controller, [=](QString id) { AddPlaylistTrack(id, track); });
}

void AddAlbumToPlaylistMenu(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		const Album &album) {
	if (album.id.isEmpty()) return;
	AddPlaylistDestinationMenu(menu, controller, [=](QString id) {
		AlbumTracks(album.id, crl::guard(controller, [=](std::vector<Track> tracks, QString error) {
			if (!error.isEmpty()) {
				controller->showToast(error);
				return;
			}
			std::erase_if(tracks, [](const Track &track) { return !CanPlayTrack(track) || track.id.isEmpty() || IsTelegramTrack(track); });
			if (tracks.empty()) {
				controller->showToast(PlaylistText("В альбоме нет доступных треков"));
				return;
			}
			for (auto &track : tracks) {
				if (track.artworkUrl.isEmpty()) track.artworkUrl = album.artworkUrl;
			}
			AddPlaylistTracks(id, tracks);
			controller->showToast(PlaylistText("Альбом добавлен в плейлист"));
		}));
	});
}

void InstallMusicContextMenu(
		not_null<Ui::RpWidget*> widget,
		Fn<void(not_null<Ui::PopupMenu*>)> populate) {
	const auto menu = widget->lifetime().make_state<base::unique_qptr<Ui::PopupMenu>>();
	widget->events() | rpl::filter([](not_null<QEvent*> event) {
		if (event->type() == QEvent::ContextMenu) return true;
		if (event->type() != QEvent::KeyPress) return false;
		const auto key = static_cast<QKeyEvent*>(event.get());
		return key->key() == Qt::Key_Menu
			|| (key->key() == Qt::Key_F10 && key->modifiers() == Qt::ShiftModifier);
	}) | rpl::on_next([=](not_null<QEvent*> event) {
		event->accept();
		*menu = base::make_unique_q<Ui::PopupMenu>(widget, st::popupMenuWithIcons);
		populate(menu->get());
		const auto position = event->type() == QEvent::ContextMenu
			&& static_cast<QContextMenuEvent*>(event.get())->reason() != QContextMenuEvent::Keyboard
			? static_cast<QContextMenuEvent*>(event.get())->globalPos()
			: widget->mapToGlobal(widget->rect().center());
		(*menu)->popup(position);
	}, widget->lifetime());
}

void InstallAlbumContextMenu(
		not_null<Ui::RpWidget*> widget,
		not_null<Window::SessionController*> controller,
		Album album) {
	InstallMusicContextMenu(widget, [=](not_null<Ui::PopupMenu*> menu) {
		AddAlbumToPlaylistMenu(menu, controller, album);
	});
}

class HomeCardPlayButton final : public QToolButton {
public:
	HomeCardPlayButton(QWidget *parent, QString title)
	: QToolButton(parent)
	, _title(std::move(title)) {
		setCursor(Qt::PointingHandCursor);
		setFocusPolicy(Qt::TabFocus);
		setAttribute(Qt::WA_NoMousePropagation);
		refreshLabel();
	}

	void layoutOnCover(int cardWidth) {
		const auto action = HomeActionRect(HomeCardArtRect(cardWidth));
		setGeometry(action);
		setVisible(action.width() >= style::ConvertScale(24));
	}

	void setPresentation(HomeTrackPlaybackState playback, double shown) {
		if (_playback != playback || _shown != shown) {
			const auto relabel = (_playback.playing != playback.playing);
			_playback = playback;
			_shown = shown;
			if (relabel) {
				refreshLabel();
			}
			update();
		}
	}

	[[nodiscard]] bool loading() const {
		return _loading;
	}

	void setLoading(bool loading) {
		if (_loading != loading) {
			_loading = loading;
			refreshLabel();
			update();
		}
	}

protected:
	void paintEvent(QPaintEvent *) override {
		const auto shown = (hasFocus() || isDown() || _loading) ? 1. : _shown;
		if (shown <= 0. && !_playback.playing) {
			return;
		}
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		PaintHomeCardAction(p, rect(), _loading ? shown * .65 : shown,
			_playback.playing);
		if (hasFocus()) {
			const auto focus = style::ConvertScale(2);
			const auto inset = focus / 2.;
			p.setPen(QPen(st::windowActiveTextFg->c, focus));
			p.setBrush(Qt::NoBrush);
			p.drawEllipse(QRectF(rect()).adjusted(inset, inset, -inset, -inset));
		}
	}

private:
	void refreshLabel() {
		const auto label = PlaylistText(_loading
			? "Загружаю альбом…"
			: _playback.playing
			? "Пауза"
			: "Воспроизвести");
		setAccessibleName(label + u": "_q + _title);
		setToolTip(label);
	}

	const QString _title;
	HomeTrackPlaybackState _playback;
	double _shown = 0.;
	bool _loading = false;
};

enum class RowMode {
	Results, Queue, Context, Home, DetailTrack, AlbumTrack,
	QueueCurrent, QueueUpcoming, QueueHistory,
};

[[nodiscard]] bool IsDetailTrackRow(RowMode mode) {
	return mode == RowMode::DetailTrack || mode == RowMode::AlbumTrack;
}

struct MovingTitlePath {
	QString title;
	QFont font;
	qreal baseline = 0.;
	QPainterPath path;
};

void PaintMovingTitle(
		QPainter &p,
		QRect rect,
		const QString &title,
		const QFont &font,
		crl::time elapsedMs,
		qreal dpr,
		MovingTitlePath &cached) {
	if (rect.isEmpty() || title.isEmpty()) return;
	const auto metrics = QFontMetricsF(font);
	const auto textWidth = metrics.horizontalAdvance(title);
	p.setFont(font);
	if (textWidth <= rect.width()) {
		p.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, title);
		return;
	}

	const auto speed = style::ConvertScale(38) / 1000.;
	const auto distance = textWidth + style::ConvertScale(36);
	const auto pause = 1100.;
	const auto ramp = 300.;
	const auto travel = distance / speed + ramp;
	const auto cycle = pause + travel;
	const auto elapsed = std::fmod(double(elapsedMs), cycle);
	const auto time = std::max(elapsed - pause, 0.);
	const auto remaining = travel - time;
	const auto offset = (time < ramp)
		? speed * time * time / (2. * ramp)
		: (remaining < ramp)
		? distance - speed * remaining * remaining / (2. * ramp)
		: speed * (time - ramp / 2.);
	const auto color = p.pen().color();
	auto text = QImage(
		QSize(int(std::ceil(rect.width() * dpr)), int(std::ceil(rect.height() * dpr))),
		QImage::Format_ARGB32_Premultiplied);
	text.setDevicePixelRatio(dpr);
	text.fill(Qt::transparent);
	{
		auto layer = QPainter(&text);
		auto hq = PainterHighQualityEnabler(layer);
		const auto baseline = (rect.height() + metrics.ascent() - metrics.descent()) / 2.;
		if (cached.title != title || cached.font != font || cached.baseline != baseline) {
			cached = { title, font, baseline, QPainterPath() };
			cached.path.addText(QPointF(0., baseline), font, title);
			if (font.underline()) {
				cached.path.addRect(QRectF(
					0., baseline + metrics.underlinePos(), textWidth, metrics.lineWidth()));
			}
		}
		const auto &glyphs = cached.path;
		layer.translate(-offset, 0.);
		layer.fillPath(glyphs, color);
		if (distance - offset < rect.width()) {
			layer.translate(distance, 0.);
			layer.fillPath(glyphs, color);
		}
		layer.resetTransform();
		layer.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		const auto feather = std::min<double>(style::ConvertScale(14), rect.width() / 2.);
		auto mask = QLinearGradient(0., 0., rect.width(), 0.);
		mask.setColorAt(0., QColor(0, 0, 0, qRound(
			255. * (1. - std::clamp(std::min(offset, distance - offset) / feather, 0., 1.)))));
		mask.setColorAt(feather / double(rect.width()), Qt::black);
		mask.setColorAt(1. - feather / double(rect.width()), Qt::black);
		mask.setColorAt(1., Qt::transparent);
		layer.fillRect(QRect(QPoint(), rect.size()), mask);
	}
	p.drawImage(rect.topLeft(), text);
}

class Row final : public Ui::AbstractButton {
public:
	void onStateChanged(State, StateChangeSource) override {
		if (_mode == RowMode::Results) {
			updateResultTitleScroll();
		}
		if (_mode == RowMode::Home || _mode == RowMode::Results) {
			_hoverFade.set(!isDisabled() && (isOver() || isDown()), [=] { update(); });
		}
		if (_mode == RowMode::Home || _grouped || _compactQueue) {
			update();
		}
	}

	Row(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		const Track &track,
		int index,
		RowMode mode = RowMode::Results,
		std::shared_ptr<std::vector<Track>> results = nullptr,
		int resultIndex = -1,
		Fn<void(Track)> open = nullptr,
		QString playlistId = {},
		bool homePlaybackControls = true)
	: AbstractButton(parent)
	, _track(track)
	, _mode(mode)
	, _grouped(dynamic_cast<MusicTrackList*>(parent) != nullptr)
	, _compactQueue(dynamic_cast<CompactQueueList*>(parent) != nullptr)
	, _titleFont(_grouped
		? style::font(
			style::ConvertScale(16),
			st::semiboldFont->flags(),
			st::semiboldFont->family())
		: st::semiboldFont)
	, _trackNumber(mode == RowMode::AlbumTrack ? QString::number(index + 1) : QString())
	, _queueIndex(index)
	, _results(std::move(results))
	, _homePlaybackControls(homePlaybackControls)
	, _playlistId(std::move(playlistId)) {
		resize(width(), listMetric((_grouped || _compactQueue) ? kGroupedTrackHeight : kRowHeight));
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		if (mode == RowMode::Results) {
			_menuButton = new TrackMenuButton(this);
			_menuButton->setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Действия с треком")));
			_menuButton->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Действия с треком")));
			connect(_menuButton, &QToolButton::clicked, this, [=] {
				auto event = QContextMenuEvent(QContextMenuEvent::Mouse, rect().center(), _menuButton->mapToGlobal(_menuButton->rect().bottomLeft()));
				showMenu(controller, resultIndex >= 0 ? resultIndex : index, &event);
			});
		}
		const auto unavailable = !CanPlayTrack(_track);
		const auto unavailableText = QString::fromUtf8(Mokagram::Lang::Tr("Недоступен для воспроизведения"));
		if (_mode != RowMode::Home && !IsDetailTrackRow(mode)
			&& _mode != RowMode::QueueCurrent && _mode != RowMode::QueueUpcoming
			&& _mode != RowMode::QueueHistory
			&& !_track.videoId.isEmpty() && _track.playbackCount > 0) {
			_playsText = QString::fromUtf8(Mokagram::Lang::Tr("%1 просл."))
				.arg(::Lang::FormatCountToShort(_track.playbackCount).string);
		}
		setAccessibleName((_trackNumber.isEmpty() ? QString() : _trackNumber + u". "_q)
			+ _track.title + u" - "_q + _track.artist
			+ (_playsText.isEmpty() ? QString() : u" · "_q + _playsText)
			+ (unavailable ? u" · "_q + unavailableText : QString()));
		if (unavailable) {
			setDisabled(true);
			setToolTip(unavailableText);
		} else if (!IsTelegramTrack(_track)) {
			_likeButton = Ui::CreateChild<TrackLikeButton>(
				this, controller, _track, _grouped || _compactQueue);
		}
		if (_mode != RowMode::Home) {
			_equalizerTimer.setInterval(33);
			connect(&_equalizerTimer, &QTimer::timeout, this, [=] {
				_equalizerPhase += _equalizerClock.restart() / 1000.;
				update(listLeadingRect());
			});
			Media::Player::instance()->updatedNotifier(
			) | rpl::on_next([=] { updateAnimation(); }, lifetime());
			PowerSaving::Changes(
			) | rpl::on_next([=] { updateAnimation(); }, lifetime());
			connect(qApp, &QGuiApplication::applicationStateChanged, this, [=] {
				updateAnimation();
			});
		} else if (_homePlaybackControls && !unavailable) {
			HomePlaybackChanges() | rpl::on_next([=] { update(); }, lifetime());
		}
		if (mode != RowMode::AlbumTrack) {
			if (const auto item = TrackMessage(track)) {
				const auto document = item->media()->document();
				_media = document->createMediaView();
				_media->thumbnailWanted(item->fullId());
				LoadLocalArtwork(track, crl::guard(this, [=](QImage image) {
					if (!image.isNull()) {
						_media = nullptr;
						_art = std::move(image);
						update();
					}
				}));
				document->session().downloaderTaskFinished(
				) | rpl::on_next([=] { update(); }, lifetime());
			} else if (!IsTelegramTrack(track)) {
				LoadImage(ArtUrl(track.artworkUrl, u"t200x200"_q), crl::guard(this, [=](QImage image) {
					_art = std::move(image);
					update();
				}));
			}
		}
		QueueState().changes.events(
		) | rpl::on_next([=] {
			updateAnimation();
			update();
		}, lifetime());
		LikesChanged(
		) | rpl::on_next([=] {
			update();
		}, lifetime());
		const auto playIndex = (resultIndex >= 0) ? resultIndex : index;
		const auto opensTrack = (_mode == RowMode::Home && bool(open));
		const auto play = [=] {
			if (_queueDragging) {
				return;
			}
			if (opensTrack && _homePlaybackControls
				&& HomeTrackPlayback(_track.id).current) {
				Media::Player::instance()->playPause(AudioMsgId::Type::Song);
				return;
			}
			if (_mode == RowMode::QueueCurrent) {
				Media::Player::instance()->playPause(AudioMsgId::Type::Song);
			} else if (_mode == RowMode::Queue || _mode == RowMode::QueueUpcoming) {
				PlayIndex(index);
			} else if (_mode == RowMode::Context) {
				PlayContext(index);
			} else if (_mode == RowMode::QueueHistory) {
				playRecent();
			} else {
				if (_results) {
					QueueState().results = *_results;
				}
				if (_mode == RowMode::Home) {
					QueueState().custom = false;
				}
				PlayFromResults(playIndex);
			}
		};
		if (_mode == RowMode::Home && !unavailable) {
			_playButton = Ui::CreateChild<HomeCardPlayButton>(this, _track.title);
			connect(_playButton, &QToolButton::clicked, this, play);
		}
		if (opensTrack) {
			setClickedCallback([=] { open(_track); });
		} else {
			setClickedCallback(play);
		}
		paintRequest(
		) | rpl::on_next([=](QRect area) {
			paint(area);
		}, lifetime());
		events(
		) | rpl::filter([](not_null<QEvent*> e) {
			return e->type() == QEvent::ContextMenu;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			e->accept();
			if (CanPlayTrack(_track)) {
				if (_mode == RowMode::QueueCurrent) {
					const auto event = static_cast<QContextMenuEvent*>(e.get());
					ShowCurrentTrackMenu(controller,
						event->reason() == QContextMenuEvent::Keyboard
							? mapToGlobal(rect().center())
							: event->globalPos(), this);
					return;
				}
				showMenu(controller, playIndex, static_cast<QContextMenuEvent*>(e.get()));
			}
		}, lifetime());
	}

	[[nodiscard]] bool isQueueTrack(const QString &id, int index) const {
		return _track.id == id && _queueIndex == index;
	}

	[[nodiscard]] bool matchesQueueTrack(const Track &track, RowMode mode, int index) const {
		const auto fields = [](const Track &t) {
			return std::tie(
				t.id, t.title, t.artist, t.videoId, t.artworkUrl, t.permalink,
				t.transcodingUrl, t.authorization, t.extension, t.durationMs,
				t.hls, t.unavailable, t.artistId, t.artistArtworkUrl, t.genre,
				t.tags, t.playbackCount, t.likesCount, t.createdAt, t.uploaderName);
		};
		return _mode == mode && _queueIndex == index && fields(_track) == fields(track);
	}

	void setQueueDragging(bool dragging) {
		_queueDragging = dragging;
	}

	void setTextLinks(
			Fn<void(Track)> title,
			Fn<void(const Track&, const QString&)> artist,
			QStringView hiddenArtist = {}) {
		if (!CanPlayTrack(_track)) return;
		_titleLink = makeTextLink(_track.title, [=] { title(_track); });
		_artistNames = TrackArtistNames(_track);
		if (!hiddenArtist.isEmpty()) {
			_artistNames.removeIf([&](const QString &name) {
				return hiddenArtist.compare(name, Qt::CaseInsensitive) == 0;
			});
		}
		for (const auto &name : _artistNames) {
			_artistLinks.push_back(makeTextLink(name, [=] { artist(_track, name); }));
		}
		if (_artistLinks.size() > 1) {
			_moreArtists = makeTextLink(PlaylistText("Все исполнители"), [=] {
				_artistsMenu = base::make_unique_q<Ui::PopupMenu>(this, st::popupMenuWithIcons);
				for (const auto &name : _artistNames) {
					_artistsMenu->addAction(name, [=] { artist(_track, name); }, nullptr);
				}
				_artistsMenu->popup(_moreArtists->mapToGlobal(_moreArtists->rect().bottomLeft()));
			});
			_moreArtists->setToolTip(PlaylistText("Все исполнители"));
		}
		layoutTextLinks();
	}


private:
	void playRecent() const {
		auto &queue = QueueState();
		const auto at = std::min<int>(queue.index + 1, queue.tracks.size());
		const auto next = queue.tracks.begin() + at;
		const auto existing = std::find_if(next, queue.tracks.end(),
			[&](const Track &track) { return track.id == _track.id; });
		if (existing == queue.tracks.end()) {
			queue.tracks.insert(next, _track);
		} else if (existing != next) {
			std::rotate(next, existing, existing + 1);
		}
		PlayIndex(at);
	}

	[[nodiscard]] bool showsCurrentTrack() const {
		return _mode == RowMode::QueueCurrent
			|| (_mode != RowMode::QueueUpcoming && _mode != RowMode::QueueHistory
				&& QueueState().currentId == _track.id);
	}

protected:
	void showEvent(QShowEvent *e) override;
	void hideEvent(QHideEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void resizeEvent(QResizeEvent *e) override {
		AbstractButton::resizeEvent(e);
		if (_mode == RowMode::Results) {
			updateResultTitleScroll();
		}
		layoutTextLinks();
		if (_menuButton) {
			const auto side = listMetric(32);
			_menuButton->setGeometry(width() - listMetric(48), (height() - side) / 2, side, side);
		}
		if (_likeButton) {
			_likeButton->setGeometry(heartRect());
		}
		if (_compactQueue && !isDisabled()) {
			setToolTip(_titleFont->width(_track.title) > listTextRect(true).width()
				? _track.title : QString());
		}
		if (_playButton) {
			_playButton->layoutOnCover(width());
		}
	}

private:
	[[nodiscard]] Ui::AbstractButton *makeTextLink(const QString &text, Fn<void()> action) {
		const auto button = Ui::CreateChild<Ui::AbstractButton>(this);
		button->setPointerCursor(true);
		button->setFocusPolicy(Qt::TabFocus);
		button->setAccessibleName(text);
		button->setClickedCallback(std::move(action));
		button->paintRequest() | rpl::on_next([=](QRect) {
			if (button->isOver() || button->hasFocus()) {
				auto p = QPainter(button);
				p.setPen(st::windowActiveTextFg);
				const auto y = button->height() - style::ConvertScale(2);
				p.drawLine(0, y, button->width(), y);
			}
		}, button->lifetime());
		return button;
	}

	[[nodiscard]] int listMetric(int value) const {
		return (_grouped || _compactQueue)
			? style::ConvertScale(value)
			: value;
	}

	[[nodiscard]] QRect listLeadingRect() const {
		if (_compactQueue) {
			const auto side = listMetric(36);
			return QRect(listMetric(8), (height() - side) / 2, side, side);
		}
		if (_grouped) {
			const auto side = listMetric(kGroupedTrackLeadWidth);
			return QRect(listMetric(kGroupedTrackLeadingInset), (height() - side) / 2, side, side);
		}
		const auto side = listMetric(kArtSize);
		return QRect(listMetric(20), (height() - side) / 2, side, side);
	}

	[[nodiscard]] int durationRight() const {
		const auto inset = _compactQueue ? 12 : _grouped
			? (_menuButton ? kGroupedTrackMenuInset : kGroupedTrackDurationInset)
			: (_menuButton ? 72 : 24);
		return width() - listMetric(inset);
	}

	[[nodiscard]] QRect listTextRect(bool title) const {
		if (_grouped || _compactQueue) {
			const auto art = listLeadingRect();
			const auto left = art.x() + art.width() + listMetric(_compactQueue ? 8 : kGroupedTrackTextGap);
			const auto right = heartRect().x() - listMetric(_compactQueue ? 4 : 8);
			const auto titleHeight = _titleFont->height;
			const auto subtitleHeight = (isDisabled() || !_artistNames.empty()
				|| (!IsDetailTrackRow(_mode) && !_track.artist.isEmpty()))
				? st::normalFont->height
				: 0;
			const auto top = (height() - titleHeight - subtitleHeight) / 2;
			return QRect(left, title ? top : top + titleHeight,
				std::max(right - left, 0), title ? titleHeight : subtitleHeight);
		}
		const auto left = listLeadingRect().right() + listMetric(16);
		const auto right = IsTelegramTrack(_track)
			? durationRight() - st::normalFont->width(FormatDuration(_track.durationMs)) - listMetric(14)
			: heartRect().left() - listMetric(12);
		return QRect(
			left,
			height() / 2 + listMetric(title ? -24 : 2),
			std::max(right - left, 0),
			listMetric(title ? 22 : 20));
	}

	void layoutTextLinks() {
		if (!_titleLink) {
			return;
		}
		auto title = listTextRect(true);
		title.setWidth(std::min(_titleFont->width(_track.title), title.width()));
		_titleLink->setGeometry(title);
		_titleLink->setVisible(!title.isEmpty());
		if (_artistLinks.empty()) {
			return;
		}
		const auto subtitle = listTextRect(false);
		const auto font = st::normalFont;
		const auto separator = font->width(u", "_q);
		auto total = -separator;
		for (const auto &name : _artistNames) {
			total += font->width(name) + separator;
		}
		const auto overflow = _moreArtists && total > subtitle.width();
		const auto moreWidth = std::min(subtitle.width(), style::ConvertScale(18));
		const auto limit = subtitle.x() + subtitle.width() - (overflow ? moreWidth + separator : 0);
		auto x = subtitle.x();
		_visibleArtistLinks = 0;
		for (auto i = 0; i != int(_artistLinks.size()); ++i) {
			const auto button = _artistLinks[i];
			const auto nameWidth = font->width(_artistNames[i]);
			const auto start = x + (i ? separator : 0);
			const auto remaining = std::max(limit - start, 0);
			const auto visible = (i == _visibleArtistLinks) && remaining > 0
				&& (i == 0 || nameWidth <= remaining);
			button->setVisible(visible);
			if (visible) {
				const auto width = std::min(nameWidth, remaining);
				button->setGeometry(start, subtitle.y(), width, subtitle.height());
				x = start + width;
				++_visibleArtistLinks;
			}
		}
		if (_moreArtists) {
			_moreArtists->setGeometry(
				x + (_visibleArtistLinks ? separator : 0),
				subtitle.y(), moreWidth, subtitle.height());
			_moreArtists->setVisible(overflow && moreWidth > 0);
		}
	}

	void paintArtistLinks(QPainter &p, const QRect &subtitle) const {
		const auto font = st::normalFont;
		const auto separator = u", "_q;
		const auto separatorWidth = font->width(separator);
		const auto more = _moreArtists && !_moreArtists->isHidden();
		for (auto i = 0; i != _visibleArtistLinks; ++i) {
			const auto rect = _artistLinks[i]->geometry();
			p.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft,
				font->elided(_artistNames[i], rect.width()));
			if (i + 1 < _visibleArtistLinks || more) {
				p.drawText(QRect(rect.right() + 1, subtitle.y(), separatorWidth, subtitle.height()),
					Qt::AlignVCenter | Qt::AlignLeft, separator);
			}
		}
		if (more) {
			p.drawText(_moreArtists->geometry(), Qt::AlignCenter, u"…"_q);
		}
	}

	Ui::AbstractButton *_titleLink = nullptr;
	QStringList _artistNames;
	std::vector<Ui::AbstractButton*> _artistLinks;
	Ui::AbstractButton *_moreArtists = nullptr;
	int _visibleArtistLinks = 0;
	base::unique_qptr<Ui::PopupMenu> _artistsMenu;
	void updateAnimation();
	QToolButton *_menuButton = nullptr;

	[[nodiscard]] QRect heartRect() const {
		if (_mode == RowMode::Home) {
			const auto inset = style::ConvertScale(8);
			const auto side = style::ConvertScale(28);
			return QRect(width() - inset - side, height() - inset - side, side, side);
		}
		if (_grouped || _compactQueue) {
			const auto side = listMetric(_compactQueue ? 32 : kGroupedTrackActionSize);
			const auto durationColumn = listMetric(_compactQueue ? 44 : kGroupedTrackDurationColumn);
			return QRect(durationRight() - durationColumn - side,
				(height() - side) / 2, side, side);
		}
		const auto durationWidth = st::normalFont->width(FormatDuration(_track.durationMs));
		const auto side = listMetric(28);
		return QRect(
			durationRight() - durationWidth - listMetric(14) - side,
			(height() - side) / 2,
			side,
			side);
	}

	void showMenu(
			not_null<Window::SessionController*> controller,
			int index,
			QContextMenuEvent *event) {
		_menu = base::make_unique_q<Ui::PopupMenu>(
			this,
			st::popupMenuWithIcons);
		const auto track = _track;
		if (!track.unavailable) AddRadioAction(_menu.get(), controller, track);
		const auto add = [&](const QString &text, Fn<void()> callback, const style::icon *icon) {
			_menu->addAction(text, std::move(callback), icon);
		};
		const auto popup = [&] {
			_menu->popup(event->reason() == QContextMenuEvent::Keyboard
				? mapToGlobal(rect().center())
				: event->globalPos());
		};
		add(QString::fromUtf8(Mokagram::Lang::Tr("Воспроизвести")), [=] {
			if (_mode == RowMode::Queue || _mode == RowMode::QueueUpcoming) {
				PlayIndex(index);
			} else if (_mode == RowMode::Context) {
				PlayContext(index);
			} else if (_mode == RowMode::QueueHistory) {
				playRecent();
			} else {
				if (_results) {
					QueueState().results = *_results;
				}
				PlayFromResults(index);
			}
		}, &st::menuIconSoundOn);
		if (CanPlayTrack(track)) {
			add(IsLiked(track.id)
				? QString::fromUtf8(Mokagram::Lang::Tr("Убрать из избранного"))
				: QString::fromUtf8(Mokagram::Lang::Tr("В избранное")), [=] {
				ToggleTrackLike(track, controller, this);
			}, &st::menuIconFave);
			AddToPlaylistMenu(_menu.get(), controller, track);
		}
		if (!_playlistId.isEmpty()) {
			add(PlaylistText("Убрать из плейлиста"), [=] {
				RemovePlaylistTrack(_playlistId, track.id);
			}, &st::menuIconDelete);
		}
		if (_mode != RowMode::Queue && _mode != RowMode::QueueUpcoming) {
			add(QString::fromUtf8(Mokagram::Lang::Tr("Играть следующим")), [=] { EnqueueNext(track); controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Добавлено в очередь"))); }, &st::menuIconAbove);
			add(QString::fromUtf8(Mokagram::Lang::Tr("В конец очереди")), [=] { EnqueueLast(track); controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Добавлено в очередь"))); }, &st::menuIconBelow);
		} else {
			if (_mode != RowMode::QueueUpcoming || index > QueueState().index + 1) {
				add(QString::fromUtf8(Mokagram::Lang::Tr("Выше")), [=] { QueueMove(index, -1); }, &st::menuIconAbove);
			}
			add(QString::fromUtf8(Mokagram::Lang::Tr("Ниже")), [=] { QueueMove(index, 1); }, &st::menuIconBelow);
			add(QString::fromUtf8(Mokagram::Lang::Tr("Убрать из очереди")), [=] { QueueRemove(index); }, &st::menuIconDelete);
		}
		if (track.unavailable) {
			popup();
			return;
		}
		if (CanForwardTrack(track)) {
			add(QString::fromUtf8(Mokagram::Lang::Tr("Переслать")), [=] {
				ForwardTrack(controller, track);
			}, &st::menuIconForward);
		}
		if (IsTelegramTrack(track)) {
			add(QString::fromUtf8(Mokagram::Lang::Tr("Показать в чате")), [=] {
				if (const auto item = TrackMessage(track)) {
					controller->showPeerHistory(item->history(), {}, item->id);
				}
			}, &st::menuIconChats);
		} else {
			add(QString::fromUtf8(Mokagram::Lang::Tr("Скачать")), [=] { SaveToDisk(track); }, &st::menuIconDownload);
			add(QString::fromUtf8(Mokagram::Lang::Tr("Добавить в профиль")), [=] { AddToProfile(controller, track); }, &st::menuIconProfile);
			add(QString::fromUtf8(Mokagram::Lang::Tr("Копировать ссылку")), [=] {
				QGuiApplication::clipboard()->setText(track.permalink);
				controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Ссылка скопирована")));
			}, &st::menuIconLink);
		}
		popup();
	}


	void updateResultTitleScroll() {
		const auto titleRect = listTextRect(true);
		const auto scroll = isOver() && !isDisabled() && !titleRect.isEmpty()
			&& _titleFont->width(_track.title) > titleRect.width();
		if (scroll) {
			if (!_resultTitleTimer) {
				_resultTitleTimer = new QTimer(this);
				_resultTitleTimer->setTimerType(Qt::PreciseTimer);
				_resultTitleTimer->setInterval(8);
				connect(_resultTitleTimer, &QTimer::timeout, this, [this] {
					if (!isVisible() || visibleRegion().isEmpty()) {
						_resultTitleTimer->stop();
					} else {
						update(listTextRect(true));
					}
				});
			}
			if (!_resultTitleTimer->isActive()) {
				_resultTitleAt = crl::now()
					- (_hoverFade.value() > 0. ? _resultTitleElapsed : 0);
				_resultTitleElapsed = 0;
				_resultTitleTimer->start();
				update(titleRect);
			}
		} else if (_resultTitleTimer && _resultTitleTimer->isActive()) {
			_resultTitleElapsed = crl::now() - _resultTitleAt;
			_resultTitleTimer->stop();
			update(titleRect);
		}
	}

	void paint(QRect area) {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto selected = showsCurrentTrack();
		if (_mode == RowMode::Home) {
			paintHome(
				p,
				selected,
				_homePlaybackControls
					? HomeTrackPlayback(_track.id)
					: HomeTrackPlaybackState());
			return;
		}
		const auto playing = selected;
		const auto over = isOver() || isDown();
		const auto grouped = _grouped || _compactQueue;
		const auto album = (_mode == RowMode::AlbumTrack);
		if (_compactQueue) {
			if (!isDisabled() && (playing || over || hasFocus())) {
				PaintHomeSurface(p, rect(), isOver() ? 1. : 0., isDown(), hasFocus(),
					playing, st::windowBgActive->c);
			}
		} else if (_grouped) {
			PaintTrackListState(p, *this, playing);
		} else {
			const auto inset = listMetric(8);
			const auto verticalInset = listMetric(4);
			const auto bounds = rect().marginsRemoved({
				inset, verticalInset, inset, verticalInset });
			const auto highlight = playing ? 1. : _mode == RowMode::Results
				? _hoverFade.value()
				: over ? 1. : 0.;
			if (!isDisabled() && highlight > 0.) {
				p.setPen(Qt::NoPen);
				auto color = st::windowBgOver->c;
				color.setAlphaF(color.alphaF() * highlight);
				p.setBrush(color);
				const auto radius = listMetric(12);
				p.drawRoundedRect(bounds, radius, radius);
			}
			if (hasFocus()) {
				p.setPen(QPen(st::windowActiveTextFg->c, listMetric(2)));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(bounds, listMetric(8), listMetric(8));
			}
		}
		const auto art = listLeadingRect();
		if (_media) {
			if (const auto image = _media->thumbnail()) {
				_art = image->original();
			} else if (const auto image = _media->thumbnailInline()) {
				_art = image->original();
			}
		}
		auto path = QPainterPath();
		if (!album) {
			path.addRoundedRect(art, listMetric(8), listMetric(8));
		}
		if (album) {
			if (!playing) {
				p.setFont(st::normalFont);
				p.setPen(MusicSecondaryColor());
				p.drawText(art, Qt::AlignCenter, _trackNumber);
			}
		} else if (!_art.isNull()) {
			p.save();
			p.setClipPath(path);
			const auto ratio = devicePixelRatioF();
			const auto pixels = art.size() * ratio;
			if (_scaledArtKey != _art.cacheKey()
				|| _scaledArt.size() != pixels
				|| _scaledArt.devicePixelRatio() != ratio) {
				_scaledArt = _art.scaled(
					pixels, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				_scaledArt.setDevicePixelRatio(ratio);
				_scaledArtKey = _art.cacheKey();
			}
			p.drawImage(art, _scaledArt);
			if (playing) {
				p.fillRect(art, QColor(0, 0, 0, 120));
			}
			p.restore();
		} else {
			p.setPen(Qt::NoPen);
			p.setBrush(grouped ? st::boxBg : st::windowBgOver);
			p.drawPath(path);
		}
		if (playing) {
			// Три столбика «играет».
			p.setPen(Qt::NoPen);
			p.setBrush(album ? st::windowActiveTextFg->c : QColor(Qt::white));
			const auto scale = [&](int value) { return grouped ? listMetric(value) : value; };
			const auto base = art.center().y() + scale(9);
			const auto left = art.center().x() - scale(10);
			const auto step = scale(8);
			const auto barWidth = scale(4);
			const auto radius = scale(2);
			const auto heightScale = barWidth / 4.;
			for (auto i = 0; i != 3; ++i) {
				const auto wave = std::sin(_equalizerPhase * (5. + i) + i * 2.1);
				const auto barHeight = (6. + 7. * (wave + 1.)) * heightScale;
				p.drawRoundedRect(QRectF(
					left + i * step, base - barHeight, barWidth, barHeight),
					radius, radius);
			}
		}
		if (art.contains(area)) {
			return;
		}
		const auto duration = FormatDuration(_track.durationMs);
		const auto durationWidth = st::normalFont->width(duration);
		p.setFont(st::normalFont);
		const auto secondary = MusicSecondaryColor();
		p.setPen(secondary);
		p.drawText(
			QRect(durationRight() - durationWidth, 0, durationWidth, height()),
			Qt::AlignVCenter | Qt::AlignRight,
			duration);
		const auto titleRect = listTextRect(true);
		auto subtitleRect = listTextRect(false);
		p.setFont(_titleFont);
		p.setPen((grouped && isDisabled()) ? secondary
			: (playing && !grouped && _mode != RowMode::QueueCurrent && !IsLightMusicTheme())
			? st::windowActiveTextFg->c
			: st::windowFg->c);
		const auto moving = _resultTitleTimer && _resultTitleTimer->isActive();
		const auto titleBlend = (moving || _resultTitleElapsed > 0)
			? _hoverFade.value()
			: 0.;
		if (titleBlend > 0.) {
			p.save();
			p.setOpacity(p.opacity() * titleBlend);
			PaintMovingTitle(p, titleRect, _track.title, p.font(),
				moving ? crl::now() - _resultTitleAt : _resultTitleElapsed,
				devicePixelRatioF(), _resultTitlePath);
			p.restore();
		}
		if (titleBlend < 1.) {
			p.save();
			p.setOpacity(p.opacity() * (1. - titleBlend));
			p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft,
				_titleFont->elided(_track.title, titleRect.width()));
			p.restore();
		}
		p.setFont(st::normalFont);
		p.setPen(secondary);
		if ((IsDetailTrackRow(_mode) && !isDisabled()) || (_moreArtists && CanPlayTrack(_track))) {
			if (!subtitleRect.isEmpty()) {
				paintArtistLinks(p, subtitleRect);
			}
			return;
		}
		const auto item = TrackMessage(_track);
		const auto subtitle = !CanPlayTrack(_track)
			? (IsDetailTrackRow(_mode) ? QString() : _track.artist + u" · "_q)
				+ QString::fromUtf8(Mokagram::Lang::Tr("Недоступен для воспроизведения"))
			: item
			? _track.artist + u" · "_q + item->history()->peer->name()
			: _artistNames.value(0, _track.artist);
		if (!_playsText.isEmpty()) {
			const auto countWidth = st::normalFont->width(_playsText);
			if (countWidth <= subtitleRect.width()) {
				p.drawText(subtitleRect, Qt::AlignVCenter | Qt::AlignRight, _playsText);
				subtitleRect.setWidth(std::max(0, subtitleRect.width() - countWidth - style::ConvertScale(8)));
			}
		}
		p.drawText(
			subtitleRect,
			Qt::AlignVCenter | Qt::AlignLeft,
			st::normalFont->elided(subtitle, subtitleRect.width()));
	}

	void paintHome(
			QPainter &p,
			bool selected,
			HomeTrackPlaybackState playback) {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto art = HomeCardArtRect(width());
		const auto current = selected || playback.current;
		const auto disabled = isDisabled();
		PaintHomeSurface(p, rect(), disabled ? 0. : _hoverFade.value(),
			!disabled && isDown(), !disabled && hasFocus(), current);
		if (disabled) {
			p.setOpacity(.38);
		}
		if (_media) {
			if (const auto image = _media->thumbnail()) {
				_art = image->original();
			} else if (const auto image = _media->thumbnailInline()) {
				_art = image->original();
			}
		}
		PaintHomeCover(p, art, _art);
		if (_playButton) {
			_playButton->setPresentation(
				{ current, _homePlaybackControls && playback.playing },
				(hasFocus() || playback.current) ? 1. : _hoverFade.value());
		}
		const auto textWidth = std::max(art.width() - (_likeButton ? s(30) : 0), 0);
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRect(art.left(), art.bottom() + s(8), art.width(), s(22)),
			Qt::AlignVCenter | Qt::AlignLeft,
			st::semiboldFont->elided(_track.title, art.width()));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		p.drawText(QRect(art.left(), art.bottom() + s(30), textWidth, s(22)),
			Qt::AlignVCenter | Qt::AlignLeft,
			st::normalFont->elided(_track.artist, textWidth));

	}

	QTimer *_resultTitleTimer = nullptr;
	crl::time _resultTitleAt = 0;
	crl::time _resultTitleElapsed = 0;
	MovingTitlePath _resultTitlePath;
	QTimer _equalizerTimer;
	QElapsedTimer _equalizerClock;
	double _equalizerPhase = 0.;
	Track _track;
	QString _playsText;
	RowMode _mode;
	const bool _grouped;
	const bool _compactQueue;
	style::font _titleFont;
	QString _trackNumber;
	const int _queueIndex;
	bool _queueDragging = false;
	std::shared_ptr<std::vector<Track>> _results;
	bool _homePlaybackControls = true;
	TrackLikeButton *_likeButton = nullptr;
	HomeCardPlayButton *_playButton = nullptr;
	QImage _art;
	QImage _scaledArt;
	qint64 _scaledArtKey = 0;
	std::shared_ptr<Data::DocumentMedia> _media;
	base::unique_qptr<Ui::PopupMenu> _menu;
	QString _playlistId;
	Ui::Animations::Simple _likeAnimation;

	HoverFade _hoverFade;

};

class HomeSectionHeader final : public Ui::AbstractButton {
public:
	HomeSectionHeader(
		QWidget *parent,
		QString title,
		Fn<void()> action,
		bool create = false)
	: AbstractButton(parent)
	, _title(std::move(title))
	, _create(create) {
		resize(width(), style::ConvertScale(56));
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(_create ? _title
			: PlaylistText("Назад") + u": "_q + _title);
		setClickedCallback(std::move(action));
		paintRequest() | rpl::on_next([=](QRect) {
			auto p = QPainter(this);
			const auto s = [](int v) { return style::ConvertScale(v); };
			auto font = st::semiboldFont->f;
			font.setPixelSize(s(20));
			p.setFont(font);
			if (hasFocus()) {
				p.setPen(QPen(st::windowActiveTextFg->c, s(2)));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(rect().adjusted(s(2), s(2), -s(2), -s(2)), s(6), s(6));
			}
			p.setPen(st::windowFg);
			const auto title = (_create ? u"+  "_q : u"‹  "_q) + _title;
			p.save();
			const auto scale = 1. - 0.03 * _press.value(_pressed ? 1. : 0.);
			p.translate(s(12), height() / 2.);
			p.scale(scale, scale);
			p.translate(-s(12), -height() / 2.);
			p.drawText(QRect(s(12), 0, std::max(width() - s(140), 0), height()),
				Qt::AlignVCenter | Qt::AlignLeft,
				QFontMetrics(font).elidedText(title, Qt::ElideRight, std::max(width() - s(140), 0)));
			p.restore();
		}, lifetime());
	}

protected:
	void mousePressEvent(QMouseEvent *event) override {
		_pointerPress = true;
		AbstractButton::mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) override {
		_pointerPress = false;
		AbstractButton::mouseReleaseEvent(event);
	}
	void onStateChanged(State was, StateChangeSource source) override {
		AbstractButton::onStateChanged(was, source);
		if (!((was ^ state()) & StateFlag::Down)) {
			return;
		}
		const auto pressed = isDown() && _pointerPress;
		if (pressed == _pressed) {
			return;
		}
		const auto from = _press.value(_pressed ? 1. : 0.);
		_pressed = pressed;
		if (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations)) {
			_press.stop();
			update();
		} else {
			_press.start([=] { update(); }, from, pressed ? 1. : 0., 140, anim::easeOutQuint);
		}
	}

private:
	QString _title;
	bool _create = false;
	bool _pointerPress = false;
	bool _pressed = false;
	Ui::Animations::Simple _press;

};

class HomeTonalControl : public Ui::AbstractButton {
public:
	explicit HomeTonalControl(QWidget *parent) : AbstractButton(parent) {
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
	}

protected:
	void onStateChanged(State, StateChangeSource) override {
		update();
	}
};

constexpr auto kHomeShelfActionInset = 16;

class HomeShelfAction final : public HomeTonalControl {
public:
	HomeShelfAction(
		QWidget *parent,
		QString label,
		const QString &section,
		Fn<void()> callback)
	: HomeTonalControl(parent)
	, _label(std::move(label)) {
		setAccessibleName(_label + u": "_q + section);
		setClickedCallback(std::move(callback));
		setNaturalWidth(st::semiboldFont->width(_label)
			+ 2 * style::ConvertScale(kHomeShelfActionInset));
		resize(naturalWidth(), style::ConvertScale(40));
	}

protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		const auto hovered = isOver() || isDown();
		const auto focused = hasFocus();
		if (hovered || focused) {
			auto hq = PainterHighQualityEnabler(p);
			const auto stroke = style::ConvertScale(2);
			const auto inset = focused ? stroke / 2. : 0.;
			const auto shape = QRectF(rect()).adjusted(inset, inset, -inset, -inset);
			auto color = st::windowFg->c;
			color.setAlphaF(isDown() ? .12 : .08);
			p.setBrush(hovered ? QBrush(color) : QBrush(Qt::NoBrush));
			p.setPen(focused ? QPen(st::windowActiveTextFg->c, stroke) : QPen(Qt::NoPen));
			const auto radius = shape.height() / 2.;
			p.drawRoundedRect(shape, radius, radius);
		}
		const auto inset = style::ConvertScale(kHomeShelfActionInset);
		const auto content = rect().adjusted(inset, 0, -inset, 0);
		if (!content.isEmpty()) {
			const auto font = st::semiboldFont;
			p.setFont(font);
			p.setPen(st::windowFg);
			p.drawText(content, Qt::AlignCenter, font->elided(_label, content.width()));
		}
	}

private:
	QString _label;
};

class HomeShelfHeader final : public Ui::RpWidget {
public:
	HomeShelfHeader(QWidget *parent, QString title, Fn<void()> open = nullptr)
	: RpWidget(parent)
	, _title(std::move(title)) {
		resize(width(), style::ConvertScale(56));
		if (open) {
			_action = Ui::CreateChild<HomeShelfAction>(
				this, PlaylistText("Показать все"), _title, std::move(open));
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		if (_action) {
			const auto width = std::clamp(newWidth - 2 * side, 0, _action->naturalWidth());
			_action->setGeometry(newWidth - side - width,
				s(8), width, _action->height());
		}
		return s(56);
	}

	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		const auto right = _action ? _action->x() - s(8) : width() - side;
		const auto available = std::max(right - side, 0);
		auto font = st::semiboldFont->f;
		font.setPixelSize(s(20));
		p.setFont(font);
		p.setPen(st::windowFg);
		p.drawText(QRect(side, 0, available, height()), Qt::AlignVCenter | Qt::AlignLeft,
			QFontMetrics(font).elidedText(_title, Qt::ElideRight, available));
	}

private:
	QString _title;
	Ui::AbstractButton *_action = nullptr;
};

class PlaylistRow final : public Ui::AbstractButton {
public:
	void onStateChanged(State was, StateChangeSource source) override {
		const auto active = isOver() || isDown();
		if (active == _hoverTarget) return;
		const auto from = _hover.value(_hoverTarget ? 1. : 0.);
		_hoverTarget = active;
		if (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations)) {
			_hover.stop();
			update();
		} else {
			_hover.start([=] { update(); }, from, active ? 1. : 0., 160, anim::easeOutCubic);
		}
	}
	PlaylistRow(QWidget *parent, not_null<Window::SessionController*> controller, Playlist playlist, Fn<void(QString)> open)
	: AbstractButton(parent), _controller(controller), _playlist(std::move(playlist)) {
		const auto s = [](int v) { return style::ConvertScale(v); };
		resize(width(), s(92));
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(_playlist.name);
		setClickedCallback([=] { open(_playlist.id); });
		_menuButton = new TrackMenuButton(this);
		_menuButton->setAccessibleName(PlaylistText("Действия с плейлистом"));
		_menuButton->setToolTip(PlaylistText("Действия с плейлистом"));
		_menuButton->setVisible(!_playlist.remote);
		connect(_menuButton, &QToolButton::clicked, this, [=] {
			showMenu(_menuButton->mapToGlobal(_menuButton->rect().bottomLeft()));
		});
		for (const auto &track : _playlist.tracks) {
			if (track.artworkUrl.isEmpty()) continue;
			const auto url = ArtUrl(track.artworkUrl, u"t200x200"_q);
			if (std::find(_urls.begin(), _urls.end(), url) != _urls.end()) continue;
			const auto index = int(_urls.size());
			_urls.push_back(url);
			LoadImage(url, crl::guard(this, [=](QImage image) {
				_art[index] = std::move(image);
				if (!_art[index].isNull() && isVisible() && !anim::Disabled()
					&& !PowerSaving::On(PowerSaving::kAnimations)) {
					_artFade[index].start([=] { update(); }, 0., 1., 220, anim::easeOutCubic);
				}
				update();
			}));
			if (_urls.size() == _art.size()) break;
		}
		paintRequest() | rpl::on_next([=](QRect) { paint(); }, lifetime());
	}
protected:
	void resizeEvent(QResizeEvent *event) override {
		AbstractButton::resizeEvent(event);
		const auto s = [](int v) { return style::ConvertScale(v); };
		_menuButton->setGeometry(width() - s(54), (height() - s(32)) / 2, s(32), s(32));
	}
	void contextMenuEvent(QContextMenuEvent *event) override {
		if (_playlist.remote) {
			event->accept();
			return;
		}
		showMenu(event->reason() == QContextMenuEvent::Keyboard
			? mapToGlobal(rect().center()) : event->globalPos());
		event->accept();
	}
	void keyPressEvent(QKeyEvent *event) override {
		if (_playlist.remote) {
			AbstractButton::keyPressEvent(event);
			return;
		}
		if (event->key() == Qt::Key_Menu
			|| (event->key() == Qt::Key_F10 && event->modifiers() == Qt::ShiftModifier)) {
			showMenu(mapToGlobal(rect().center()));
			event->accept();
		} else {
			AbstractButton::keyPressEvent(event);
		}
	}
private:
	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto card = rect().adjusted(s(8), s(4), -s(8), -s(4));
		const auto hover = (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations))
			? (isOver() || isDown() ? 1. : 0.)
			: _hover.value(isOver() || isDown() ? 1. : 0.);
		if (hover > 0.) {
			p.save();
			p.setOpacity(hover);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(card, s(12), s(12));
			p.restore();
		}
		if (hasFocus()) {
			p.setPen(QPen(st::windowActiveTextFg->c, s(2)));
			p.setBrush(Qt::NoBrush);
			p.drawRoundedRect(card.adjusted(s(1), s(1), -s(1), -s(1)), s(12), s(12));
		}
		const auto side = s(68);
		const auto cover = QRect(s(16), (height() - side) / 2, side, side);
		auto path = QPainterPath();
		path.addRoundedRect(cover, s(9), s(9));
		p.save();
		p.setClipPath(path);
		p.fillRect(cover, QColor(49, 57, 70));
		if (_urls.empty()) {
			p.setPen(QColor(181, 199, 221));
			auto font = st::semiboldFont->f;
			font.setPixelSize(s(30));
			p.setFont(font);
			p.drawText(cover, Qt::AlignCenter, QString::fromUtf8("♫"));
		} else {
			const auto count = (_urls.size() == 1) ? 1 : 4;
			for (auto i = 0; i != count; ++i) {
				const auto &image = _art[i % _urls.size()];
				const auto x = count == 1 ? 0 : i % 2;
				const auto y = count == 1 ? 0 : i / 2;
				const auto cell = count == 1 ? cover : QRect(
					cover.x() + x * (side / 2), cover.y() + y * (side / 2),
					x ? side - side / 2 : side / 2, y ? side - side / 2 : side / 2);
				p.fillRect(cell, (i % 2) ? QColor(57, 65, 79) : QColor(68, 78, 94));
				if (!image.isNull()) {
					p.save();
					p.setOpacity((anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations))
						? 1. : _artFade[i % _urls.size()].value(1.));
					const auto dimension = std::min(image.width(), image.height());
					const auto source = QRect((image.width() - dimension) / 2,
						(image.height() - dimension) / 2, dimension, dimension);
					p.drawImage(cell, image, source);
					p.restore();
				}
			}
		}
		p.restore();
		const auto left = cover.right() + s(18);
		const auto textWidth = std::max(width() - left - s(70), 0);
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRect(left, height() / 2 - s(21), textWidth, s(23)), Qt::AlignVCenter,
			p.fontMetrics().elidedText(_playlist.name, Qt::ElideRight, textWidth));
		p.setFont(st::normalFont);
		p.setPen(st::windowSubTextFg);
		p.drawText(QRect(left, height() / 2 + s(3), textWidth, s(20)), Qt::AlignVCenter,
			PlaylistText("Треков: %1").arg(int(_playlist.tracks.size())));
	}
	void showMenu(QPoint position) {
		_menu = base::make_unique_q<Ui::PopupMenu>(this, st::popupMenuWithIcons);
		const auto id = _playlist.id;
		const auto controller = _controller;
		const auto name = _playlist.name;
		_menu->addAction(PlaylistText("Переименовать"), [=] {
			PlaylistNameBox(controller, PlaylistText("Переименовать плейлист"), name, [=](QString value) {
				RenamePlaylist(id, value);
			});
		}, &st::menuIconEdit);
		_menu->addAction(PlaylistText("Удалить плейлист"), [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = TextWithEntities{ PlaylistText("Удалить плейлист «%1»?").arg(name) },
				.confirmed = [=](Fn<void()> close) { close(); DeletePlaylist(id); },
				.confirmText = PlaylistText("Удалить"),
				.cancelText = PlaylistText("Отмена"),
				.title = rpl::single(PlaylistText("Удалить плейлист")),
			}));
		}, &st::menuIconDelete);
		_menu->popup(position);
	}
	const not_null<Window::SessionController*> _controller;
	Playlist _playlist;
	std::vector<QString> _urls;
	std::array<QImage, 4> _art;
	std::array<Ui::Animations::Simple, 4> _artFade;
	Ui::Animations::Simple _hover;
	bool _hoverTarget = false;
	QToolButton *_menuButton = nullptr;
	base::unique_qptr<Ui::PopupMenu> _menu;
};

void Row::updateAnimation() {
	if (_mode == RowMode::Home) {
		return;
	}
	const auto eligible = isVisible()
		&& QGuiApplication::applicationState() == Qt::ApplicationActive
		&& showsCurrentTrack()
		&& !PowerSaving::On(PowerSaving::kAnimations);
	if (!eligible) {
		_equalizerTimer.stop();
		return;
	}
	const auto state = Media::Player::instance()->getState(AudioMsgId::Type::Song);
	const auto animate = (_mode == RowMode::QueueCurrent || UsesQueue(state.id))
		&& (state.state == Media::Player::State::Playing
			|| state.state == Media::Player::State::Starting
			|| state.state == Media::Player::State::Resuming);
	if (animate && !_equalizerTimer.isActive()) {
		_equalizerClock.start();
		_equalizerTimer.start();
	} else if (!animate) {
		_equalizerTimer.stop();
	}
}

void Row::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Menu
		|| (e->key() == Qt::Key_F10 && e->modifiers() == Qt::ShiftModifier)) {
		auto event = QContextMenuEvent(
			QContextMenuEvent::Keyboard,
			rect().center(),
			mapToGlobal(rect().center()));
		QCoreApplication::sendEvent(this, &event);
		e->accept();
	} else {
		AbstractButton::keyPressEvent(e);
	}
}

void Row::showEvent(QShowEvent *e) {
	AbstractButton::showEvent(e);
	updateAnimation();
}

void Row::hideEvent(QHideEvent *e) {
	_equalizerTimer.stop();
	AbstractButton::hideEvent(e);
}

// Альбом в результатах поиска: обложка, название, исполнитель и число треков.
class AlbumRow final : public Ui::AbstractButton {
public:
	AlbumRow(QWidget *parent, not_null<Window::SessionController*> controller, const Album &album, Fn<void()> open)
	: AbstractButton(parent)
	, _album(album) {
		resize(width(), kRowHeight);
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Альбом"))
			+ u": "_q + _album.title + u" - "_q + _album.artist);
		setClickedCallback(std::move(open));
		InstallAlbumContextMenu(this, controller, album);
		LoadImage(ArtUrl(album.artworkUrl, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			update();
		}));
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
	}

private:
	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		if (isOver() || isDown()) {
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(rect().marginsRemoved({ 8, 4, 8, 4 }), 12, 12);
		}
		if (hasFocus()) {
			p.setPen(QPen(st::windowActiveTextFg->c, 2));
			p.setBrush(Qt::NoBrush);
			p.drawRoundedRect(rect().adjusted(8, 4, -8, -4), 8, 8);
		}
		const auto art = QRect(20, (height() - kArtSize) / 2, kArtSize, kArtSize);
		auto path = QPainterPath();
		path.addRoundedRect(art, 10, 10);
		if (_art.isNull()) {
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawPath(path);
		} else {
			p.save();
			p.setClipPath(path);
			p.drawImage(art, _art);
			p.restore();
		}
		const auto left = art.right() + 16;
		const auto right = width() - 24;
		// Стрелка «открыть».
		p.setPen(QPen(st::windowSubTextFg, 2., Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.setBrush(Qt::NoBrush);
		const auto cy = height() / 2.;
		p.drawPolyline(QPolygonF({
			QPointF(right - 5, cy - 6),
			QPointF(right + 1, cy),
			QPointF(right - 5, cy + 6) }));
		const auto textWidth = right - 20 - left;
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(
			QRect(left, height() / 2 - 24, textWidth, 22),
			Qt::AlignVCenter | Qt::AlignLeft,
			st::semiboldFont->elided(_album.title, textWidth));
		p.setFont(st::normalFont);
		p.setPen(st::windowSubTextFg);
		const auto subtitle = _album.artist
			+ u" · "_q
			+ QString::fromUtf8(Mokagram::Lang::Tr("треков: %1")).arg(_album.trackCount);
		p.drawText(
			QRect(left, height() / 2 + 2, textWidth, 20),
			Qt::AlignVCenter | Qt::AlignLeft,
			st::normalFont->elided(subtitle, textWidth));
	}

	Album _album;
	QImage _art;

};

[[nodiscard]] QString HomeReleaseKindText(ReleaseKind kind) {
	switch (kind) {
	case ReleaseKind::Single: return PlaylistText("Сингл");
	case ReleaseKind::Ep: return PlaylistText("EP");
	case ReleaseKind::Compilation: return PlaylistText("Сборник");
	case ReleaseKind::Unknown:
	case ReleaseKind::Album: return PlaylistText("Альбом");
	}
	Unexpected("Release kind.");
}

class HomeAlbumCard final : public Ui::AbstractButton {
public:
	void onStateChanged(State was, StateChangeSource source) override {
		_hoverFade.set(isOver() || isDown(), [=] { update(); });
		update();
	}

	HomeAlbumCard(QWidget *parent, not_null<Window::SessionController*> controller, Album album, Fn<void()> open)
	: AbstractButton(parent)
	, _album(std::move(album)) {
		if (!_album.id.isEmpty()) {
			_play = Ui::CreateChild<HomeCardPlayButton>(this, _album.title);
			connect(_play, &QToolButton::clicked, this, [=] {
				if (!_play->loading()) {
					_play->setLoading(true);
					PlayHomeAlbum(controller, _album, crl::guard(_play, [button = _play] {
						button->setLoading(false);
					}));
				}
			});
			const auto refreshPlayback = [=] {
				const auto playback = HomeAlbumPlayback(_album.id);
				if (_playback != playback) {
					_playback = playback;
					update();
				}
			};
			HomePlaybackChanges() | rpl::on_next(refreshPlayback, lifetime());
			QueueState().changes.events(
			) | rpl::on_next(refreshPlayback, lifetime());
			refreshPlayback();
		}
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(HomeReleaseKindText(_album.releaseKind) + u" · "_q + _album.title
			+ u" · "_q + _album.artist);
		setClickedCallback(std::move(open));
		InstallAlbumContextMenu(this, controller, _album);
		LoadImage(ArtUrl(_album.artworkUrl, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			update();
		}));
	}

protected:
	void resizeEvent(QResizeEvent *event) override {
		AbstractButton::resizeEvent(event);
		if (_play) {
			_play->layoutOnCover(width());
		}
	}

	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto art = HomeCardArtRect(width());
		PaintHomeSurface(p, rect(), _hoverFade.value(), isDown(), hasFocus(), _playback.current);
		PaintHomeCover(p, art, _art);
		if (_play) {
			_play->setPresentation(_playback,
				(hasFocus() || _playback.current) ? 1. : _hoverFade.value());
		}
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRect(art.left(), art.bottom() + s(8), art.width(), s(22)),
			Qt::AlignVCenter | Qt::AlignLeft, st::semiboldFont->elided(_album.title, art.width()));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		const auto kind = HomeReleaseKindText(_album.releaseKind);
		const auto details = _album.releaseYear > 0
			? QString::number(_album.releaseYear) + u" · "_q + kind
			: kind + u" · "_q + _album.artist;
		p.drawText(QRect(art.left(), art.bottom() + s(30), art.width(), s(22)),
			Qt::AlignVCenter | Qt::AlignLeft, st::normalFont->elided(details, art.width()));
	}

private:
	Album _album;
	QImage _art;
	HomeCardPlayButton *_play = nullptr;
	HomeTrackPlaybackState _playback;

	HoverFade _hoverFade;

};

class HomeRecentTile final : public Ui::AbstractButton {
public:
	void onStateChanged(State was, StateChangeSource source) override {
		_hoverFade.set(isOver() || isDown(), [=] { update(); });
		update();
	}

	HomeRecentTile(QWidget *parent, not_null<Window::SessionController*> controller, RecentItem item, Fn<void()> activate, Fn<void()> play)
	: AbstractButton(parent)
	, _item(std::move(item)) {
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		const auto title = _item.isAlbum ? _item.album.title : _item.track.title;
		const auto artist = _item.isAlbum ? _item.album.artist : _item.track.artist;
		setAccessibleName((_item.isAlbum
			? QString::fromUtf8(Mokagram::Lang::Tr("Альбом")) + u": "_q
			: QString()) + title + u" - "_q + artist);
		setClickedCallback([=] {
			if (actionRect().contains(mapFromGlobal(QCursor::pos()))) {
				if (!_item.isAlbum
					&& HomeTrackPlayback(_item.track.id).current) {
					Media::Player::instance()->playPause(AudioMsgId::Type::Song);
				} else {
					play();
				}
			} else {
				activate();
			}
		});
		const auto refreshPlayback = [=] {
			const auto playback = _item.isAlbum
				? HomeAlbumPlayback(_item.album.id)
				: HomeTrackPlayback(_item.track.id);
			if (_current != playback.current || _playing != playback.playing) {
				_current = playback.current;
				_playing = playback.playing;
				update();
			}
		};
		HomePlaybackChanges() | rpl::on_next(refreshPlayback, lifetime());
		QueueState().changes.events(
		) | rpl::on_next(refreshPlayback, lifetime());
		refreshPlayback();
		const auto recentItem = _item;
		InstallMusicContextMenu(this, [=](not_null<Ui::PopupMenu*> menu) {
			const auto pause = recentItem.isAlbum && HomeAlbumPlayback(recentItem.album.id).playing;
			menu->addAction(PlaylistText(pause ? "Пауза" : "Воспроизвести"), play, &st::menuIconSoundOn);
			if (recentItem.isAlbum) {
				menu->addAction(PlaylistText("Открыть альбом"), activate, &st::menuIconSearch);
				AddAlbumToPlaylistMenu(menu, controller, recentItem.album);
				return;
			}
			const auto track = recentItem.track;
			AddRadioAction(menu, controller, track);
			menu->addAction(IsLiked(track.id) ? PlaylistText("Убрать из избранного") : PlaylistText("В избранное"), [=] {
				ToggleTrackLike(track, controller, this);
			}, &st::menuIconFave);
			AddToPlaylistMenu(menu, controller, track);
			menu->addAction(PlaylistText("Играть следующим"), [=] {
				EnqueueNext(track); controller->showToast(PlaylistText("Добавлено в очередь"));
			}, &st::menuIconAbove);
			menu->addAction(PlaylistText("В конец очереди"), [=] {
				EnqueueLast(track); controller->showToast(PlaylistText("Добавлено в очередь"));
			}, &st::menuIconBelow);
			if (CanForwardTrack(track)) menu->addAction(PlaylistText("Переслать"), [=] { ForwardTrack(controller, track); }, &st::menuIconForward);
			menu->addAction(PlaylistText("Скачать"), [=] { SaveToDisk(track); }, &st::menuIconDownload);
			menu->addAction(PlaylistText("Добавить в профиль"), [=] { AddToProfile(controller, track); }, &st::menuIconProfile);
			menu->addAction(PlaylistText("Копировать ссылку"), [=] {
				QGuiApplication::clipboard()->setText(track.permalink);
				controller->showToast(PlaylistText("Ссылка скопирована"));
			}, &st::menuIconLink);
		});
		const auto url = _item.isAlbum ? _item.album.artworkUrl : _item.track.artworkUrl;
		LoadImage(ArtUrl(url, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			update();
		}));
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		PaintHomeSurface(p, rect(), _hoverFade.value(), isDown(), hasFocus(), _current);
		const auto inset = s(kHomeCardInset);
		const auto side = std::max(std::min(s(44), width() - 2 * inset), 0);
		const auto art = QRect(inset, (height() - side) / 2, side, side);
		PaintHomeCover(p, art, _art);
		const auto action = actionRect();
		PaintHomeCardAction(p, action,
			(hasFocus() || _current) ? 1. : _hoverFade.value(), _playing);
		const auto left = art.left() + art.width() + s(12);
		const auto available = std::max(action.left() - s(8) - left, 0);
		const auto textHeight = s(22);
		const auto top = height() / 2 - textHeight;
		const auto &title = _item.isAlbum ? _item.album.title : _item.track.title;
		const auto &artist = _item.isAlbum ? _item.album.artist : _item.track.artist;
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRect(left, top, available, textHeight), Qt::AlignLeft | Qt::AlignVCenter,
			st::semiboldFont->elided(title, available));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		p.drawText(QRect(left, top + textHeight, available, textHeight),
			Qt::AlignLeft | Qt::AlignVCenter, st::normalFont->elided(artist, available));
	}

private:
	[[nodiscard]] QRect actionRect() const {
		const auto side = style::ConvertScale(38);
		return QRect(width() - style::ConvertScale(16) - side,
			(height() - side) / 2, side, side);
	}

	RecentItem _item;
	QImage _art;
	bool _current = false;
	bool _playing = false;

	HoverFade _hoverFade;

};

class HomeArtistCard final : public Ui::AbstractButton {
public:
	void onStateChanged(State was, StateChangeSource source) override {
		AbstractButton::onStateChanged(was, source);
		_hoverFade.set(isOver() || isDown(), [=] { update(); });
		update();
	}

	HomeArtistCard(QWidget *parent, Track track, Fn<void(Track)> open)
	: AbstractButton(parent), _track(std::move(track)) {
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Исполнитель")) + u": "_q + _track.artist);
		setClickedCallback([=] { open(_track); });
		const auto url = _track.artistArtworkUrl.isEmpty() ? _track.artworkUrl : _track.artistArtworkUrl;
		LoadImage(ArtUrl(url, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			_scaledArt = QImage();
			update();
		}));
	}
protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		PaintHomeSurface(p, rect(), _hoverFade.value(), isDown(), hasFocus());
		const auto art = HomeCardArtRect(width());
		p.save();
		auto clip = QPainterPath();
		clip.addEllipse(art);
		p.setClipPath(clip);
		if (_art.isNull()) {
			p.fillRect(art, st::windowBgOver->c);
		} else {
			if (_scaledArt.isNull() || _scaledArtSize != art.size()) {
				_scaledArt = _art.scaled(art.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
				_scaledArtSize = art.size();
			}
			p.drawImage(art, _scaledArt, QRect((_scaledArt.width() - art.width()) / 2, (_scaledArt.height() - art.height()) / 2, art.width(), art.height()));
		}
		p.restore();
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(QRect(art.left(), art.bottom() + s(8), art.width(), s(22)),
			Qt::AlignVCenter, st::semiboldFont->elided(_track.artist, art.width()));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		p.drawText(QRect(art.left(), art.bottom() + s(30), art.width(), s(22)),
			Qt::AlignVCenter, QString::fromUtf8(Mokagram::Lang::Tr("Исполнитель")));
	}
private:
	Track _track;
	QImage _art;
	QImage _scaledArt;
	QSize _scaledArtSize;
	HoverFade _hoverFade;
};

[[nodiscard]] std::vector<Track> HomeRecommendationArtists(const std::vector<Track> &tracks) {
	auto result = std::vector<Track>();
	auto names = std::set<QString>();
	for (const auto &track : tracks) {
		const auto name = track.artist.trimmed();
		if (!name.isEmpty() && !track.artistId.isEmpty() && track.artistId != u"0"_q
			&& names.insert(name.toCaseFolded()).second) {
			result.push_back(track);
		}
	}
	return result;
}

[[nodiscard]] std::vector<Track> HomeSimilarArtists(
		const ArtistInfo &artist,
		const std::vector<Track> &tracks) {
	auto result = HomeRecommendationArtists(tracks);
	const auto artistName = artist.name.trimmed();
	std::erase_if(result, [&](const Track &candidate) {
		return (!artist.id.isEmpty() && candidate.artistId == artist.id)
			|| (!artistName.isEmpty()
				&& candidate.artist.trimmed().compare(artistName, Qt::CaseInsensitive) == 0);
	});
	return result;
}

[[nodiscard]] bool SameHomeTrack(const Track &a, const Track &b) {
	if (!a.id.isEmpty() && a.id == b.id) {
		return true;
	}
	const auto normalize = [](QString text) {
		return text.simplified().toCaseFolded();
	};
	if (normalize(a.title) != normalize(b.title)) {
		return false;
	}
	if (a.durationMs > 0 && b.durationMs > 0
		&& std::abs(a.durationMs - b.durationMs) > 5000) {
		return false;
	}
	const auto names = TrackArtistNames(b);
	return std::ranges::any_of(TrackArtistNames(a), [&](const QString &credit) {
		return std::ranges::any_of(names, [&](const QString &other) {
			return normalize(credit) == normalize(other);
		});
	});
}

// Каталог профиля с дополнением YouTube Music, если оно разрешено настройками.
[[nodiscard]] std::vector<Track> HomeArtistPopular(
		const std::vector<Track> &tracks,
		const std::vector<Track> &youtube) {
	auto result = tracks;
	if (Settings::Bool(u"player.hide_youtube_fallback"_q)) {
		return result;
	}
	for (const auto &track : youtube) {
		const auto known = std::ranges::any_of(result, [&](const Track &other) {
			return SameHomeTrack(track, other);
		});
		if (!known) {
			result.push_back(track);
		}
	}
	return result;
}

class HomeRecentGrid final : public Ui::RpWidget {
public:
	HomeRecentGrid(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		const std::vector<RecentItem> &recent,
		Fn<void(Album)> open,
		Fn<void(Track)> openTrack)
	: RpWidget(parent) {
		const auto tracks = std::make_shared<std::vector<Track>>();
		for (const auto &item : recent) {
			if (!item.isAlbum) tracks->push_back(item.track);
		}
		auto trackIndex = 0;
		const auto count = std::min(int(recent.size()), 8);
		for (auto i = 0; i != count; ++i) {
			const auto item = recent[i];
			if (item.isAlbum) {
				_items.push_back(Ui::CreateChild<HomeRecentTile>(this, controller, item,
					[=] { open(item.album); }, [=] { PlayHomeAlbum(controller, item.album); }));
			} else {
				const auto index = trackIndex++;
				_items.push_back(Ui::CreateChild<HomeRecentTile>(this, controller, item,
					[=] { openTrack(item.track); }, [=] {
					QueueState().results = *tracks;
					QueueState().custom = false;
					PlayFromResults(index);
				}));
			}
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto count = int(_items.size());
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto gap = s(kHomeCardGap);
		const auto side = s(BoxParts::kOuterSide);
		const auto available = std::max(newWidth - 2 * side, 1);
		const auto columns = std::clamp((available + gap) / (s(240) + gap), 1, 4);
		const auto itemWidth = (available - (columns - 1) * gap) / columns;
		for (auto i = 0; i != count; ++i) {
			_items[i]->setGeometry(side + (i % columns) * (itemWidth + gap),
				(i / columns) * (s(64) + gap), itemWidth, s(64));
		}
		return ((count + columns - 1) / columns) * (s(64) + gap) + s(8);
	}

private:
	std::vector<HomeRecentTile*> _items;

};

class HomeCarouselArrow final : public HomeTonalControl {
public:
	HomeCarouselArrow(QWidget *parent, bool forward, Fn<void()> action)
	: HomeTonalControl(parent)
	, _forward(forward) {
		setAccessibleName(PlaylistText(forward ? "Вперёд" : "Назад"));
		setClickedCallback(std::move(action));
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		PaintHomeSurface(p, rect(), isOver() ? 1. : 0., isDown(), hasFocus());
		// Lucide chevron-right, https://lucide.dev/icons/chevron-right (ISC).
		constexpr auto chevron = R"svg(<svg viewBox="0 0 24 24" fill="none"><path d="m9 18 6-6-6-6" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/></svg>)svg";
		const auto center = QRectF(rect()).center();
		p.translate(center);
		if (!_forward) p.scale(-1., 1.);
		PaintIslandGlyph(p, chevron, QPointF(), s(24), st::windowFg->c);
	}

private:
	const bool _forward;

};

class HomeMixedGrid final : public Ui::RpWidget {
public:
	HomeMixedGrid(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		const std::vector<Track> &tracks,
		const std::vector<Album> &albums,
		Fn<void(Album)> open,
		Fn<void(Track)> openTrack,
		bool expanded,
		std::shared_ptr<int> offset,
		std::vector<Track> artists = {},
		Fn<void(Track)> openArtist = nullptr,
		bool homePlaybackControls = true)
	: RpWidget(parent)
	, _expanded(expanded)
	, _homePlaybackControls(homePlaybackControls)
	, _offset(std::move(offset)) {
		if (!_expanded) {
			_scroll = Ui::CreateChild<Ui::ScrollArea>(this, st::defaultScrollArea);
			_content = _scroll->setOwnedWidget(object_ptr<Ui::RpWidget>(_scroll));
			const auto makeEdge = [=](bool right) {
				const auto edge = Ui::CreateChild<Ui::RpWidget>(this);
				edge->setAttribute(Qt::WA_TransparentForMouseEvents);
				edge->paintOn([=](QPainter &p) {
					const auto outer = right ? edge->width() : 0;
					const auto inner = right ? 0 : edge->width();
					auto fade = QLinearGradient(outer, 0, inner, 0);
					fade.setColorAt(0., st::boxBg->c);
					fade.setColorAt(1., Qt::transparent);
					p.fillRect(edge->rect(), fade);
				});
				return edge;
			};
			_leftEdge = makeEdge(false);
			_rightEdge = makeEdge(true);
			_scroll->setCustomWheelProcess([=](not_null<QWheelEvent*> event) {
				// Keep a trackpad gesture on one axis, including its momentum.
				auto delta = event->pixelDelta();
				if (delta.isNull()) {
					delta = event->angleDelta() / 2;
				}
				if (event->modifiers().testFlag(Qt::ShiftModifier)
					&& std::abs(delta.y()) > std::abs(delta.x())) {
					delta = QPoint(delta.y(), 0);
				}
				const auto axis = _wheelDirectionLock.update(
					event->phase(), delta).value_or(
						std::abs(delta.x()) > std::abs(delta.y())
							? Qt::Horizontal
							: Qt::Vertical);
				if (axis != Qt::Horizontal) {
					// Skip QScrollArea's fallback and let the Home feed scroll.
					event->ignore();
					return true;
				}
				_slide.stop();
				_scroll->scrollToX(_scroll->scrollLeft() - delta.x());
				event->accept();
				return true;
			});
			_scroll->scrolls() | rpl::on_next([=] {
				if (!_restoring && _offset) *_offset = _scroll->scrollLeft();
				updateScrollEdges();
			}, lifetime());
			const auto slide = [=](int delta) {
				const auto from = _scroll->scrollLeft();
				const auto to = std::clamp(from + delta, 0, _scroll->scrollLeftMax());
				_slide.start([=](float64 value) {
					_scroll->scrollToX(int(base::SafeRound(value)));
				}, from, to, 320, anim::easeOutCubic);
			};
			_previous = Ui::CreateChild<HomeCarouselArrow>(this, false, [=] {
				slide(-_scroll->width() * 3 / 4);
			});
			_next = Ui::CreateChild<HomeCarouselArrow>(this, true, [=] {
				slide(_scroll->width() * 3 / 4);
			});
		} else {
			_content = this;
		}
		const auto results = std::make_shared<std::vector<Track>>(tracks);
		auto trackIndex = 0;
		auto albumIndex = 0;
		auto artistIndex = 0;
		const auto limit = _expanded ? tracks.size() + albums.size() + artists.size() : size_t(12);
		const auto artistKey = (artists.empty() ? 0ULL : artists.front().artistId.toULongLong())
			+ (tracks.empty() ? 0ULL : tracks.front().id.toULongLong());
		const auto artistSlot = 1 + int(artistKey % 4);
		while (_items.size() < limit
			&& (trackIndex < int(tracks.size()) || albumIndex < int(albums.size()) || artistIndex < int(artists.size()))) {
			if (artistIndex < int(artists.size()) && openArtist
				&& ((_items.size() % 4) == artistSlot || (trackIndex >= int(tracks.size()) && albumIndex >= int(albums.size())))) {
				_items.push_back(Ui::CreateChild<HomeArtistCard>(_content, artists[artistIndex++], openArtist));
			} else if (albumIndex < int(albums.size())
				&& ((_items.size() % 3) == 2 || (trackIndex >= int(tracks.size()) && artistIndex >= int(artists.size())))) {
				const auto album = albums[albumIndex++];
				_items.push_back(Ui::CreateChild<HomeAlbumCard>(
					_content, controller, album, [=] { open(album); }));
			} else if (trackIndex < int(tracks.size())) {
				const auto index = trackIndex++;
				_items.push_back(Ui::CreateChild<Row>(
					_content, controller, tracks[index], index, RowMode::Home,
					results, -1, openTrack, QString(), _homePlaybackControls));
			} else if (albumIndex < int(albums.size())) {
				const auto album = albums[albumIndex++];
				_items.push_back(Ui::CreateChild<HomeAlbumCard>(_content, controller, album, [=] { open(album); }));
			} else {
				break;
			}
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto gap = s(kHomeCardGap);
		const auto side = s(BoxParts::kOuterSide);
		const auto available = std::max(newWidth - 2 * side, 1);
		if (_expanded) {
			const auto columns = std::clamp((available + gap) / (s(180) + gap), 1, 8);
			const auto cardWidth = (available - (columns - 1) * gap) / columns;
			const auto cardHeight = cardWidth + s(kHomeCardDetailsHeight);
			for (auto i = 0; i != int(_items.size()); ++i) {
				_items[i]->setGeometry(side + (i % columns) * (cardWidth + gap),
					(i / columns) * (cardHeight + gap), cardWidth, cardHeight);
			}
			return ((_items.size() + columns - 1) / columns) * (cardHeight + gap);
		}
		const auto cardWidth = HomeShelfCardWidth(newWidth);
		const auto cardHeight = cardWidth + s(kHomeCardDetailsHeight);
		_scroll->setGeometry(0, 0, newWidth, cardHeight + gap);
		_content->resize(2 * side + int(_items.size()) * (cardWidth + gap) - gap,
			cardHeight + gap);
		for (auto i = 0; i != int(_items.size()); ++i) {
			_items[i]->setGeometry(side + i * (cardWidth + gap), 0, cardWidth, cardHeight);
		}
		const auto arrow = s(40);
		_previous->setGeometry(side, (cardWidth - arrow) / 2, arrow, arrow);
		_next->setGeometry(std::max(newWidth - side - arrow, 0),
			(cardWidth - arrow) / 2, arrow, arrow);
		_previous->raise();
		_next->raise();
		if (!_restoreQueued) {
			_restoreQueued = true;
			crl::on_main(this, [=] {
				if (_offset) _scroll->scrollToX(*_offset);
				_restoring = false;
				updateScrollEdges();
			});
		}
		updateScrollEdges();
		return cardHeight + gap;
	}

private:
	void updateScrollEdges() {
		if (!_previous || !_next) return;
		const auto left = _scroll->scrollLeft();
		const auto right = _scroll->scrollLeftMax() - left;
		const auto width = _scroll->width();
		const auto height = _scroll->height() - style::ConvertScale(kHomeCardGap);
		const auto fade = std::min(style::ConvertScale(48), width / 2);
		const auto updateEdge = [&](Ui::RpWidget *edge, int overflow, bool right) {
			const auto extent = std::clamp(overflow, 0, fade);
			if (extent > 0 && height > 0) {
				edge->setGeometry(right ? width - extent : 0, 0, extent, height);
				edge->show();
			} else {
				edge->hide();
			}
		};
		updateEdge(_leftEdge, left, false);
		updateEdge(_rightEdge, right, true);
		_previous->setVisible(left > 0);
		_next->setVisible(right > 0);
	}

	bool _expanded = false;
	bool _homePlaybackControls = true;
	bool _restoring = true;
	bool _restoreQueued = false;
	std::shared_ptr<int> _offset;
	Ui::ScrollArea *_scroll = nullptr;
	Ui::RpWidget *_content = nullptr;
	Ui::RpWidget *_leftEdge = nullptr;
	Ui::RpWidget *_rightEdge = nullptr;
	HomeCarouselArrow *_previous = nullptr;
	HomeCarouselArrow *_next = nullptr;
	Ui::Animations::Simple _slide;
	Ui::ScrollDirectionLock _wheelDirectionLock;
	std::vector<Ui::RpWidget*> _items;

};


enum class CatalogArtwork { Artist, Playlist, Release };

class CatalogEntityRow final : public Ui::AbstractButton {
public:
	CatalogEntityRow(
		QWidget *parent,
		QString title,
		QString subtitle,
		QString type,
		QString artwork,
		CatalogArtwork artworkStyle,
		Fn<void()> open,
		Fn<void(not_null<Ui::RoundButton*>)> play = nullptr)
	: AbstractButton(parent)
	, _title(std::move(title))
	, _subtitle(std::move(subtitle))
	, _artworkStyle(artworkStyle) {
		resize(width(), style::ConvertScale(
			play ? kFeaturedArtistHeight : kGroupedTrackHeight));
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		auto accessible = QStringList{ _title, _subtitle, std::move(type) };
		accessible.removeAll(QString());
		setAccessibleName(accessible.join(u" · "_q));
		setClickedCallback(std::move(open));
		if (play) {
			_play = Ui::CreateChild<Ui::RoundButton>(
				this,
				rpl::single(PlaylistText("Слушать")),
				BoxParts::PillLight());
			_play->setFocusPolicy(Qt::TabFocus);
			_play->setClickedCallback([=] { play(_play); });
		}
		LoadImage(ArtUrl(artwork, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			update();
		}));
	}

	void setAlbumBackdrop(Album album) {
		if (album.backdropArtworkUrl && album.backdropArtworkUrl->isEmpty()) {
			return;
		}
		_startBackdropLoad = [=, album = std::move(album)] {
			LoadAlbumBackdrop(album, crl::guard(this, [=](QString url) {
				if (url.isEmpty()) {
					return;
				}
				LoadImage(ArtUrl(url, u"t200x200"_q), crl::guard(this, [=](QImage image) {
					_backdrop = std::move(image);
					update();
				}));
			}));
		};
	}

protected:
	void onStateChanged(State, StateChangeSource) override {
		update();
	}

	void resizeEvent(QResizeEvent *event) override {
		AbstractButton::resizeEvent(event);
		if (_play) {
			_play->setFullWidth(style::ConvertScale(kFeaturedArtistButtonWidth));
			_play->moveToRight(style::ConvertScale(30), (height() - _play->height()) / 2);
		}
	}

	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		const auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int value) { return style::ConvertScale(value); };
		if (_play) {
			PaintHomeSurface(p, rect(), isOver() ? 1. : 0., isDown(), hasFocus());
		} else {
			PaintTrackListState(p, *this, false);
		}
		const auto side = s(_play ? kFeaturedArtistArtSize : kGroupedTrackLeadWidth);
		const auto art = QRect(s(kGroupedTrackLeadingInset), (height() - side) / 2, side, side);
		const auto release = (_artworkStyle == CatalogArtwork::Release);
		if (release) {
			if (_startBackdropLoad) {
				crl::on_main(this, std::exchange(_startBackdropLoad, {}));
			}
			const auto card = QRectF(art).adjusted(s(2), s(3), -s(2), -s(3)).translated(-s(8), 0);
			const auto local = card.translated(-card.center());
			p.save();
			p.translate(card.center());
			p.rotate(-12);
			auto clip = QPainterPath();
			clip.addRoundedRect(local, s(11), s(11));
			p.fillPath(clip, anim::color(st::windowBgOver, st::windowSubTextFg, 0.28));
			if (!_backdrop.isNull()) {
				p.setClipPath(clip, Qt::IntersectClip);
				p.drawImage(local, _backdrop);
			}
			p.restore();
		}
		auto clip = QPainterPath();
		if (_artworkStyle == CatalogArtwork::Artist) {
			clip.addEllipse(art);
		} else {
			const auto radius = s(release ? 12 : 8);
			clip.addRoundedRect(art, radius, radius);
		}
		p.save();
		p.setClipPath(clip);
		if (_art.isNull()) {
			p.fillRect(art, _play ? st::windowBgOver : st::windowBg);
		} else {
			p.drawImage(art, _art);
		}
		p.restore();

		const auto left = art.x() + art.width() + s(kGroupedTrackTextGap);
		const auto right = _play ? _play->x() : width();
		const auto available = std::max(right - left - s(14), 0);
		auto titleFont = st::semiboldFont->f;
		const auto titleHeight = s(_play ? 26 : 20);
		const auto subtitleHeight = _subtitle.isEmpty() ? 0 : s(18);
		titleFont.setPixelSize(s(_play ? 22 : 16));
		const auto top = (height() - titleHeight - subtitleHeight) / 2;
		p.setFont(titleFont);
		p.setPen(st::windowFg);
		p.drawText(
			QRect(left, top, available, titleHeight),
			Qt::AlignVCenter,
			QFontMetrics(titleFont).elidedText(_title, Qt::ElideRight, available));
		p.setFont(st::normalFont);
		p.setPen(MusicSecondaryColor());
		p.drawText(
			QRect(left, top + titleHeight, available, subtitleHeight),
			Qt::AlignVCenter,
			st::normalFont->elided(_subtitle, available));
	}

private:
	QString _title;
	QString _subtitle;
	CatalogArtwork _artworkStyle;
	QImage _art;
	QImage _backdrop;
	Fn<void()> _startBackdropLoad;
	Ui::RoundButton *_play = nullptr;
};

class AlbumBar final : public Ui::AbstractButton {
public:
	AlbumBar(QWidget *parent, const QString &title, Fn<void()> back, Fn<void()> play)
	: AbstractButton(parent), _title(title) {
		resize(width(), 40);
		setPointerCursor(true);
		setMouseTracking(true);
		setClickedCallback([=] {
			if (playRect().contains(mapFromGlobal(QCursor::pos()))) play();
			else back();
		});
		paintRequest() | rpl::on_next([=](QRect) {
			auto p = QPainter(this);
			auto hq = PainterHighQualityEnabler(p);
			const auto onPlay = playRect().contains(mapFromGlobal(QCursor::pos()));
			p.setPen(QPen(isOver() && !onPlay ? st::windowFg : st::windowSubTextFg,
				2., Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
			const auto cy = height() / 2.;
			p.drawPolyline(QPolygonF({ QPointF(16, cy - 6), QPointF(10, cy), QPointF(16, cy + 6) }));
			const auto available = playRect().left() - 38;
			p.setFont(st::semiboldFont);
			p.setPen(st::windowFg);
			p.drawText(QRect(26, 0, available, height()), Qt::AlignVCenter | Qt::AlignLeft,
				st::semiboldFont->elided(_title, available));
			p.setPen(onPlay ? st::windowFg : st::windowActiveTextFg);
			p.drawText(playRect(), Qt::AlignVCenter | Qt::AlignRight,
				QString::fromUtf8(Mokagram::Lang::Tr("Слушать альбом")));
		}, lifetime());
	}
protected:
	void mouseMoveEvent(QMouseEvent *event) override {
		AbstractButton::mouseMoveEvent(event);
		update();
	}
private:
	[[nodiscard]] QRect playRect() const {
		const auto w = st::semiboldFont->width(QString::fromUtf8(Mokagram::Lang::Tr("Слушать альбом")));
		return QRect(width() - 8 - w, 0, w, height());
	}
	QString _title;
};

[[nodiscard]] QString HomeReleaseFacts(
		int year,
		int count,
		const std::vector<Track> &tracks,
		bool complete) {
	auto facts = QStringList();
	if (year > 0) {
		facts.push_back(QString::number(year));
	}
	if (count > 0) {
		const auto last = count % 10;
		const auto tens = (count / 10) % 10;
		auto summary = (count == 1) ? PlaylistText("1 трек")
			: PlaylistText(tens == 1 ? "%1 треков"
				: last == 1 ? "%1 трек"
				: (last >= 2 && last <= 4) ? "%1 трека"
				: "%1 треков").arg(count);
		if (complete && tracks.size() == size_t(count)) {
			auto duration = qint64(0);
			for (const auto &track : tracks) {
				if (track.durationMs <= 0) {
					duration = 0;
					break;
				}
				duration += track.durationMs;
			}
			if (duration >= 1000) {
				summary = u"%1, %2"_q.arg(summary, Ui::FormatDurationWords(duration / 1000));
			}
		}
		facts.push_back(std::move(summary));
	}
	return facts.join(u" · "_q);
}

class HomeAlbumHeader final : public Ui::RpWidget {
public:
	HomeAlbumHeader(
		QWidget *parent,
		Album album,
		QString facts,
		QString artistAvatarUrl,
		bool isTrack,
		Fn<void(QImage, QRect)> showCover,
		bool isArtist,
		QString bannerUrl,
		Fn<void()> openArtist,
		Fn<void()> bannerReady)
	: RpWidget(parent)
	, _album(std::move(album))
	, _isArtist(isArtist)
	, _tyankaAvatar(isArtist && TyankaHikka::IsArtist(_album.title))
	, _kind(isTrack
		? QString()
		: isArtist ? PlaylistText("Исполнитель") : HomeReleaseKindText(_album.releaseKind))
	, _facts(std::move(facts))
	, _showCover(std::move(showCover)) {
		setToolTip(_album.title);
		setAccessibleName(_album.title);
		auto description = QStringList{ _kind, _facts };
		description.removeAll(QString());
		setAccessibleDescription(description.join(u" · "_q));
		_cover = Ui::CreateChild<HomeTonalControl>(this);
		_cover->setAccessibleName(PlaylistText("Открыть обложку"));
		_cover->setClickedCallback([=] { openCover(); });
		_cover->hide();
		_cover->paintOn([=](QPainter &p) {
			if (!_cover->hasFocus()) return;
			auto hq = PainterHighQualityEnabler(p);
			const auto s = [](int v) { return style::ConvertScale(v); };
			const auto radius = _isArtist ? _cover->width() / 2 : s(16);
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(st::windowActiveTextFg->c, s(2)));
			p.drawRoundedRect(_cover->rect().adjusted(s(1), s(1), -s(1), -s(1)),
				radius, radius);
		});

		if (!_isArtist && !_album.artist.trimmed().isEmpty()) {
			_authorWidth = st::semiboldFont->width(_album.artist);
		}
		if (_authorWidth > 0 && openArtist) {
			_artist = Ui::CreateChild<HomeTonalControl>(this);
			if (!artistAvatarUrl.isEmpty()) {
				_avatarSpace = style::ConvertScale(30);
				LoadImage(artistAvatarUrl, crl::guard(this, [=](QImage image) {
					_artistAvatar = std::move(image);
					_artist->update();
				}));
			}
			_artist->setToolTip(_album.artist);
			_artist->setAccessibleName(_album.artist);
			_artist->setClickedCallback(std::move(openArtist));
			_artist->paintOn([=](QPainter &p) {
				auto text = _artist->rect();
				const auto avatarSide = style::ConvertScale(24);
				if (_avatarSpace && text.width() > _avatarSpace) {
					const auto avatar = QRect(
						0, (text.height() - avatarSide) / 2, avatarSide, avatarSide);
					const auto hq = PainterHighQualityEnabler(p);
					auto clip = QPainterPath();
					clip.addEllipse(avatar);
					p.save();
					p.setClipPath(clip);
					paintArtwork(p, avatar, _artistAvatar);
					p.restore();
					text.setLeft(_avatarSpace);
				}
				auto font = st::semiboldFont->f;
				font.setUnderline(_artist->isOver() || _artist->isDown() || _artist->hasFocus());
				p.setFont(font);
				p.setPen(st::windowFg);
				p.drawText(text, Qt::AlignVCenter | Qt::AlignLeft,
					st::semiboldFont->elided(_album.artist, text.width()));
			});
		}
		LoadImage(ArtUrl(_album.artworkUrl, u"t500x500"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			_cover->setVisible(!_art.isNull() && _identityOpacity >= .5);
			update();
		}));
		if (_isArtist && !bannerUrl.isEmpty()) {
			LoadImage(bannerUrl, crl::guard(this, [=](QImage image) {
				if (image.isNull()) return;
				_banner = std::move(image);
				_bannerTone = _banner.pixelColor(_banner.width() / 2, _banner.height() - 1);
				resizeToWidth(width());
				update();
				if (bannerReady) bannerReady();
			}));
		}
	}

	void setArtistIdentityOpacity(double opacity) {
		if (_identityOpacity == opacity) return;
		_identityOpacity = opacity;
		_cover->setVisible(!_art.isNull() && opacity >= .5);
		update();
	}

	[[nodiscard]] QColor artistBannerColor() const {
		return _bannerTone.isValid() && _bannerTone.alpha()
			? anim::color(st::windowBg->c, _bannerTone, .16)
			: QColor();
	}

protected:
	void resizeEvent(QResizeEvent *e) override {
		RpWidget::resizeEvent(e);
		_layout = geometryForWidth(width());
		_cover->setGeometry(_layout.art);
		if (_artist) _artist->setGeometry(_layout.author);
	}

	int resizeGetHeight(int newWidth) override {
		return geometryForWidth(newWidth).card.height();
	}

	void paintEvent(QPaintEvent *) override {
		const auto &layout = _layout;
		if (layout.card.isEmpty()) return;
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		PaintMusicCard(p, layout.card);
		if (!layout.banner.isEmpty()) {
			auto path = QPainterPath();
			const auto radius = s(BoxParts::kCardRadius);
			path.addRoundedRect(layout.card, radius, radius);
			p.save();
			p.setClipPath(path);
			const auto bottom = layout.banner.y() + layout.banner.height();
			const auto fadeHeight = std::min(s(32), layout.banner.height());
			const auto band = QRect(layout.banner.x(), bottom - fadeHeight,
				layout.banner.width(), fadeHeight);
			p.save();
			p.setClipRect(layout.banner.adjusted(0, 0, 0, -fadeHeight), Qt::IntersectClip);
			paintArtwork(p, layout.banner, _banner);
			p.restore();

			const auto dpr = devicePixelRatioF();
			auto fade = QImage(QSize(
				int(std::ceil(band.width() * dpr)),
				int(std::ceil(band.height() * dpr))),
				QImage::Format_ARGB32_Premultiplied);
			fade.setDevicePixelRatio(dpr);
			fade.fill(Qt::transparent);
			{
				auto layer = QPainter(&fade);
				layer.setRenderHint(QPainter::SmoothPixmapTransform);
				paintArtwork(layer, layout.banner.translated(-band.topLeft()), _banner);
				layer.setCompositionMode(QPainter::CompositionMode_DestinationIn);
				auto alpha = QLinearGradient(0., 0., 0., fadeHeight);
				alpha.setColorAt(0., Qt::black);
				alpha.setColorAt(1., Qt::transparent);
				layer.fillRect(QRect(QPoint(), band.size()), alpha);
			}
			p.drawImage(band.topLeft(), fade);
			p.restore();
		}

		p.setOpacity(_identityOpacity);
		if (!layout.art.isEmpty()) {
			auto path = QPainterPath();
			if (_isArtist) {
				path.addEllipse(layout.art);
			} else {
				path.addRoundedRect(layout.art, s(16), s(16));
			}
			p.save();
			p.setClipPath(path);
			paintArtwork(p, layout.art, _art);
			p.restore();
		}

		const auto paintText = [&](const QRect &rect, const QFont &font, const QString &text) {
			if (rect.isEmpty()) return;
			p.setFont(font);
			p.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft,
				QFontMetrics(font).elidedText(text, Qt::ElideRight, rect.width()));
		};
		p.setPen(MusicSecondaryColor());
		paintText(layout.kind, st::normalFont->f, _kind);
		if (!_artist) paintText(layout.author, st::normalFont->f, _album.artist);
		paintText(layout.facts, st::normalFont->f, _facts);
		if (layout.inlineFacts) {
			paintText(layout.separator, st::normalFont->f, u"·"_q);
		}
		p.setPen(st::windowFg);
		auto font = st::semiboldFont->f;
		font.setPixelSize(layout.titleSize);
		paintText(layout.title, font, _album.title);
	}

private:
	struct HeaderLayout {
		QRect card;
		QRect banner;
		QRect art;
		QRect kind;
		QRect title;
		QRect author;
		QRect separator;
		QRect facts;
		int titleSize = 0;
		bool inlineFacts = false;
	};

	[[nodiscard]] HeaderLayout geometryForWidth(int width) const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		const auto cardWidth = std::max(width - 2 * side, 0);
		const auto inset = std::min(s(16), cardWidth / 2);
		const auto left = side + inset;
		const auto right = side + cardWidth - inset;
		const auto available = right - left;
		auto result = HeaderLayout();
		const auto compact = cardWidth < s(_isArtist ? 480 : 420);
		const auto hasBanner = _isArtist && !_banner.isNull();
		const auto bannerHeight = hasBanner ? std::clamp(cardWidth / 4, s(112), s(220)) : 0;
		result.banner = QRect(side, 0, cardWidth, bannerHeight);
		const auto body = bannerHeight + inset;
		const auto artSide = std::min(available, _isArtist
			? s(compact ? 64 : 88)
			: std::clamp(available / (compact ? 2 : 3), s(128), s(200)));
		result.titleSize = _isArtist
			? s(compact ? 28 : 36)
			: std::clamp(available / 12, s(32), s(60));
		auto titleFont = st::semiboldFont->f;
		titleFont.setPixelSize(result.titleSize);
		const auto titleMetrics = QFontMetrics(titleFont);
		const auto titleHeight = _isArtist
			? s(compact ? 40 : 48)
			: titleMetrics.height();
		const auto kindHeight = _kind.isEmpty() ? 0 : s(20);
		if (_isArtist) {
			const auto row = std::max(artSide, kindHeight + titleHeight);
			const auto textLeft = left + artSide + s(16);
			const auto textWidth = std::max(right - textLeft, 0);
			const auto textTop = body + (row - kindHeight - titleHeight) / 2;
			result.art = QRect(left, body + (row - artSide) / 2, artSide, artSide);
			result.kind = QRect(textLeft, textTop, textWidth, kindHeight);
			result.title = QRect(textLeft, textTop + kindHeight, textWidth, titleHeight);
			result.card = QRect(side, 0, cardWidth, body + row + inset);
		} else {
			const auto textLeft = compact ? left : left + artSide + s(20);
			const auto textWidth = std::max(right - textLeft, 0);
			const auto titleWidth = titleMetrics.horizontalAdvance(_album.title);
			if (titleWidth > textWidth) {
				result.titleSize = std::max(s(28),
					int(qint64(result.titleSize) * textWidth / titleWidth));
			}
			const auto authorWidth = _authorWidth + _avatarSpace;
			const auto factsWidth = st::normalFont->width(_facts);
			const auto separatorWidth = s(12);
			const auto both = authorWidth > 0 && factsWidth > 0;
			result.inlineFacts = both && authorWidth + separatorWidth + factsWidth <= textWidth;
			const auto hasMetadata = authorWidth > 0 || factsWidth > 0;
			const auto metadataHeight = !hasMetadata ? 0
				: (both && !result.inlineFacts) ? s(48) : s(24);
			const auto titleGap = hasMetadata ? s(6) : 0;
			const auto detailsHeight = kindHeight + titleHeight + titleGap + metadataHeight;
			const auto row = compact ? artSide + s(16) + detailsHeight
				: std::max(artSide, detailsHeight);
			const auto textTop = compact ? body + artSide + s(16) : body + (row - detailsHeight) / 2;
			result.art = QRect(compact ? left + (available - artSide) / 2 : left,
				compact ? body : body + (row - artSide) / 2, artSide, artSide);
			result.kind = QRect(textLeft, textTop, textWidth, kindHeight);
			result.title = QRect(textLeft, textTop + kindHeight, textWidth, titleHeight);
			const auto metadataTop = textTop + kindHeight + titleHeight + titleGap;
			result.author = QRect(textLeft, metadataTop, std::min(textWidth, authorWidth), s(24));
			const auto factsLeft = result.inlineFacts ? textLeft + authorWidth + separatorWidth
				: textLeft;
			const auto factsTop = result.inlineFacts || !authorWidth
				? metadataTop : metadataTop + s(24);
			result.separator = QRect(textLeft + authorWidth + s(4), metadataTop,
				separatorWidth - s(4), s(24));
			result.facts = QRect(factsLeft, factsTop, right - factsLeft, factsWidth ? s(24) : 0);
			result.card = QRect(side, 0, cardWidth, body + row + inset);
		}
		return result;
	}

	static void paintArtwork(QPainter &p, const QRect &rect, const QImage &image) {
		if (image.isNull()) {
			p.fillRect(rect, st::windowBgOver);
			return;
		}
		const auto scale = std::min(
			double(image.width()) / rect.width(), double(image.height()) / rect.height());
		const auto sourceWidth = rect.width() * scale;
		const auto sourceHeight = rect.height() * scale;
		p.drawImage(rect, image, QRectF(
			(image.width() - sourceWidth) / 2., (image.height() - sourceHeight) / 2.,
			sourceWidth, sourceHeight));
	}

	void openCover() {
		if (_art.isNull() || _identityOpacity < .5) return;
		const auto show = [this] {
			const auto rect = _cover->rect();
			_showCover(_art, QRect(_cover->mapToGlobal(rect.topLeft()), rect.size()));
		};
		if (!_tyankaAvatar) {
			show();
			return;
		}
		constexpr auto kAvatarClickGap = 800;
		const auto now = crl::now();
		if (now - _lastAvatarClick >= kAvatarClickGap) {
			_avatarClicks = 0;
		}
		_lastAvatarClick = now;
		if (++_avatarClicks == 6) {
			_avatarClicks = 0;
			TyankaHikka::Unlock();
		} else if (_avatarClicks == 1) {
			QTimer::singleShot(kAvatarClickGap, this, [=, this] {
				if (_avatarClicks == 1 && _lastAvatarClick == now
					&& _identityOpacity >= .5) {
					show();
				}
			});
		}
	}

	Album _album;
	bool _isArtist = false;
	bool _tyankaAvatar = false;
	int _avatarClicks = 0;
	crl::time _lastAvatarClick = 0;
	QString _kind;
	QString _facts;
	HeaderLayout _layout;
	QImage _art;
	QImage _artistAvatar;
	QImage _banner;
	QColor _bannerTone;
	Fn<void(QImage, QRect)> _showCover;
	HomeTonalControl *_cover = nullptr;
	HomeTonalControl *_artist = nullptr;
	int _authorWidth = 0;
	int _avatarSpace = 0;
	double _identityOpacity = 1.;
};

class HomeArtistLikesCard final : public HomeTonalControl {
public:
	HomeArtistLikesCard(
		QWidget *parent,
		QString artworkUrl,
		QString counts,
		bool expanded,
		Fn<void()> toggle)
	: HomeTonalControl(parent)
	, _counts(std::move(counts))
	, _action(PlaylistText(expanded ? "Свернуть" : "Показать все")) {
		_title = PlaylistText("Вам нравится");
		_actionWidth = st::semiboldFont->width(_action);
		setAccessibleName(_title + u" · "_q + _counts + u" · "_q + _action);
		setClickedCallback(std::move(toggle));
		LoadImage(ArtUrl(artworkUrl, u"t200x200"_q), crl::guard(this, [=](QImage image) {
			_art = std::move(image);
			update();
		}));
	}

protected:
	int resizeGetHeight(int newWidth) override {
		return style::ConvertScale(compact(newWidth) ? 128 : 84);
	}

	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int value) { return style::ConvertScale(value); };
		PaintHomeSurface(p, rect(), isOver() ? 1. : 0., isDown(), hasFocus());
		const auto artwork = QRect(s(14), s(14), s(56), s(56));
		PaintHomeCover(p, myrtlrect(artwork), _art);

		const auto left = artwork.right() + s(15);
		const auto stacked = compact(width());
		const auto actionInset = s(kHomeShelfActionInset);
		const auto actionWidth = std::min(_actionWidth, std::max(width() - 2 * actionInset, 0));
		const auto action = QRect(width() - actionInset - actionWidth,
			stacked ? s(78) : s(22), actionWidth, s(40));
		const auto right = stacked ? width() - s(14) : action.x() - s(12);
		const auto available = std::max(right - left, 0);
		const auto leading = Qt::AlignVCenter | Qt::AlignAbsolute
			| (rtl() ? Qt::AlignRight : Qt::AlignLeft);
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(myrtlrect(QRect(left, s(16), available, s(24))),
			leading, st::semiboldFont->elided(_title, available));
		p.setFont(st::normalFont);
		p.setPen(st::windowSubTextFg);
		p.drawText(myrtlrect(QRect(left, s(43), available, s(22))),
			leading, st::normalFont->elided(_counts, available));
		p.setFont(st::semiboldFont);
		p.setPen(st::windowFg);
		p.drawText(myrtlrect(action), Qt::AlignCenter,
			st::semiboldFont->elided(_action, actionWidth));
	}

private:
	[[nodiscard]] bool compact(int width) const {
		return width < _actionWidth + style::ConvertScale(244);
	}

	QString _title;
	QString _counts;
	QString _action;
	int _actionWidth = 0;
	QImage _art;
};

enum class DiscographyFilter {
	All,
	Albums,
	SinglesAndEps,
};

class MusicChoiceChip final : public HomeTonalControl {
public:
	MusicChoiceChip(QWidget *parent, QString label, bool selected)
	: HomeTonalControl(parent)
	, _label(std::move(label))
	, _selected(selected) {
		setAccessibleName(_label);
		setNaturalWidth(st::semiboldFont->width(_label) + style::ConvertScale(24));
	}

	Ui::AccessibilityState accessibilityState() const override {
		auto result = HomeTonalControl::accessibilityState();
		result.checkable = true;
		result.checked = _selected;
		return result;
	}

protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto shape = rect().adjusted(s(1), s(4), -s(1), -s(4));
		const auto base = _selected
			? anim::color(st::windowBg->c, st::windowBgActive->c, .24)
			: st::windowBg->c;
		p.setBrush(anim::color(base, st::windowFg->c, isDown() ? .12 : isOver() ? .08 : 0.));
		p.setPen(hasFocus()
			? QPen(st::windowActiveTextFg->c, s(2))
			: QPen(Qt::NoPen));
		const auto radius = s(8);
		p.drawRoundedRect(shape, radius, radius);
		const auto font = st::semiboldFont;
		p.setFont(font);
		p.setPen(st::windowFg);
		p.drawText(shape, Qt::AlignCenter,
			font->elided(_label, std::max(shape.width() - s(16), 0)));
	}

private:
	QString _label;
	bool _selected = false;
};

class HomeDiscographyBar final : public Ui::RpWidget {
public:
	HomeDiscographyBar(
		QWidget *parent,
		DiscographyFilter selected,
		bool expanded,
		Fn<void(DiscographyFilter)> filter,
		Fn<void()> toggle)
	: RpWidget(parent) {
		const auto title = PlaylistText("Дискография");
		_title = Ui::CreateChild<Ui::FlatLabel>(this, title, st::boxTitle);
		if (toggle) {
			_toggle = Ui::CreateChild<HomeShelfAction>(
				this,
				PlaylistText(expanded ? "Свернуть" : "Показать все"),
				title,
				std::move(toggle));
		}
		const auto labels = std::array{
			"Все релизы",
			"Альбомы",
			"Синглы и EP",
		};
		for (auto i = 0; i != int(labels.size()); ++i) {
			const auto category = DiscographyFilter(i);
			const auto button = _filters[i] = Ui::CreateChild<MusicChoiceChip>(
				this, PlaylistText(labels[i]), selected == category);
			button->setClickedCallback([=] { filter(category); });
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto rowHeight = s(44);
		const auto toggleWidth = _toggle
			? std::min(_toggle->naturalWidth(), newWidth / 2)
			: 0;
		_title->resizeToWidth(std::max(newWidth - toggleWidth - s(8), 0));
		_title->moveToLeft(0, (rowHeight - _title->height()) / 2, newWidth);
		if (_toggle) {
			_toggle->resizeToWidth(toggleWidth);
			_toggle->moveToRight(0, (rowHeight - _toggle->height()) / 2, newWidth);
		}
		const auto gap = s(kHomeCardGap);
		const auto chipHeight = s(40);
		auto x = 0;
		auto y = rowHeight;
		for (const auto chip : _filters) {
			const auto width = std::min(chip->naturalWidth(), newWidth);
			if (x > 0 && x + width > newWidth) {
				x = 0;
				y += chipHeight + gap;
			}
			chip->resize(width, chipHeight);
			chip->moveToLeft(x, y, newWidth);
			x += width + gap;
		}
		return y + chipHeight;
	}

private:
	Ui::FlatLabel *_title = nullptr;
	HomeTonalControl *_toggle = nullptr;
	std::array<MusicChoiceChip*, 3> _filters = {};
};

class HomeCoverPreview final : public Ui::RpWidget {
public:
	HomeCoverPreview(QWidget *parent, QImage image, QRect source)
	: RpWidget(parent)
	, _image(std::move(image))
	, _source(source) {
		setGeometry(parent->rect());
		setFocusPolicy(Qt::TabFocus);
		parent->installEventFilter(this);
		show();
		raise();
		setFocus();
		if (!PowerSaving::On(PowerSaving::kAnimations)) {
			_animation.start([=] { update(); }, 0., 1., 220, anim::easeOutCubic);
		}
	}

protected:
	bool eventFilter(QObject *watched, QEvent *event) override {
		if (watched == parentWidget() && event->type() == QEvent::Resize) {
			setGeometry(parentWidget()->rect());
		}
		return RpWidget::eventFilter(watched, event);
	}

	void mouseReleaseEvent(QMouseEvent *event) override {
		if (event->button() == Qt::LeftButton && !targetRect().contains(event->pos())) {
			closePreview();
		}
	}

	void keyPressEvent(QKeyEvent *event) override {
		if (event->key() == Qt::Key_Escape) {
			closePreview();
		} else {
			RpWidget::keyPressEvent(event);
		}
	}

	void paintEvent(QPaintEvent *event) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto progress = _animation.value(_closing ? 0. : 1.);
		p.fillRect(rect(), QColor(0, 0, 0, int(210 * progress)));
		const auto target = targetRect();
		const auto start = QRectF(_source);
		const auto current = QRectF(
			start.x() + (target.x() - start.x()) * progress,
			start.y() + (target.y() - start.y()) * progress,
			start.width() + (target.width() - start.width()) * progress,
			start.height() + (target.height() - start.height()) * progress);
		p.setOpacity(progress);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.drawImage(current, _image);
		p.setFont(st::semiboldFont);
		p.setPen(Qt::white);
		p.drawText(QRectF(target.left(), target.bottom() + style::ConvertScale(12),
			target.width(), style::ConvertScale(32)), Qt::AlignCenter,
			QString::fromUtf8(Mokagram::Lang::Tr("Закрыть")));
	}

private:
	[[nodiscard]] QRectF targetRect() const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = std::max(std::min({ width() - s(48), height() - s(100), s(600) }), 0);
		return QRectF((width() - side) / 2., (height() - side - s(40)) / 2., side, side);
	}

	void closePreview() {
		if (_closing) return;
		_closing = true;
		if (PowerSaving::On(PowerSaving::kAnimations)) {
			deleteLater();
			return;
		}
		const auto current = _animation.value(1.);
		_animation.start([=] { update(); }, current, 0., 180, anim::easeOutCubic);
		QTimer::singleShot(180, this, [=] { deleteLater(); });
	}

	QImage _image;
	QRect _source;
	Ui::Animations::Simple _animation;
	bool _closing = false;

};

// ---- полноценный плеер ----

class HomeRetryView final : public HomeTonalControl {
public:
	HomeRetryView(QWidget *parent, QString message, Fn<void()> retry)
	: HomeTonalControl(parent)
	, _message(std::move(message)) {
		setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Попробовать снова")));
		setAccessibleDescription(_message);
		setClickedCallback(std::move(retry));
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto text = QFontMetrics(st::normalFont->f).boundingRect(
			QRect(0, 0, std::max(newWidth - 2 * s(BoxParts::kOuterSide + 16), 1), s(1000)),
			Qt::TextWordWrap, _message);
		return text.height() + s(64);
	}

	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		const auto s = [](int v) { return style::ConvertScale(v); };
		auto hq = PainterHighQualityEnabler(p);
		const auto side = s(BoxParts::kOuterSide);
		const auto body = rect().adjusted(side, 0, -side, -s(8));
		PaintHomeSurface(p, body, isOver() ? 1. : 0., isDown(), hasFocus());
		const auto text = body.marginsRemoved(QMargins(s(16), s(12), s(16), s(8)));
		p.setFont(st::normalFont);
		p.setPen(st::windowFg);
		p.drawText(text.adjusted(0, 0, 0, -s(36)),
			Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, _message);
		p.setFont(st::semiboldFont);
		p.drawText(QRect(text.left(), text.bottom() - s(32) + 1, text.width(), s(32)),
			Qt::AlignLeft | Qt::AlignVCenter, PlaylistText("Попробовать снова"));
	}

private:
	QString _message;

};

enum class HomeLoadingLayout { Shelf, TrackList };

class HomeLoadingView final : public Ui::RpWidget {
public:
	HomeLoadingView(
		QWidget *parent,
		QString text,
		HomeLoadingLayout layout = HomeLoadingLayout::Shelf)
	: RpWidget(parent)
	, _text(std::move(text))
	, _layout(layout) {
		setAccessibleName(_text);
		if (!anim::Disabled() && !PowerSaving::On(PowerSaving::kAnimations)) {
			_started = crl::now();
			_animation.init([=](crl::time) {
				update(progressBarRect());
				return true;
			});
			_animation.start();
		}
	}

protected:
	int resizeGetHeight(int newWidth) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		if (_layout == HomeLoadingLayout::TrackList) {
			return s(56) + kTrackPlaceholders * s(kGroupedTrackHeight);
		}
		return s(56) + s(kHomeCardDetailsHeight) + s(kHomeCardGap)
			+ HomeShelfCardWidth(newWidth);
	}

	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		const auto available = std::max(width() - 2 * side, 1);
		const auto &dirty = e->region();
		const auto text = QRect(side, 0, available, s(24));
		if (dirty.intersects(text)) {
			p.setFont(st::normalFont);
			p.setPen(MusicSecondaryColor());
			p.drawText(text, Qt::AlignVCenter | Qt::AlignLeft,
				st::normalFont->elided(_text, available));
		}
		const auto bar = progressBarRect();
		p.setPen(Qt::NoPen);
		if (dirty.intersects(bar)) {
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(bar, s(2), s(2));
			// Сегмент въезжает слева и уезжает вправо за край полоски, поэтому на
			// стыке цикла нет скачка в начало.
			const auto segment = bar.width() * 0.3;
			const auto x = _started
				? bar.left() - segment + (bar.width() + segment)
					* std::fmod((crl::now() - _started) / 1400., 1.)
				: bar.left();
			p.save();
			auto clip = QPainterPath();
			clip.addRoundedRect(bar, s(2), s(2));
			p.setClipPath(clip);
			p.setBrush(st::windowActiveTextFg);
			p.drawRoundedRect(QRectF(x, bar.top(), segment, bar.height()), s(2), s(2));
			p.restore();
		}
		const auto cardsTop = s(56);
		if (e->rect().bottom() < cardsTop) {
			return;
		}
		if (_layout == HomeLoadingLayout::TrackList) {
			paintTrackPlaceholders(p, dirty, cardsTop);
			return;
		}
		const auto cardWidth = HomeShelfCardWidth(width());
		const auto cardHeight = cardWidth + s(kHomeCardDetailsHeight);
		const auto step = cardWidth + s(kHomeCardGap);
		const auto art = HomeCardArtRect(cardWidth);
		const auto coverRadius = s(BoxParts::kCardRadius - kHomeCardInset);
		const auto radius = std::min(s(BoxParts::kCardRadius), cardWidth / 2);
		for (auto left = side; left < width(); left += step) {
			const auto card = QRect(left, cardsTop, cardWidth, cardHeight);
			if (!dirty.intersects(card)) {
				continue;
			}
			p.setBrush(MusicCardColor());
			p.drawRoundedRect(card, radius, radius);
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(art.translated(card.topLeft()), coverRadius, coverRadius);
			const auto textLeft = left + art.left();
			p.drawRoundedRect(QRect(textLeft, cardsTop + cardWidth + s(4),
				art.width() * 3 / 4, s(10)), s(5), s(5));
			p.drawRoundedRect(QRect(textLeft, cardsTop + cardWidth + s(28),
				art.width() / 2, s(8)), s(4), s(4));
		}
	}

private:
	static constexpr auto kTrackPlaceholders = 3;

	void paintTrackPlaceholders(QPainter &p, const QRegion &dirty, int top) const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		const auto pitch = s(kGroupedTrackHeight);
		const auto leadWidth = s(kGroupedTrackLeadWidth);
		const auto leadLeft = side + s(kGroupedTrackLeadingInset);
		const auto left = leadLeft + leadWidth + s(kGroupedTrackTextGap);
		const auto panel = QRect(side, top, std::max(width() - 2 * side, 0),
			kTrackPlaceholders * pitch);
		if (!dirty.intersects(panel)) {
			return;
		}
		PaintMusicCard(p, panel);
		p.setBrush(st::windowBgOver);
		const auto durationRight = panel.right() + 1 - s(kGroupedTrackDurationInset);
		const auto textRight = durationRight - s(kGroupedTrackDurationColumn) - s(kGroupedTrackActionSize) - s(8);
		const auto textWidth = std::clamp(textRight - left, 0, s(320));
		for (auto i = 0; i != kTrackPlaceholders; ++i) {
			const auto rowTop = top + i * pitch;
			if (!dirty.intersects(QRect(panel.x(), rowTop, panel.width(), pitch))) {
				continue;
			}
			const auto center = rowTop + pitch / 2;
			const auto radius = s(4);
			p.drawRoundedRect(QRect(leadLeft + (leadWidth - s(12)) / 2,
				center - s(4), s(12), s(8)), radius, radius);
			p.drawRoundedRect(QRect(left, center - s(5),
				textWidth * (4 - i) / 4, s(10)), radius, radius);
			p.drawRoundedRect(QRect(durationRight - s(28),
				center - s(4), s(28), s(8)), radius, radius);
		}
	}

	QRect progressBarRect() const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto side = s(BoxParts::kOuterSide);
		const auto available = std::max(width() - 2 * side, 1);
		return QRect(side, s(32), std::min(available, s(240)), s(3));
	}

	QString _text;
	HomeLoadingLayout _layout;
	Ui::Animations::Basic _animation;
	crl::time _started = 0;

};

// Загрузка списка: бегущая полоска, подпись и мерцающие заглушки строк.
class LoadingView final : public Ui::RpWidget {
public:
	LoadingView(QWidget *parent, const QString &text)
	: RpWidget(parent)
	, _text(text) {
		resize(width(), kHeaderHeight + kRows * kRowStep + 8);
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
		if (!PowerSaving::On(PowerSaving::kAnimations)) {
			_started = crl::now();
			_animation.init([=](crl::time) {
				update();
				return true;
			});
			_animation.start();
		}
	}

private:
	static constexpr auto kHeaderHeight = 44;
	static constexpr auto kRows = 4;
	static constexpr auto kRowStep = 60;

	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto time = _started ? (crl::now() - _started) : crl::time(0);
		const auto skeleton = [&] {
			auto color = st::windowSubTextFg->c;
			color.setAlphaF(0.16);
			return color;
		}();

		// Подпись слегка «дышит».
		auto textColor = st::windowSubTextFg->c;
		textColor.setAlphaF(_started
			? 0.6 + 0.4 * (0.5 + 0.5 * std::sin(time / 260.))
			: 1.);
		p.setPen(textColor);
		p.setFont(st::normalFont);
		p.drawText(
			QRect(24, 0, width() - 48, 24),
			Qt::AlignLeft | Qt::AlignVCenter,
			_text);

		// Неопределённая полоска: сегмент бежит слева направо.
		const auto barRect = QRectF(24, 28, width() - 48, 4);
		p.setPen(Qt::NoPen);
		p.setBrush(skeleton);
		p.drawRoundedRect(barRect, 2, 2);
		const auto phase = std::fmod(time / 1100., 1.);
		const auto eased = 1. - std::pow(1. - phase, 2.);
		const auto segment = barRect.width() * 0.32;
		const auto x = _started
			? barRect.left() - segment + (barRect.width() + segment) * eased
			: barRect.left();
		p.save();
		auto clip = QPainterPath();
		clip.addRoundedRect(barRect, 2, 2);
		p.setClipPath(clip);
		p.setBrush(st::windowActiveTextFg->c);
		p.drawRoundedRect(QRectF(x, barRect.top(), segment, barRect.height()), 2, 2);
		p.restore();

		// Заглушки строк с бегущим бликом.
		const auto sweep = _started
			? std::fmod(time / 1400., 1.) * (width() + 160) - 80
			: -1000.;
		for (auto i = 0; i != kRows; ++i) {
			const auto top = kHeaderHeight + i * kRowStep;
			auto shape = QPainterPath();
			shape.addRoundedRect(QRectF(16, top + 6, 44, 44), 8, 8);
			shape.addRoundedRect(QRectF(72, top + 12, width() * 0.45 - i * 14, 10), 5, 5);
			shape.addRoundedRect(QRectF(72, top + 32, width() * 0.28, 9), 4.5, 4.5);
			p.setPen(Qt::NoPen);
			p.setBrush(skeleton);
			p.drawPath(shape);
			p.save();
			p.setClipPath(shape);
			auto glow = QLinearGradient(sweep - 70, 0, sweep + 70, 0);
			auto edge = st::windowFg->c;
			edge.setAlphaF(0.);
			auto mid = st::windowFg->c;
			mid.setAlphaF(0.10);
			glow.setColorAt(0., edge);
			glow.setColorAt(0.5, mid);
			glow.setColorAt(1., edge);
			p.fillRect(QRectF(0, top, width(), kRowStep), glow);
			p.restore();
		}
	}

	const QString _text;
	Ui::Animations::Basic _animation;
	crl::time _started = 0;

};

constexpr auto kHintStickerKey = "music.hint_sticker";

// Свой стикер вместо стандартного: документ целиком, с версией формата,
// чтобы после перезапуска не зависеть от загруженных наборов.
[[nodiscard]] DocumentData *LoadHintSticker(not_null<Main::Session*> session) {
	const auto encoded = Mokagram::Settings::String(
		QString::fromLatin1(kHintStickerKey));
	if (encoded.isEmpty()) {
		return nullptr;
	}
	auto bytes = QByteArray::fromBase64(encoded.toLatin1());
	auto stream = QDataStream(&bytes, QIODevice::ReadOnly);
	stream.setVersion(QDataStream::Qt_5_1);
	auto version = qint32();
	stream >> version;
	const auto document = Serialize::Document::readFromStream(
		session,
		version,
		stream);
	return (stream.status() == QDataStream::Ok
		&& document
		&& document->sticker())
		? document
		: nullptr;
}

void SaveHintSticker(DocumentData *document) {
	const auto key = QString::fromLatin1(kHintStickerKey);
	if (!document) {
		Mokagram::Settings::Reset(key);
		return;
	}
	auto bytes = QByteArray();
	{
		auto stream = QDataStream(&bytes, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << qint32(AppVersion);
		Serialize::Document::writeToStream(stream, document);
	}
	Mokagram::Settings::SetString(key, QString::fromLatin1(bytes.toBase64()));
}

class HintStickerMenu {
public:
	void show(
			QWidget *parent,
			QWidget *panelParent,
			not_null<Window::SessionController*> controller,
			QPoint global) {
		_menu = base::make_unique_q<Ui::PopupMenu>(
			parent,
			st::popupMenuWithIcons);
		_menu->addAction(
			QString::fromUtf8(Mokagram::Lang::Tr("Сменить стикер")),
			[=] { choose(panelParent, controller, global); },
			&st::menuIconStickers);
		if (!Mokagram::Settings::String(
				QString::fromLatin1(kHintStickerKey)).isEmpty()) {
			_menu->addAction(
				QString::fromUtf8(Mokagram::Lang::Tr("Вернуть стандартный")),
				[] { SaveHintSticker(nullptr); },
				&st::menuIconRestore);
		}
		_menu->popup(global);
	}

	void hide() {
		_menu.reset();
		if (_panel) {
			_panel->hideFast();
		}
	}

private:
	void choose(
			QWidget *parent,
			not_null<Window::SessionController*> controller,
			QPoint global) {
		if (!_panel) {
			using Selector = ChatHelpers::TabbedSelector;
			_panel = base::make_unique_q<ChatHelpers::TabbedPanel>(
				parent,
				controller,
				object_ptr<Selector>(
					nullptr,
					controller->uiShow(),
					Window::GifPauseReason::Layer,
					Selector::Mode::StickersOnly));
			const auto panel = _panel.get();
			panel->setDesiredHeightValues(
				1.,
				st::emojiPanMinHeight / 2,
				st::emojiPanMinHeight);
			panel->selector()->setCurrentPeer(controller->session().user());
			panel->selector()->fileChosen(
			) | rpl::on_next([=](ChatHelpers::FileChosen data) {
				if (data.document->sticker()) {
					SaveHintSticker(data.document);
					panel->hideAnimated();
				}
			}, panel->lifetime());
		}
		const auto panel = _panel.get();
		const auto local = parent->mapFromGlobal(global);
		const auto bottom = std::clamp(
			local.y(),
			std::min(panel->height(), parent->height()),
			parent->height());
		panel->moveBottomRight(bottom, local.x() + panel->width() / 2);
		panel->showAnimated();
	}

	base::unique_qptr<Ui::PopupMenu> _menu;
	base::unique_qptr<ChatHelpers::TabbedPanel> _panel;
};

class StickerHint final : public Ui::RpWidget {
public:
	enum class Mode { Compact, Fullscreen };
	static constexpr auto kVideoWidth = 512;
	static constexpr auto kVideoHeight = 377;
	static constexpr auto kVideoMaxWidth = 220;

	StickerHint(
		QWidget *parent,
		not_null<Main::Session*> session,
		Fn<void(QPoint)> showMenu,
		Mode mode = Mode::Compact)
	: RpWidget(parent)
	, _session(session)
	, _showMenu(std::move(showMenu))
	, _mode(mode) {
		if (_mode == Mode::Fullscreen) {
			setFocusPolicy(Qt::StrongFocus);
			connect(qApp, &QGuiApplication::applicationStateChanged, this, [=] {
				update();
			});
			PowerSaving::Changes(
			) | rpl::on_next([=] { update(); }, lifetime());
		} else {
			_sheet = QPixmap(u":/gui/art/mokagram_music_hint.png"_q);
			_sheet.setDevicePixelRatio(2.);
			resize(width(), kSide + 56);
		}
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
		Mokagram::Settings::StringValue(
			QString::fromLatin1(kHintStickerKey)
		) | rpl::on_next([=](const QString &) {
			loadSticker();
		}, lifetime());
		if (_mode == Mode::Compact && !PowerSaving::On(PowerSaving::kAnimations)) {
			_started = crl::now();
			_animation.init([=](crl::time) {
				update();
				return true;
			});
			_animation.start();
		}
	}

protected:
	void contextMenuEvent(QContextMenuEvent *e) override {
		if (_showMenu) {
			const auto global = (e->reason() == QContextMenuEvent::Keyboard)
				? mapToGlobal(rect().center())
				: e->globalPos();
			_showMenu(global);
			e->accept();
		}
	}

private:
	void loadSticker() {
		_playerLifetime.destroy();
		_player = nullptr;
		_custom = LoadHintSticker(_session);
		update();
		if (!_custom) {
			return;
		}
		const auto document = not_null<DocumentData*>(_custom);
		const auto media = document->createMediaView();
		media->checkStickerLarge();
		media->goodThumbnailWanted();
		rpl::single() | rpl::then(
			_session->downloaderTaskFinished()
		) | rpl::filter([=] {
			return media->loaded();
		}) | rpl::take(1) | rpl::on_next([=] {
			using namespace HistoryView;
			const auto info = document->sticker();
			const auto box = (_mode == Mode::Fullscreen)
				? QSize(style::ConvertScale(kVideoMaxWidth), style::ConvertScale(kVideoMaxWidth))
				: QSize(kSide, kSide);
			if (info->isLottie()) {
				_player = std::make_shared<LottiePlayer>(
					ChatHelpers::LottiePlayerFromDocument(
						media.get(),
						ChatHelpers::StickerLottieSize::ChatIntroHelloSticker,
						box,
						Lottie::Quality::High));
			} else if (info->isWebm()) {
				_player = std::make_shared<WebmPlayer>(
					media->owner()->location(),
					media->bytes(),
					box);
			} else {
				_player = std::make_shared<StaticStickerPlayer>(
					media->owner()->location(),
					media->bytes(),
					box);
			}
			_player->setRepaintCallback([=] { update(); });
			update();
		}, _playerLifetime);
	}

	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto fullscreen = (_mode == Mode::Fullscreen);
		const auto target = fullscreen
			? rect()
			: QRect((width() - kSide) / 2, 28, kSide, kSide);
		if (target.isEmpty()) {
			return;
		}
		const auto paused = fullscreen
			? (!isVisible() || !Mokagram::Perf::UserIsLooking()
				|| PowerSaving::On(PowerSaving::kAnimations))
			: (!_started || !window()->isActiveWindow());
		if (_player && _player->ready()) {
			const auto fitted = fullscreen
				? _custom->dimensions.scaled(target.size(), Qt::KeepAspectRatio)
				: target.size();
			const auto frame = _player->frame(
				fitted.isEmpty() ? target.size() : fitted,
				QColor(0, 0, 0, 0),
				false,
				crl::now(),
				paused).image;
			if (!frame.isNull()) {
				const auto size = HistoryView::DownscaledSize(
					frame.size() / style::DevicePixelRatio(),
					target.size());
				p.drawImage(
					QRect(
						target.x() + (target.width() - size.width()) / 2,
						target.y() + (target.height() - size.height()) / 2,
						size.width(),
						size.height()),
					frame);
				if (!fullscreen || !paused) {
					_player->markFrameShown();
				}
				_video = nullptr;
				return;
			}
		}
		if (fullscreen) {
			paintVideo(p, target, paused);
			return;
		}
		if (_custom || _sheet.isNull()) {
			return;
		}
		const auto frame = _started
			? int(((crl::now() - _started) / kFrameDuration) % kFrames)
			: 0;
		// Кадр в листе задан в пикселях устройства, а целевой квадрат —
		// в логических, иначе при DPR 2 картинка ужимается и уезжает влево.
		p.drawPixmap(
			target,
			_sheet,
			QRect(
				(frame % kColumns) * kSource,
				(frame / kColumns) * kSource,
				kSource,
				kSource));
	}

	void paintVideo(QPainter &p, const QRect &target, bool paused) {
		if (!_video) {
			auto file = QFile(u":/animations/mokagram_search_empty.webm"_q);
			if (!file.open(QIODevice::ReadOnly)) {
				return;
			}
			_video = Media::Clip::MakeReader(
				file.readAll(),
				[=](Media::Clip::Notification) { update(); });
		}
		if (!_video->ready()) {
			return;
		}
		const auto request = Media::Clip::FrameRequest{
			.frame = target.size(),
			.outer = target.size(),
			.factor = style::DevicePixelRatio(),
		};
		if (!_video->started()) {
			_video->start(request);
		}
		p.drawImage(target, _video->current(request, paused ? 0 : crl::now()));
	}

	static constexpr auto kSource = 192;
	static constexpr auto kSide = kSource / 2;
	static constexpr auto kColumns = 9;
	static constexpr auto kFrames = 49;
	static constexpr auto kFrameDuration = crl::time(1000 / 15);

	const not_null<Main::Session*> _session;
	const Fn<void(QPoint)> _showMenu;
	const Mode _mode;
	QPixmap _sheet;
	Media::Clip::ReaderPointer _video;
	Ui::Animations::Basic _animation;
	crl::time _started = 0;
	DocumentData *_custom = nullptr;
	std::shared_ptr<HistoryView::StickerPlayer> _player;
	rpl::lifetime _playerLifetime;

};

[[nodiscard]] QString FormatSeekTime(qint64 seconds) {
	const auto time = std::max<qint64>(seconds, 0);
	return u"%1:%2"_q.arg(time / 60).arg(time % 60, 2, 10, QChar('0'));
}

[[nodiscard]] QColor ContrastingForeground(const QColor &background) {
	return Ui::CountContrast(background, Qt::white) >= Ui::CountContrast(background, Qt::black)
		? QColor(Qt::white)
		: QColor(Qt::black);
}

struct PlaybackBarColors {
	QColor surface;
	QColor onSurface;
	QColor onSurfaceVariant;
	QColor primary;
	QColor inactiveTrack;
};

[[nodiscard]] PlaybackBarColors MakePlaybackBarColors(QColor accent = {}, bool now = false) {
	const auto readable = [](QColor background, QColor ink, QColor alternate) {
		return (Ui::CountContrast(background, ink) >= 4.5) ? ink : alternate;
	};
	const auto opaque = [](QColor color) {
		color.setAlpha(255);
		return color;
	};
	const auto base = PlaybackSurfaceColor();
	const auto surface = accent.isValid()
		? anim::color(base, opaque(accent), IsLightMusicTheme() ? .16 : .32)
		: base;
	const auto onSurface = readable(
		surface, opaque(st::windowFg->c), ContrastingForeground(surface));
	auto primary = accent.isValid()
		? opaque(accent)
		: now ? onSurface : opaque(st::activeButtonBg->c);
	if (accent.isValid() && Ui::CountContrast(surface, primary) < 3.) {
		primary = anim::color(primary, onSurface, .48);
	}
	if (Ui::CountContrast(surface, primary) < 3.) {
		primary = accent.isValid() ? onSurface : opaque(st::windowActiveTextFg->c);
		if (Ui::CountContrast(surface, primary) < 3.) {
			primary = onSurface;
		}
	}
	return {
		.surface = surface,
		.onSurface = onSurface,
		.onSurfaceVariant = readable(
			surface, opaque(st::windowSubTextFg->c), onSurface),
		.primary = primary,
		.inactiveTrack = anim::color(surface, onSurface, .25),
	};
}

[[nodiscard]] double PlaybackStateLayer(double hover, double press) {
	return std::max(.08 * hover, .12 * press);
}

class SeekBar final : public Ui::RpWidget {
public:
	explicit SeekBar(QWidget *parent, const PlaybackBarColors *colors = nullptr)
	: RpWidget(parent)
	, _colors(colors) {
		resize(width(), 28);
		setCursor(style::cur_pointer);
		Mokagram::Settings::Changed() | rpl::on_next([=](const QString &key) {
			if (_expressive && key == u"player.expressive_seek"_q) {
				updateWaveAnimation();
				update();
			}
		}, lifetime());
		paintRequest(
		) | rpl::on_next([=](QRect area) {
			auto p = QPainter(this);
			if (testAttribute(Qt::WA_OpaquePaintEvent)) {
				p.fillRect(area, _colors->surface);
			}
			auto hq = PainterHighQualityEnabler(p);
			if (_thin) {
				if (showsWave()) {
					paintWave(p);
					return;
				}
				// Тонкая полоса: толще на наведении и при перетаскивании.
				const auto grow = _grow.value((_over || _dragging) ? 1. : 0.);
				const auto h = 4. + 2. * grow;
				const auto line = QRectF(0, height() / 2. - h / 2., width(), h);
				p.setPen(Qt::NoPen);
				p.setBrush(_colors->inactiveTrack);
				p.drawRoundedRect(line, h / 2., h / 2.);
				const auto progress = _dragging ? _drag : _progress;
				auto fill = line;
				fill.setWidth(line.width() * progress);
				if (progress > 0.) {
					p.setBrush(_colors->primary);
					p.drawRoundedRect(fill, h / 2., h / 2.);
				}
				if (grow > 0.) {
					const auto radius = std::min(5. * grow, line.width() / 2.);
					const auto x = std::clamp(line.width() * progress, radius, line.width() - radius);
					p.setBrush(_colors->onSurface);
					p.drawEllipse(QPointF(x, line.center().y()), radius, radius);
				}
				return;
			}
			const auto line = QRectF(8, height() / 2. - 2, width() - 16, 4);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(line, 3, 3);
			const auto progress = _dragging ? _drag : _progress;
			auto fill = line;
			fill.setWidth(line.width() * progress);
			p.setBrush(st::windowFg);
			p.drawRoundedRect(fill, 2, 2);
			const auto knob = QPointF(fill.right(), line.center().y());
			p.setBrush(Qt::white);
			p.drawEllipse(knob, _dragging ? 8. : 6., _dragging ? 8. : 6.);
		}, lifetime());
	}

	void enableTimePreview() {
		_timePreview = Ui::CreateChild<Ui::RpWidget>(parentWidget());
		_timePreview->setAttribute(Qt::WA_TransparentForMouseEvents);
		_timePreview->paintOn([=](QPainter &p) {
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			const auto radius = _timePreview->height() / 2.;
			p.drawRoundedRect(_timePreview->rect(), radius, radius);
			p.setFont(st::semiboldFont);
			p.setPen(st::windowFg);
			p.drawText(_timePreview->rect(), Qt::AlignCenter, _previewTime);
		});
		_timePreview->hide();
	}

	void setDuration(qint64 duration) {
		if (_duration == duration) return;
		_duration = duration;
		refreshTimePreview(mapFromGlobal(QCursor::pos()).x());
	}

	void setOpaqueBackground(bool opaque) {
		if (testAttribute(Qt::WA_OpaquePaintEvent) != opaque) {
			setAttribute(Qt::WA_OpaquePaintEvent, opaque);
			update();
		}
	}

	void setExpressive(bool expressive, bool playing) {
		if (_expressive == expressive && _wavePlaying == playing) {
			return;
		}
		_expressive = expressive;
		_wavePlaying = playing;
		updateWaveAnimation();
		update();
	}
	void setThin(bool thin) {
		_thin = thin;
		setMouseTracking(true);
		updateWaveAnimation();
		update();
	}
	void setProgress(float64 value) {
		value = std::clamp(value, 0., 1.);
		if (_progress == value) {
			return;
		}
		_progress = value;
		if (!_dragging) {
			update();
		}
	}
	[[nodiscard]] bool waveAnimating() const {
		return _waveAnimating.current();
	}
	[[nodiscard]] rpl::producer<bool> waveAnimationChanges() const {
		return _waveAnimating.changes();
	}
	void advanceWave(float64 elapsed) {
		const auto wanted = _wavePlaying ? 1. : 0.;
		_wavePhase += elapsed * 2. * wanted;
		_waveAmp += (wanted - _waveAmp) * (1. - std::exp(-elapsed * 8.));
		if (!_wavePlaying && _waveAmp <= 0.01) {
			_waveAmp = 0.;
			_waveAnimating = false;
		}
		update();
	}
	[[nodiscard]] rpl::producer<float64> sought() const {
		return _sought.events();
	}
protected:
	void showEvent(QShowEvent *e) override {
		RpWidget::showEvent(e);
		updateWaveAnimation();
	}
	void hideEvent(QHideEvent *e) override {
		_waveAnimating = false;
		if (_timePreview) _timePreview->hide();
		RpWidget::hideEvent(e);
	}
	void enterEventHook(QEnterEvent *e) override {
		setOver(true);
		refreshTimePreview(e->position().toPoint().x());
	}
	void leaveEventHook(QEvent *e) override {
		setOver(false);
		refreshTimePreview(mapFromGlobal(QCursor::pos()).x());
	}
	void mousePressEvent(QMouseEvent *e) override {
		if (e->button() != Qt::LeftButton) {
			e->ignore();
			return;
		}
		_dragging = true;
		startGrow();
		moveTo(e->pos().x());
		refreshTimePreview(e->pos().x());
	}
	void mouseMoveEvent(QMouseEvent *e) override {
		if (_dragging) {
			moveTo(e->pos().x());
		}
		refreshTimePreview(e->pos().x());
	}
	void mouseReleaseEvent(QMouseEvent *e) override {
		if (_dragging && e->button() == Qt::LeftButton) {
			_dragging = false;
			_over = rect().contains(e->pos());
			startGrow();
			moveTo(e->pos().x());
			_progress = _drag;
			refreshTimePreview(e->pos().x());
			_sought.fire_copy(_drag);
		}
	}

private:
	[[nodiscard]] bool showsWave() const {
		return _thin && _expressive && !_over && !_dragging
			&& Mokagram::Settings::Bool(u"player.expressive_seek"_q);
	}
	void updateWaveAnimation() {
		_waveAnimating = isVisible() && showsWave()
			&& (_wavePlaying || _waveAmp > 0.01);
	}
	void paintWave(QPainter &p) {
		const auto y = height() / 2.;
		const auto end = width() * _progress;
		const auto phase = _wavePhase;
		const auto amplitude = st::motionWaveAmplitude
			* std::max(_waveAmp, _wavePlaying ? .65 : 0.);
		p.setPen(QPen(_colors->inactiveTrack, st::motionWaveStroke,
			Qt::SolidLine, Qt::RoundCap));
		p.drawLine(QPointF(std::min(end + st::motionWaveGap, double(width())), y),
			QPointF(width(), y));
		auto path = QPainterPath();
		for (auto x = 0.; x <= end; x += std::max(st::motionWaveSample, 1)) {
			const auto envelope = std::clamp((end - x) / st::motionWaveLength, 0., 1.);
			const auto point = QPointF(x, y + amplitude * envelope
				* std::sin(x * 2. * M_PI / st::motionWaveLength - phase));
			if (x == 0.) path.moveTo(point); else path.lineTo(point);
		}
		p.setPen(QPen(_colors->primary, st::motionWaveStroke, Qt::SolidLine, Qt::RoundCap));
		p.drawPath(path);
		p.setPen(Qt::NoPen);
		p.setBrush(_colors->primary);
		const auto handle = st::motionWaveHandle;
		p.drawRoundedRect(QRectF(end - handle / 2.,
			y - st::motionWaveHandleHeight / 2., handle, st::motionWaveHandleHeight),
			handle / 2., handle / 2.);
	}

	void setOver(bool over) {
		if (_over != over) {
			_over = over;
			startGrow();
		}
	}
	void startGrow() {
		updateWaveAnimation();
		const auto to = (_over || _dragging) ? 1. : 0.;
		_grow.start([=] { update(); }, 1. - to, to, 180, anim::easeOutCubic);
	}
	[[nodiscard]] float64 progressAt(int x) const {
		const auto pad = _thin ? 0 : 8;
		return std::clamp((x - pad) / float64(std::max(width() - 2 * pad, 1)), 0., 1.);
	}
	void moveTo(int x) {
		_drag = progressAt(x);
		update();
	}
	void refreshTimePreview(int x) {
		if (!_timePreview) return;
		if (!isVisible() || (!_over && !_dragging) || _duration <= 0) {
			_timePreview->hide();
			return;
		}

		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto progress = progressAt(x);
		const auto time = FormatSeekTime(qint64(_duration * progress / 1000.));
		const auto width = st::semiboldFont->width(time) + s(16);
		const auto left = this->x() + std::clamp(x, 0, this->width()) - width / 2;
		_timePreview->setGeometry(
			std::clamp(left, 0, std::max(parentWidget()->width() - width, 0)),
			y() - s(21), width, s(22));
		if (time != _previewTime) {
			_previewTime = time;
			_timePreview->update();
		}
		_timePreview->raise();
		_timePreview->show();
	}

	const PlaybackBarColors *const _colors;
	Ui::RpWidget *_timePreview = nullptr;
	QString _previewTime;
	qint64 _duration = 0;
	Ui::Animations::Simple _grow;
	rpl::variable<bool> _waveAnimating = false;
	bool _expressive = false;
	bool _wavePlaying = false;
	float64 _wavePhase = 0.;
	float64 _waveAmp = 0.;
	bool _thin = false;
	bool _over = false;
	float64 _progress = 0.;
	float64 _drag = 0.;
	bool _dragging = false;
	rpl::event_stream<float64> _sought;

};

// Значки островка (SVG из mokagram_island_glyphs.h) в квадрат size по центру c.
void PaintIslandGlyph(
		QPainter &p,
		const char *svg,
		QPointF c,
		int size,
		QColor color) {
	using Key = std::tuple<const void*, int, QRgb>;
	static auto cache = base::flat_map<Key, QImage>();
	const auto ratio = style::DevicePixelRatio();
	const auto key = Key(svg, size, color.rgba());
	auto i = cache.find(key);
	if (i == end(cache)) {
		auto image = QImage(
			QSize(size, size) * ratio,
			QImage::Format_ARGB32_Premultiplied);
		image.setDevicePixelRatio(ratio);
		image.fill(Qt::transparent);
		auto q = QPainter(&image);
		auto renderer = QSvgRenderer(QByteArray(svg));
		renderer.render(&q, QRectF(0, 0, size, size));
		q.setCompositionMode(QPainter::CompositionMode_SourceIn);
		q.fillRect(QRectF(0, 0, size, size), color);
		q.end();
		i = cache.emplace(key, std::move(image)).first;
	}
	p.drawImage(QPointF(c.x() - size / 2., c.y() - size / 2.), i->second);
}

class VolumeSlider final : public Ui::ContinuousSlider {
public:
	VolumeSlider(QWidget *parent, const PlaybackBarColors &colors)
	: ContinuousSlider(parent)
	, _colors(colors) {
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Громкость")));
	}

	void setHovered(bool hovered) {
		_hovered = hovered;
		refreshHighlight();
	}

protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto seek = QRectF(myrtlrect(getSeekRect()));
		const auto thickness = style::ConvertScale(4);
		const auto y = seek.center().y();
		const auto from = seek.left();
		const auto to = seek.right();
		const auto value = getCurrentValue();
		const auto x = from + (to - from) * value;
		const auto highlight = _highlight.value();
		p.setPen(QPen(_colors.inactiveTrack, thickness,
			Qt::SolidLine, Qt::RoundCap));
		p.drawLine(QPointF(from, y), QPointF(to, y));
		if (value > 0.) {
			const auto color = anim::color(_colors.onSurface, _colors.primary, highlight);
			p.setPen(QPen(color, thickness, Qt::SolidLine, Qt::RoundCap));
			p.drawLine(QPointF(from, y), QPointF(x, y));
		}
		if (highlight > 0.) {
			p.setOpacity(highlight);
			p.setPen(Qt::NoPen);
			p.setBrush(_colors.onSurface);
			const auto radius = style::ConvertScale(6);
			p.drawEllipse(QPointF(x, y), radius, radius);
		}
	}

	void mousePressEvent(QMouseEvent *e) override {
		if (e->button() != Qt::LeftButton) {
			e->ignore();
			return;
		}
		ContinuousSlider::mousePressEvent(e);
		refreshHighlight();
	}

	void mouseReleaseEvent(QMouseEvent *e) override {
		if (e->button() != Qt::LeftButton) {
			e->ignore();
			return;
		}
		ContinuousSlider::mouseMoveEvent(e);
		ContinuousSlider::mouseReleaseEvent(e);
		refreshHighlight();
	}

	void focusInEvent(QFocusEvent *e) override {
		ContinuousSlider::focusInEvent(e);
		refreshHighlight();
	}

	void focusOutEvent(QFocusEvent *e) override {
		ContinuousSlider::focusOutEvent(e);
		refreshHighlight();
	}

private:
	// The parent tracks hover across the speaker and the slider together.
	void enterEventHook(QEnterEvent *) override {
	}
	void leaveEventHook(QEvent *) override {
	}
	float64 getOverDuration() const override {
		return 160.;
	}
	QSize getSeekDecreaseSize() const override {
		return QSize(2 * style::ConvertScale(6), 0);
	}
	void refreshHighlight() {
		_highlight.set(_hovered || isChanging() || hasFocus(), [=] { update(); });
	}

	const PlaybackBarColors &_colors;
	HoverFade _highlight;
	bool _hovered = false;
};

class VolumeControl final : public Ui::RpWidget {
public:
	VolumeControl(QWidget *parent, const PlaybackBarColors &colors)
	: RpWidget(parent)
	, _mute(Ui::CreateChild<Ui::AbstractButton>(this))
	, _slider(Ui::CreateChild<VolumeSlider>(this, colors)) {
		resize(width(), style::ConvertScale(28));
		_mute->setPointerCursor(true);
		_mute->setFocusPolicy(Qt::TabFocus);
		_mute->paintRequest() | rpl::on_next([=, &colors](QRect) {
			auto p = QPainter(_mute);
			auto hq = PainterHighQualityEnabler(p);
			const auto bounds = QRectF(_mute->rect());
			if (_mute->hasFocus()) {
				const auto inset = style::ConvertScale(2);
				p.setBrush(Qt::NoBrush);
				p.setPen(QPen(colors.onSurface, inset));
				p.drawEllipse(bounds.adjusted(inset, inset, -inset, -inset));
			}
			using namespace Mokagram::Music::IslandGlyphs;
			PaintIslandGlyph(p, _muted ? kGlyphSpeaker : kGlyphOutput,
				bounds.center(), style::ConvertScale(20),
				_mute->isOver() || _mute->hasFocus() ? colors.onSurface : colors.onSurfaceVariant);
		}, _mute->lifetime());

		const auto setVolume = [](float64 value) {
			auto &app = Core::App();
			if (app.settings().songVolume() != value) {
				Media::Player::mixer()->setSongVolume(value);
				app.settings().setSongVolume(value);
				app.saveSettingsDelayed();
			}
		};
		_slider->setChangeProgressCallback(setVolume);
		_slider->setChangeFinishedCallback([](float64 value) {
			if (value > 0.) {
				Core::App().settings().setRememberedSongVolume(value);
			}
		});
		_mute->setClickedCallback([=] {
			auto &settings = Core::App().settings();
			const auto volume = settings.songVolume();
			if (volume > 0.) {
				settings.setRememberedSongVolume(volume);
				setVolume(0.);
			} else {
				setVolume(settings.rememberedSongVolume());
			}
		});
		const auto refresh = [=](float64 volume) {
			const auto value = std::clamp(volume, 0., 1.);
			_slider->setValue(value);
			if (value > 0. && !_slider->isChanging()) {
				Core::App().settings().setRememberedSongVolume(value);
			}
			const auto muted = (value == 0.);
			if (_muted != muted || _mute->accessibleName().isEmpty()) {
				_muted = muted;
				const auto text = QString::fromUtf8(Mokagram::Lang::Tr(
					muted ? "Включить звук" : "Выключить звук"));
				_mute->setAccessibleName(text);
				_mute->setToolTip(text);
				_mute->update();
			}
		};
		const auto &settings = Core::App().settings();
		refresh(settings.songVolume());
		settings.songVolumeChanges() | rpl::on_next(refresh, lifetime());
	}

protected:
	void enterEventHook(QEnterEvent *e) override {
		RpWidget::enterEventHook(e);
		_slider->setHovered(true);
	}
	void leaveEventHook(QEvent *e) override {
		RpWidget::leaveEventHook(e);
		_slider->setHovered(false);
	}
	void hideEvent(QHideEvent *e) override {
		_slider->setHovered(false);
		RpWidget::hideEvent(e);
	}
	void resizeEvent(QResizeEvent *) override {
		const auto iconWidth = style::ConvertScale(28);
		_mute->setGeometry(0, 0, iconWidth, height());
		_slider->setGeometry(iconWidth, 0, std::max(width() - iconWidth, 0), height());
	}

private:
	const not_null<Ui::AbstractButton*> _mute;
	const not_null<VolumeSlider*> _slider;
	bool _muted = false;
};

enum class Glyph { Previous, Next, Play, Add, Shuffle, Repeat, More, Block, Collapse, Queue, Filter };

[[nodiscard]] QColor PaintPlayerHeaderButton(
		QPainter &p,
		QRectF rect,
		QColor background,
		double hovered,
		double pressed) {
	background.setAlpha(255);
	if (hovered > 0. || pressed > 0.) {
		background = anim::color(
			background, ContrastingForeground(background), 0.1 * hovered + 0.08 * pressed);
	}
	p.setPen(Qt::NoPen);
	p.setBrush(background);
	const auto radius = rect.height() / 2.;
	p.drawRoundedRect(rect, radius, radius);
	return ContrastingForeground(background);
}

class RoundGlyph final : public Ui::AbstractButton {
public:
	RoundGlyph(QWidget *parent, Glyph glyph, int size, Fn<QColor()> headerBackground = nullptr)
	: AbstractButton(parent)
	, _glyph(glyph)
	, _headerBackground(std::move(headerBackground)) {
		resize(size, size);
		setPointerCursor(true);
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
	}

	void setPlaying(bool playing) {
		if (_playing != playing) {
			_playing = playing;
			update();
		}
	}
	void setLiked(bool liked) {
		if (_liked != liked) {
			_liked = liked;
			if ((_overlay || _playbackColors) && liked) {
				_pop.start([=] { update(); }, 0., 1., 360, anim::easeOutCubic);
			}
			update();
		}
	}
	void animateLike(bool liked) {
		setLiked(liked);
		if (LikeAnimationsEnabled()) {
			_likeAnimation.stop();
			_likeAnimation.start([=] { update(); }, 0., 1., kLikeDuration);
		}
	}
	void setPlaybackColors(const PlaybackBarColors &colors) {
		_playbackColors = &colors;
		update();
	}
	void setQueueOpen(bool open) {
		_queueOpen = open;
		update();
	}
	// Кнопки полноэкранного плеера: белые значки, тёмный полупрозрачный круг.
	void setOverlay(bool overlay) {
		_overlay = overlay;
		update();
	}

protected:
	void onStateChanged(State was, StateChangeSource source) override {
		AbstractButton::onStateChanged(was, source);
		if ((_overlay || _playbackColors) && ((was & StateFlag::Over) != (state() & StateFlag::Over))) {
			const auto over = isOver();
			_hover.start([=] { update(); }, over ? 0. : 1., over ? 1. : 0., 180, anim::easeOutCubic);
		}
		if ((_overlay || _playbackColors) && ((was & StateFlag::Down) != (state() & StateFlag::Down))) {
			const auto down = isDown();
			_press.start([=] { update(); }, down ? 0. : 1., down ? 1. : 0., 120, anim::easeOutCubic);
		}
	}

private:
	void paintOverlay(QPainter &p) {
		auto hq = PainterHighQualityEnabler(p);
		const auto r = QRectF(rect());
		auto c = r.center();
		const auto over = _hover.value(isOver() ? 1. : 0.);
		const auto down = _press.value(isDown() ? 1. : 0.);
		const auto play = (_glyph == Glyph::Play);
		const auto dark = (_glyph == Glyph::Collapse);
		p.translate(c);
		const auto squeeze = 1. - 0.08 * down;
		p.scale(squeeze, squeeze);
		p.translate(-c);
		const auto background = _headerBackground ? _headerBackground() : QColor();
		auto foreground = QColor(Qt::white);
		if (background.isValid()) {
			foreground = PaintPlayerHeaderButton(p, r, background, over, down);
		} else {
			p.setPen(Qt::NoPen);
			// Play: тёмный полупрозрачный круг с белым значком, как кнопки в шапке.
			p.setBrush(play
				? QColor(255, 255, 255, int(36 + 20 * over))
				: dark
				? QColor(0, 0, 0, int(80 + 50 * over))
				: QColor(255, 255, 255, int(40 * over)));
			p.drawEllipse(r);
		}
		const auto pen = QPen(foreground, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
		const auto u = r.width() / 48.;
		if (_glyph == Glyph::Collapse) {
			p.setPen(pen);
			p.setBrush(Qt::NoBrush);
			p.drawPolyline(QPolygonF({
				c + QPointF(-7, -3) * u * 1.2,
				c + QPointF(0, 4) * u * 1.2,
				c + QPointF(7, -3) * u * 1.2 }));
			return;
		}
		if (_glyph == Glyph::Filter) {
			p.setPen(pen);
			for (auto i = 0; i != 3; ++i) {
				const auto x = c.x() + (i - 1) * 7 * u;
				const auto y = c.y() + (i - 1) * 3 * u;
				p.drawLine(QPointF(x, c.y() - 9 * u), QPointF(x, y - 2 * u));
				p.drawLine(QPointF(x, y + 2 * u), QPointF(x, c.y() + 9 * u));
				p.drawEllipse(QPointF(x, y), 2 * u, 2 * u);
			}
			return;
		}
		if (_glyph == Glyph::Queue) {
			p.setPen(pen);
			p.setBrush(Qt::NoBrush);
			for (auto i = 0; i != 3; ++i) {
				const auto y = c.y() + (i - 1) * 6 * u * 1.2;
				p.drawLine(QPointF(c.x() - 3 * u * 1.2, y), QPointF(c.x() + 8 * u * 1.2, y));
				p.drawPoint(QPointF(c.x() - 8 * u * 1.2, y));
			}
			return;
		}
		if (_glyph == Glyph::Add) {
			const auto pop = 1. + 0.3 * std::sin(_pop.value(1.) * M_PI) + 0.06 * over;
			p.translate(c);
			const auto base = (width() >= 44) ? 1.05 : 0.8;
			p.scale(pop * base, pop * base);
			auto heart = QPainterPath();
			heart.moveTo(QPointF(0, 8.5));
			heart.cubicTo(QPointF(-14, -1), QPointF(-8.5, -11.5), QPointF(0, -4));
			heart.cubicTo(QPointF(8.5, -11.5), QPointF(14, -1), QPointF(0, 8.5));
			heart.closeSubpath();
			if (_liked) {
				p.setPen(Qt::NoPen);
				p.setBrush(QColor(0xFF, 0x3B, 0x5C));
			} else {
				p.setPen(QPen(Qt::white, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				p.setBrush(Qt::NoBrush);
			}
			p.drawPath(heart);
			return;
		}
		if (_glyph == Glyph::Shuffle || _glyph == Glyph::Repeat) {
			const auto &settings = Core::App().settings();
			const auto shuffle = (_glyph == Glyph::Shuffle);
			const auto mode = settings.playerRepeatMode();
			const auto active = shuffle
				? settings.playerOrderMode() == Media::OrderMode::Shuffle
				: mode != Media::RepeatMode::None;
			const auto &icon = shuffle
				? st::mediaPlayerOrderIconShuffle
				: mode == Media::RepeatMode::One
				? st::mediaPlayerRepeatOneIcon
				: st::mediaPlayerRepeatDisabledIcon;
			// Включённый режим: яркий значок и точка под ним, выключенный приглушён.
			if (active) {
				p.setBrush(Qt::white);
				p.drawEllipse(QPointF(c.x(), c.y() + 17. * u), 2. * u, 2. * u);
			}
			const auto scale = 28. / std::max(icon.width(), icon.height());
			p.translate(
				c.x() - icon.width() * scale / 2.,
				c.y() - icon.height() * scale / 2.);
			p.scale(scale, scale);
			icon.paint(
				p,
				QPoint(),
				icon.width(),
				active ? QColor(Qt::white) : QColor(255, 255, 255, 150));
			return;
		}
		p.setBrush(Qt::white);
		p.setPen(Qt::NoPen);
		if (_glyph == Glyph::Play) {
			if (_playing) {
				p.drawRoundedRect(QRectF(c.x() - 11 * u, c.y() - 12 * u, 8 * u, 24 * u), 2.5 * u, 2.5 * u);
				p.drawRoundedRect(QRectF(c.x() + 3 * u, c.y() - 12 * u, 8 * u, 24 * u), 2.5 * u, 2.5 * u);
			} else {
				auto path = QPainterPath();
				path.moveTo(c.x() - 7 * u, c.y() - 12 * u);
				path.lineTo(c.x() + 13 * u, c.y());
				path.lineTo(c.x() - 7 * u, c.y() + 12 * u);
				path.closeSubpath();
				p.drawPath(path);
			}
			return;
		}
		const auto dir = (_glyph == Glyph::Next) ? 1. : -1.;
		auto path = QPainterPath();
		path.moveTo(c.x() + dir * (-9) * u, c.y() - 11 * u);
		path.lineTo(c.x() + dir * 7 * u, c.y());
		path.lineTo(c.x() + dir * (-9) * u, c.y() + 11 * u);
		path.closeSubpath();
		p.drawPath(path);
		p.drawRoundedRect(QRectF(c.x() + dir * 9 * u - 2 * u, c.y() - 11 * u, 4 * u, 22 * u), 1.5 * u, 1.5 * u);
	}

	void paintPlaybackButton(QPainter &p) {
		using namespace Mokagram::Music::IslandGlyphs;
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto c = QRectF(rect()).center();
		const auto over = _hover.value(isOver() ? 1. : 0.);
		const auto down = _press.value(isDown() ? 1. : 0.);
		auto scale = 1. - 0.12 * down;
		const auto &colors = *_playbackColors;
		const auto layer = PlaybackStateLayer(over, down);
		auto color = colors.onSurface;
		auto svg = (const char*)nullptr;
		auto size = s(22);
		auto selected = false;
		switch (_glyph) {
		case Glyph::Previous: svg = kGlyphBack; size = s(34); break;
		case Glyph::Next: svg = kGlyphForward; size = s(34); break;
		case Glyph::Play:
			svg = _playing ? kGlyphPause : kGlyphPlay;
			size = s(30);
			break;
		case Glyph::Queue:
			svg = kGlyphQueue;
			selected = _queueOpen;
			break;
		case Glyph::Add: {
			svg = _liked ? kGlyphHeartFill : kGlyphHeart;
			color = _liked ? colors.onSurface : colors.onSurfaceVariant;
			const auto pop = _pop.value(1.);
			scale *= 1. + 0.3 * std::sin(pop * M_PI);
		} break;
		case Glyph::Shuffle:
		case Glyph::Repeat: {
			const auto &settings = Core::App().settings();
			const auto shuffle = (_glyph == Glyph::Shuffle);
			const auto mode = settings.playerRepeatMode();
			selected = shuffle
				? (settings.playerOrderMode() == Media::OrderMode::Shuffle)
				: (mode != Media::RepeatMode::None);
			svg = shuffle
				? kGlyphShuffle
				: (mode == Media::RepeatMode::One)
				? kGlyphRepeatOne
				: kGlyphRepeat;
		} break;
		default: break;
		}
		if (!svg) {
			return;
		}
		if (_glyph != Glyph::Play && (selected || layer > 0.)) {
			const auto radius = (std::min(width(), height()) - s(4)) / 2.;
			p.setPen(Qt::NoPen);
			p.setBrush(anim::color(colors.surface, colors.onSurface, selected ? .10 + layer : layer));
			p.drawEllipse(c, radius, radius);
		}
		if (_glyph == Glyph::Queue && hasFocus()) {
			const auto radius = s(17);
			p.setPen(QPen(colors.onSurface, s(1)));
			p.setBrush(Qt::NoBrush);
			p.drawEllipse(c, radius, radius);
		}
		p.translate(c);
		p.scale(scale, scale);
		PaintIslandGlyph(p, svg, QPointF(), size, color);
	}

	void paint() {
		auto p = QPainter(this);
		if (_playbackColors) {
			paintPlaybackButton(p);
			return;
		}
		if (_overlay) {
			paintOverlay(p);
			return;
		}
		auto hq = PainterHighQualityEnabler(p);
		const auto r = QRectF(rect());
		const auto c = r.center();
		p.setPen(Qt::NoPen);
		if (_glyph == Glyph::Add) {
			// Лайк: контурное сердце, после добавления в профиль закрашено.
			auto heart = QPainterPath();
			heart.moveTo(c + QPointF(0, 8.5));
			heart.cubicTo(
				c + QPointF(-14, -1),
				c + QPointF(-8.5, -11.5),
				c + QPointF(0, -4));
			heart.cubicTo(
				c + QPointF(8.5, -11.5),
				c + QPointF(14, -1),
				c + QPointF(0, 8.5));
			heart.closeSubpath();
			const auto progress = _likeAnimation.value(1.);
			if (progress < 1.) {
				if (_liked) {
					DrawLikeBurst(p, c, 18., progress, QColor(0xFF, 0x3B, 0x5C));
				}
				const auto scale = LikeScale(progress, _liked);
				p.translate(c);
				p.scale(scale, scale);
				p.translate(-c);
			}
			if (_liked) {
				p.setPen(Qt::NoPen);
				p.setBrush(QColor(0xFF, 0x3B, 0x5C));
			} else {
				p.setPen(QPen(
					QColor(st::windowFg->c),
					2.,
					Qt::SolidLine,
					Qt::RoundCap,
					Qt::RoundJoin));
				p.setBrush(Qt::NoBrush);
			}
			p.drawPath(heart);
			return;
		}
		if (_glyph == Glyph::More || _glyph == Glyph::Block) {
			const auto pen = QPen(
				isOver() ? QColor(st::windowActiveTextFg->c) : QColor(st::windowFg->c),
				2.,
				Qt::SolidLine,
				Qt::RoundCap,
				Qt::RoundJoin);
			p.setPen(pen);
			p.setBrush(Qt::NoBrush);
			if (_glyph == Glyph::More) {
				p.drawLine(c + QPointF(-8, 0), c + QPointF(8, 0));
				p.drawLine(c + QPointF(0, -8), c + QPointF(0, 8));
			} else {
				p.drawEllipse(c, 9., 9.);
				p.drawLine(c + QPointF(-6.4, 6.4), c + QPointF(6.4, -6.4));
			}
			return;
		}
		if (_glyph == Glyph::Shuffle || _glyph == Glyph::Repeat) {
			const auto &settings = Core::App().settings();
			const auto shuffle = (_glyph == Glyph::Shuffle);
			const auto mode = settings.playerRepeatMode();
			const auto active = shuffle
				? settings.playerOrderMode() == Media::OrderMode::Shuffle
				: mode != Media::RepeatMode::None;
			const auto &icon = shuffle
				? st::mediaPlayerOrderIconShuffle
				: mode == Media::RepeatMode::One
				? st::mediaPlayerRepeatOneIcon
				: st::mediaPlayerRepeatDisabledIcon;
			const auto scale = 22. / std::max(icon.width(), icon.height());
			p.translate(
				c.x() - icon.width() * scale / 2.,
				c.y() - icon.height() * scale / 2.);
			p.scale(scale, scale);
			icon.paint(p, QPoint(), icon.width(), active
				? QColor(st::windowActiveTextFg->c)
				: QColor(st::windowFg->c));
			return;
		}
		if (_glyph == Glyph::Play) {
			auto fill = QColor(st::windowFg->c);
			fill.setAlpha(isOver() ? 56 : 38);
			p.setBrush(fill);
			p.drawEllipse(r.adjusted(2, 2, -2, -2));
			p.setBrush(st::windowFg);
			const auto u = r.width() / 100.;
			if (_playing) {
				p.drawRoundedRect(QRectF(c.x() - 14 * u, c.y() - 17 * u, 10 * u, 34 * u), 3 * u, 3 * u);
				p.drawRoundedRect(QRectF(c.x() + 4 * u, c.y() - 17 * u, 10 * u, 34 * u), 3 * u, 3 * u);
			} else {
				auto path = QPainterPath();
				path.moveTo(c.x() - 11 * u, c.y() - 19 * u);
				path.lineTo(c.x() + 19 * u, c.y());
				path.lineTo(c.x() - 11 * u, c.y() + 19 * u);
				path.closeSubpath();
				p.drawPath(path);
			}
			return;
		}
		if (isOver()) {
			p.setBrush(st::windowBgOver);
			p.drawEllipse(r);
		}
		p.setBrush(st::windowFg);
		const auto u = r.width() / 48.;
		const auto dir = (_glyph == Glyph::Next) ? 1. : -1.;
		auto tri = [&](double x) {
			auto path = QPainterPath();
			path.moveTo(c.x() + dir * (x - 8) * u, c.y() - 11 * u);
			path.lineTo(c.x() + dir * (x + 8) * u, c.y());
			path.lineTo(c.x() + dir * (x - 8) * u, c.y() + 11 * u);
			path.closeSubpath();
			p.drawPath(path);
		};
		tri(-2);
		p.drawRoundedRect(QRectF(c.x() + dir * 9 * u - 2 * u, c.y() - 11 * u, 4 * u, 22 * u), 1.5 * u, 1.5 * u);
	}

	Glyph _glyph;
	Fn<QColor()> _headerBackground;
	const PlaybackBarColors *_playbackColors = nullptr;
	bool _queueOpen = false;
	bool _playing = false;
	bool _liked = false;
	Ui::Animations::Simple _likeAnimation;
	bool _overlay = false;
	Ui::Animations::Simple _hover;
	Ui::Animations::Simple _press;
	Ui::Animations::Simple _pop;

};

} // namespace

Track details::PlayingTrack() {
	const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
	const auto document = current.audio();
	if (!document) {
		return {};
	}
	const auto &track = QueueState().current;
	if (OwnsTrack(current)
		&& !track.id.isEmpty()
		&& document->id == DocumentIdForTrack(track.id)) {
		return track;
	}
	if (const auto item = document->session().data().message(current.contextId())) {
		return TrackFromMessage(item);
	}
	const auto name = Ui::Text::FormatSongNameFor(document).composedName();
	auto result = Track();
	result.id = u"telegram:%1"_q.arg(document->id);
	result.title = name.title;
	result.artist = name.performer;
	result.durationMs = document->duration();
	return result;
}

// Комментарии: у SoundCloud-трека свой id, у локальной песни id двойника.
void details::ResolveCommentsTrack(const Track &track, Fn<void(QString)> done) {
	LOG(("[moka-dbg] comments track id '%1' tg %2 title '%3' artist '%4'").arg(track.id).arg(IsTelegramTrack(track) ? 1 : 0).arg(track.title).arg(track.artist));
	if (track.id.toULongLong()
		&& QString::number(track.id.toULongLong()) == track.id) {
		done(track.id);
	} else if (!IsTelegramTrack(track)) {
		done(QString());
	} else {
		ResolveLocalSoundCloud(track, [=](Track match) {
			LOG(("[moka-dbg] comments match '%1' artist '%2'").arg(match.id).arg(match.artist));
			done(match.id);
		});
	}
}

void details::LoadPlaybackArtwork(const Track &track) {
	const auto id = track.id;
	// Обложка SC имеет приоритет; прежний поиск остаётся запасным вариантом.
	const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
	const auto document = current.audio();
	if (!document
		|| !CoverForDocument(document->id).isNull()) {
		return;
	}
	const auto documentId = document->id;
	if (!IsTelegramTrack(track)) {
		if (!track.artworkUrl.isEmpty()) {
			LoadQueuePlaybackCover(documentId, track);
		}
		return;
	}
	LoadLocalArtwork(track, [=](QImage image) {
		if (!image.isNull()) {
			SetCover(documentId, std::move(image));
			return;
		}
		FetchArtwork(
			id,
			track.title,
			track.artist,
			track.durationMs,
			[=](QImage fallback) {
				if (!fallback.isNull()) {
					SetCover(documentId, std::move(fallback));
				}
			});
	});
}

namespace {

[[nodiscard]] QColor ComputeAccent(const QImage &cover) {
	if (cover.isNull()) {
		return QColor();
	}
	const auto small = cover.scaled(
		24,
		24,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
	constexpr auto kBins = 12;
	double weight[kBins] = {};
	double sumR[kBins] = {}, sumG[kBins] = {}, sumB[kBins] = {};
	for (auto y = 0; y != small.height(); ++y) {
		for (auto x = 0; x != small.width(); ++x) {
			const auto c = QColor(small.pixel(x, y));
			const auto s = c.hsvSaturationF();
			const auto v = c.valueF();
			if (v < 0.12) {
				continue;
			}
			const auto bin = std::clamp(int(std::max(c.hsvHueF(), 0.f) * kBins), 0, kBins - 1);
			const auto w = (0.05 + s * s) * (0.3 + v);
			weight[bin] += w;
			sumR[bin] += w * c.redF();
			sumG[bin] += w * c.greenF();
			sumB[bin] += w * c.blueF();
		}
	}
	auto totalS = 0., totalV = 0., total = 0.;
	for (auto y = 0; y != small.height(); ++y) {
		for (auto x = 0; x != small.width(); ++x) {
			const auto c = QColor(small.pixel(x, y));
			totalS += c.hsvSaturationF();
			totalV += c.valueF();
			total += 1.;
		}
	}
	const auto avgS = totalS / total;
	const auto avgV = totalV / total;
	// Почти чёрно-белая обложка: нейтральный серый, без случайного оттенка.
	if (avgS < 0.16) {
		return QColor::fromHsvF(0., 0., std::clamp(avgV, 0.3, 0.5));
	}
	auto best = 0;
	for (auto i = 1; i != kBins; ++i) {
		if (weight[i] > weight[best]) {
			best = i;
		}
	}
	if (weight[best] <= 0.) {
		return QColor();
	}
	auto result = QColor::fromRgbF(
		sumR[best] / weight[best],
		sumG[best] / weight[best],
		sumB[best] / weight[best]);
	auto h = 0.f, s = 0.f, v = 0.f;
	result.getHsvF(&h, &s, &v);
	return QColor::fromHsvF(
		h < 0 ? 0.f : h,
		std::clamp(s, 0.25f, 0.8f),
		std::clamp(v, 0.4f, 0.7f));
}

constexpr auto kFullscreenSide = 56;

[[nodiscard]] QColor TintedNavTrack(const QColor &accent) {
	const auto light = IsLightMusicTheme();
	return anim::color(st::windowBgOver->c, light ? accent : accent.darker(220),
		light ? .12 : .65);
}

struct NavColors {
	QColor track;
	QColor selected;
};

[[nodiscard]] QColor MusicBackdropBase() {
	return IsLightMusicTheme() ? st::boxBg->c : QColor(0x14, 0x14, 0x15);
}

// Полноэкранный плеер: размытый цветной фон, крупный текст песни, тонкая полоса времени.
// Пункт навигации полноэкранного плеера или кнопка возврата.
class NavItem final : public Ui::AbstractButton {
public:
	NavItem(
		QWidget *parent,
		const QString &text,
		bool centered,
		Fn<NavColors()> colors = nullptr,
		Fn<QColor()> headerBackground = nullptr)
	: AbstractButton(parent)
	, _text(text)
	, _colors(std::move(colors))
	, _headerBackground(std::move(headerBackground))
	, _centered(centered) {
		setPointerCursor(true);
		setFocusPolicy(Qt::TabFocus);
		setAccessibleName(text);
		setToolTip(text);
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
	}

	void setActive(bool active) {
		if (_active != active) {
			const auto from = _fade.value(_active ? 1. : 0.);
			_active = active;
			if (isPageTab()) {
				accessibilityStateChanged({ .selected = true });
			}
			if (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations)) {
				_fade.stop();
				update();
			} else {
				_fade.start(
					[=] { update(); },
					from,
					_active ? 1. : 0.,
					kSwitchDuration,
					anim::easeOutCubic);
			}
		}
	}
	void setCentered(bool centered) {
		if (_centered != centered) {
			_centered = centered;
			update();
		}
	}

	Ui::AccessibilityState accessibilityState() const override {
		auto state = AbstractButton::accessibilityState();
		state.selectable = isPageTab();
		state.selected = isPageTab() && _active;
		return state;
	}

protected:
	void onStateChanged(State was, StateChangeSource source) override {
		AbstractButton::onStateChanged(was, source);
		update();
	}

private:
	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto font = PlayerStyle::kControlLabel.musicFont();
		p.setFont(font);
		auto inactive = isOver() ? st::windowFg->c : MusicSecondaryColor();
		auto active = st::windowFg->c;
		const auto background = _headerBackground ? _headerBackground() : QColor();
		if (background.isValid()) {
			inactive = active = PaintPlayerHeaderButton(p, rect(), background, isOver(), isDown());
		} else if (_colors) {
			const auto colors = _colors();
			if (Ui::CountContrast(colors.track, inactive) < 4.5) {
				inactive = ContrastingForeground(colors.track);
			}
			active = ContrastingForeground(colors.selected);
		}
		p.setPen(anim::color(
			inactive,
			active,
			_fade.value(_active ? 1. : 0.)));
		const auto centered = _centered || background.isValid();
		const auto inset = std::min(s(centered ? PlayerStyle::kSpace4 : PlayerStyle::kSpace16), width() / 2);
		const auto inner = rect().adjusted(inset, 0, -inset, 0);
		p.drawText(
			inner,
			(centered ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignVCenter,
			QFontMetrics(font).elidedText(_text, Qt::ElideRight, inner.width()));
		if (hasFocus()) {
			p.setPen(QPen(background.isValid() ? inactive : st::windowFg->c, s(1)));
			p.setBrush(Qt::NoBrush);
			p.drawRoundedRect(rect().adjusted(s(2), s(2), -s(2), -s(2)),
				s(18), s(18));
		}
	}

	QString _text;
	Fn<NavColors()> _colors;
	Fn<QColor()> _headerBackground;
	Ui::Animations::Simple _fade;
	bool _centered = false;
	bool _active = false;

};

// Переключатель из двух-трёх сегментов, как в окне музыки: пилюля с ползущим акцентом.
class Segmented final : public Ui::AbstractButton {
public:
	Segmented(QWidget *parent, std::vector<QString> labels, Fn<void(int)> chosen)
	: AbstractButton(parent)
	, _labels(std::move(labels)) {
		setPointerCursor(true);
		setClickedCallback([=] {
			const auto x = mapFromGlobal(QCursor::pos()).x();
			const auto index = std::clamp(
				x * int(_labels.size()) / std::max(width(), 1),
				0,
				int(_labels.size()) - 1);
			if (index != _index) {
				setIndex(index);
				chosen(index);
			}
		});
		paintRequest(
		) | rpl::on_next([=](QRect) {
			paint();
		}, lifetime());
	}

	void setIndex(int index) {
		if (_index != index) {
			_slide.start([=] { update(); }, _index, index, 220, anim::easeOutCubic);
			_index = index;
		}
	}

private:
	void paint() {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		p.setPen(Qt::NoPen);
		p.setBrush(st::windowBgOver);
		p.drawRoundedRect(rect(), height() / 2., height() / 2.);
		const auto count = int(_labels.size());
		const auto inset = s(3);
		const auto segment = (width() - 2 * inset) / double(count);
		const auto at = _slide.value(_index);
		p.setBrush(st::windowActiveTextFg);
		const auto active = QRectF(
			inset + segment * at,
			inset,
			segment,
			height() - 2 * inset);
		p.drawRoundedRect(active, active.height() / 2., active.height() / 2.);
		auto font = st::semiboldFont->f;
		font.setPixelSize(s(15));
		p.setFont(font);
		for (auto i = 0; i != count; ++i) {
			const auto mix = std::clamp(1. - std::abs(at - i), 0., 1.);
			p.setPen(anim::color(st::windowSubTextFg, QColor(Qt::white), mix));
			p.drawText(
				QRectF(inset + segment * i, 0, segment, height()),
				Qt::AlignCenter,
				_labels[i]);
		}
	}

	std::vector<QString> _labels;
	int _index = 0;
	Ui::Animations::Simple _slide;

};

class SearchSkeleton final : public Ui::RpWidget {
public:
	SearchSkeleton(
		QWidget *parent,
		not_null<Ui::ScrollArea*> scroll,
		std::optional<SearchCategory> category,
		int headerHeight = 0)
	: RpWidget(parent)
	, _category(category)
	, _collection(headerHeight > 0)
	, _animation([this] { update(); }) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		scroll->heightValue() | rpl::on_next([this, headerHeight](int height) {
			resize(width(), std::max(height - headerHeight, 0));
		}, lifetime());
		PowerSaving::Changes(
		) | rpl::on_next([this] { updateAnimation(); }, lifetime());
		connect(qApp, &QGuiApplication::applicationStateChanged, this, [this] {
			updateAnimation();
		});
	}

protected:
	void showEvent(QShowEvent *event) override {
		RpWidget::showEvent(event);
		updateAnimation();
	}

	void hideEvent(QHideEvent *event) override {
		if (_animation.animating()) {
			_animation.stop();
		}
		RpWidget::hideEvent(event);
	}

	void paintEvent(QPaintEvent *event) override {
		auto p = QPainter(this);
		const auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto catalog = _category.has_value();
		const auto rowHeight = s(kGroupedTrackHeight);
		const auto featured = _category == SearchCategory::All;
		const auto featuredHeight = featured ? s(kFeaturedArtistHeight) + s(kMusicListGap) : 0;
		const auto artSize = s(kGroupedTrackLeadWidth);
		const auto hasTrackActions = !_category
			|| featured
			|| _category == SearchCategory::Tracks;
		const auto hasMenu = catalog || !_collection;
		const auto durationRight = width() - s(hasMenu ? kGroupedTrackMenuInset : kGroupedTrackDurationInset);
		const auto actionsLeft = durationRight - s(kGroupedTrackDurationColumn) - s(kGroupedTrackActionSize);
		const auto textRight = hasTrackActions ? actionsLeft - s(8) : width() - s(14);
		const auto left = s(kGroupedTrackLeadingInset) + artSize + s(kGroupedTrackTextGap);
		const auto widths = std::array{ 184, 132, 216, 156, 248, 110, 196 };
		const auto subWidths = std::array{ 84, 100, 68, 112, 90, 72, 128 };
		constexpr auto kPulseDuration = crl::time(1700);
		const auto elapsed = _animation.animating()
			? crl::now() % kPulseDuration
			: 0;
		const auto pulse = (1. - std::cos(elapsed * 2. * M_PI / kPulseDuration)) / 2.;
		auto base = st::windowSubTextFg->c;
		base.setAlphaF(0.18 + 0.08 * pulse);
		p.setPen(Qt::NoPen);
		p.setBrush(base);
		const auto bar = [&](int x, int center, int right, int w, int h) {
			const auto width = std::clamp(right - x, 0, s(w));
			const auto height = s(h);
			p.drawRoundedRect(QRect(x, center - height / 2, width, height),
				height / 2., height / 2.);
		};
		PaintMusicCard(p, rect().adjusted(0, featuredHeight, 0, 0));
		p.setBrush(base);
		if (featured && event->rect().top() < featuredHeight) {
			const auto center = s(kFeaturedArtistHeight) / 2;
			const auto side = s(kFeaturedArtistArtSize);
			const auto card = QRect(0, 0, width(), s(kFeaturedArtistHeight));
			PaintMusicCard(p, card);
			p.setBrush(base);
			p.drawEllipse(QRect(s(kGroupedTrackLeadingInset), center - side / 2, side, side));
			const auto textLeft = s(kGroupedTrackLeadingInset + kGroupedTrackTextGap) + side;
			const auto buttonRight = width() - s(30);
			const auto buttonLeft = buttonRight - s(kFeaturedArtistButtonWidth);
			bar(textLeft, center - s(12), buttonLeft - s(14), 174, 14);
			bar(textLeft, center + s(10), buttonLeft - s(14), 88, 8);
			bar(std::max(buttonLeft, textLeft), center, buttonRight,
				kFeaturedArtistButtonWidth, 40);
		}
		const auto first = std::max(event->rect().top() - featuredHeight, 0)
			/ rowHeight;
		auto top = featuredHeight + first * rowHeight;
		for (auto i = first; top <= event->rect().bottom(); ++i, top += rowHeight) {
			const auto center = top + rowHeight / 2;
			const auto art = QRect(s(kGroupedTrackLeadingInset), center - artSize / 2, artSize, artSize);
			if (_category == SearchCategory::Artists) {
				p.drawEllipse(art);
			} else {
				p.drawRoundedRect(art, s(8), s(8));
			}
			const auto index = i % widths.size();
			bar(left, center - s(8), textRight, widths[index], 10);
			bar(left, center + s(9), textRight, subWidths[index], 8);
			if (hasTrackActions && actionsLeft >= left + s(32)) {
				bar(durationRight - s(36), center, durationRight, 36, 8);
				if (catalog || _collection) {
					bar(actionsLeft + s(kGroupedTrackActionSize / 2 - 6), center, durationRight, 12, 12);
				}
				if (hasMenu) {
					bar(width() - s(40), center, width() - s(16), 16, 6);
				}
			}
		}
	}

private:
	void updateAnimation() {
		const auto animate = isVisible()
			&& QGuiApplication::applicationState() == Qt::ApplicationActive
			&& !anim::Disabled()
			&& !PowerSaving::On(PowerSaving::kAnimations);
		if (animate && !_animation.animating()) {
			_animation.start();
		} else if (!animate && _animation.animating()) {
			_animation.stop();
		}
		update();
	}

	const std::optional<SearchCategory> _category;
	const bool _collection;
	Ui::Animations::Basic _animation;
};

class SearchIndicator final : public QWidget {
public:
	explicit SearchIndicator(QWidget *parent) : QWidget(parent) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		_timer.setInterval(40);
		connect(&_timer, &QTimer::timeout, this, [=] {
			if (PowerSaving::On(PowerSaving::kAnimations)) {
				_timer.stop();
			}
			update();
		});
		connect(qApp, &QGuiApplication::applicationStateChanged, this, [=] {
			setLoading(_loading);
		});
	}

	void setLoading(bool loading) {
		_loading = loading;
		if (_loading && isVisible()
			&& QGuiApplication::applicationState() == Qt::ApplicationActive
			&& !PowerSaving::On(PowerSaving::kAnimations)) {
			_clock.start();
			_timer.start();
		} else {
			_timer.stop();
		}
		update();
	}

protected:
	void showEvent(QShowEvent *event) override {
		QWidget::showEvent(event);
		setLoading(_loading);
	}
	void hideEvent(QHideEvent *event) override {
		_timer.stop();
		QWidget::hideEvent(event);
	}
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(QPen(st::windowSubTextFg->c, style::ConvertScale(2), Qt::SolidLine, Qt::RoundCap));
		const auto unit = width() / 20.;
		p.scale(unit, unit);
		if (_loading) {
			const auto angle = PowerSaving::On(PowerSaving::kAnimations)
				? 0 : int((_clock.elapsed() % 1000) * 360 / 1000);
			p.drawArc(QRectF(3, 3, 14, 14), -angle * 16, 260 * 16);
		} else {
			p.drawEllipse(QRectF(3, 3, 10, 10));
			p.drawLine(QPointF(12, 12), QPointF(17, 17));
		}
	}

private:
	QTimer _timer;
	QElapsedTimer _clock;
	bool _loading = false;
};

constexpr auto kBarHeight = 108;
constexpr auto kBarMaxWidth = 1040;
constexpr auto kBarCover = 72;
constexpr auto kBarRadius = 40;
constexpr auto kHomeTrendingSection = 3;
constexpr auto kSeekFrameInterval = 33;
constexpr auto kBackdropFrameInterval = 100;
constexpr auto kHomeSideGlowMin = 96;

[[nodiscard]] int HomeSideGlowWidth(int width) {
	return std::max((width - style::ConvertScale(kMusicListMaxWidth)) / 2, 0);
}

void PaintHomeSideGlow(
		QPainter &p,
		QRect band,
		QColor tint,
		bool light,
		double pulse) {
	const auto side = HomeSideGlowWidth(band.width());
	const auto minimum = style::ConvertScale(kHomeSideGlowMin);
	if (side <= minimum || band.isEmpty() || !tint.alpha()) {
		return;
	}
	const auto strength = std::min(
		(side - minimum) / double(style::ConvertScale(64)), 1.);
	const auto opacity = light ? (.16 + .08 * pulse) : (.30 + .14 * pulse);
	tint.setAlphaF(tint.alphaF() * strength * opacity);
	auto glow = QRadialGradient(QPointF(.5, .5), .5);
	glow.setCoordinateMode(QGradient::ObjectMode);
	glow.setColorAt(0., tint);
	tint.setAlpha(0);
	glow.setColorAt(1., tint);
	const auto height = band.height() * .8;
	p.fillRect(QRectF(band.x() - side, band.y(), 2. * side, height), glow);
	p.fillRect(QRectF(band.x() + band.width() - side, band.y(), 2. * side, height), glow);
}

struct PlayerSpringSpec {
	double damping;
	double stiffness;
};

// Material 3 Standard Motion, unit mass. Effects are critically damped.
constexpr auto kSpatial = PlayerSpringSpec{ 0.9, 700. };
constexpr auto kFastSpatial = PlayerSpringSpec{ 0.9, 1400. };
constexpr auto kEffects = PlayerSpringSpec{ 1., 1600. };
constexpr auto kFastEffects = PlayerSpringSpec{ 1., 3800. };
constexpr auto kSlowEffects = PlayerSpringSpec{ 1., 800. };
// Canvas follows video scene changes slowly enough to avoid flashing.
constexpr auto kAmbientEffects = PlayerSpringSpec{ 1., 16. };

[[nodiscard]] bool PlayerMotionEnabled() {
	return !anim::Disabled() && !PowerSaving::On(PowerSaving::kAnimations);
}

class PlayerSpring final {
public:
	template <typename Callback>
	explicit PlayerSpring(Callback callback)
	: _animation([this, callback = std::move(callback)](crl::time now) {
		const auto enabled = PlayerMotionEnabled();
		if (enabled && now > _last) {
			step((now - _last) / (1000. * anim::SlowMultiplier()));
			_last = now;
		}
		const auto keep = enabled
			&& (std::abs(_value - _target) >= 0.001 || std::abs(_velocity) >= 0.02);
		if (!keep) {
			stop();
		}
		callback();
		return keep;
	}) {
	}

	void start(double from, double target, PlayerSpringSpec spec) {
		if (!PlayerMotionEnabled()) {
			stop();
			return;
		}
		const auto running = animating();
		if (running) {
			if (_target == target) {
				return;
			}
			// Retarget without discarding the current position or velocity.
		} else {
			_value = from;
			_velocity = 0.;
		}
		_target = target;
		if (_value == _target && _velocity == 0.) {
			stop();
			return;
		}
		const auto frequency = std::sqrt(spec.stiffness);
		_decay = spec.damping * frequency;
		_oscillation = std::sqrt(1. - spec.damping * spec.damping) * frequency;
		_last = crl::now();
		if (!running) {
			_animation.start();
		}
	}

	void stop() {
		if (animating()) {
			_animation.stop();
		}
	}

	[[nodiscard]] bool animating() const { return _animation.animating(); }
	[[nodiscard]] double value(double target) const {
		return animating() ? _value : target;
	}
	[[nodiscard]] double unitValue(double target) const {
		return std::clamp(value(target), 0., 1.);
	}
	[[nodiscard]] double velocity() const {
		return animating() ? _velocity : 0.;
	}

private:
	void step(double dt) {
		// Exact solution for critical and under damping, even on a dropped frame.
		const auto x = _value - _target;
		const auto y = _velocity + _decay * x;
		const auto angle = _oscillation * dt;
		const auto c = _oscillation ? std::cos(angle) : 1.;
		const auto s = _oscillation ? std::sin(angle) / _oscillation : dt;
		const auto envelope = std::exp(-_decay * dt);
		const auto next = (x * c + y * s) * envelope;
		const auto crossed = !_oscillation && (x * next < 0.);
		_value = crossed ? _target : _target + next;
		_velocity = crossed
			? 0.
			: (y * c - x * _oscillation * _oscillation * s) * envelope - _decay * next;
	}

	double _value = 0.;
	double _velocity = 0.;
	double _target = 0.;
	double _decay = 0.;
	double _oscillation = 0.;
	crl::time _last = 0;
	Ui::Animations::Basic _animation;
};

[[nodiscard]] QRectF CoverSourceRect(QSizeF source, QSizeF target) {
	const auto scale = std::max(target.width() / source.width(), target.height() / source.height());
	const auto crop = target / scale;
	return QRectF(QPointF(source.width() - crop.width(), source.height() - crop.height()) / 2., crop);
}

class CoverTransition final {
public:
	void prepare(
			QRectF compact,
			double compactRadius,
			QRectF expanded,
			double expandedRadius) {
		_fromRect = compact;
		_toRect = expanded;
		_fromPath = preparePath(compact, compactRadius, 0.6);
		_toPath = preparePath(expanded, expandedRadius, 0.);
		Expects(_fromPath.elementCount() == _toPath.elementCount());
		_path = _fromPath;
	}
	void update(double progress) {
		_rect = QRectF(
			_fromRect.topLeft() + (_toRect.topLeft() - _fromRect.topLeft()) * progress,
			_fromRect.size() + (_toRect.size() - _fromRect.size()) * progress);
		_expanded = progress;
		// Keep the same path storage after the first update of a transition.
		for (auto i = 0; i != _path.elementCount(); ++i) {
			const auto from = _fromPath.elementAt(i);
			const auto to = _toPath.elementAt(i);
			_path.setElementPositionAt(
				i, progress == 1. ? to.x : from.x + (to.x - from.x) * progress,
				progress == 1. ? to.y : from.y + (to.y - from.y) * progress);
		}
		if (progress == 1.) {
			_rect = _toRect;
		}
	}
	[[nodiscard]] const QRectF &rect() const { return _rect; }
	[[nodiscard]] const QPainterPath &path() const { return _path; }
	[[nodiscard]] double expanded() const { return _expanded; }

private:
	static QPainterPath preparePath(QRectF rect, double radius, double smoothing) {
		const auto source = Ui::SquirclePath(rect, radius, smoothing);
		if (source.elementCount() == 41) return source;
		// Qt removes zero-length curves and sides. Restore three cubics per
		// corner, including the degenerate cubics beside a circular arc.
		auto result = Ui::SquirclePath(QRectF(0., 0., 1., 1.), 0.1);
		auto curves = 0;
		for (auto i = 0; i != source.elementCount(); ++i) {
			if (source.elementAt(i).isCurveTo()) ++curves;
		}
		Expects(curves == 0 || curves == 4 || curves == 12);
		const auto point = [&](int i) {
			const auto element = source.elementAt(i);
			return QPointF(element.x, element.y);
		};
		const auto put = [&](int i, QPointF point) {
			result.setElementPositionAt(i, point.x(), point.y());
		};
		auto at = 1;
		auto current = point(0);
		for (auto corner = 0; corner != 4; ++corner) {
			if (corner && source.elementAt(at).isLineTo()) current = point(at++);
			put(corner * 10, current);
			if (!curves) {
				for (auto i = 1; i != 10; ++i) put(corner * 10 + i, current);
			} else if (curves == 4) {
				for (auto i = 1; i != 4; ++i) put(corner * 10 + i, current);
				for (auto i = 4; i != 7; ++i) put(corner * 10 + i, point(at++));
				current = point(at - 1);
				for (auto i = 7; i != 10; ++i) put(corner * 10 + i, current);
			} else {
				for (auto i = 1; i != 10; ++i) put(corner * 10 + i, point(at++));
				current = point(at - 1);
			}
		}
		put(40, point(0));
		return result;
	}

	QRectF _rect;
	QRectF _fromRect;
	QRectF _toRect;
	QPainterPath _path;
	QPainterPath _fromPath;
	QPainterPath _toPath;
	double _expanded = 0.;
};

class FullscreenPlayer final : public Ui::RpWidget {
public:
	FullscreenPlayer(
		not_null<Ui::RpWidget*> parent,
		not_null<Window::SessionController*> controller,
		bool openHome)
	: RpWidget(parent.get())
	, _controller(controller)
	, _view(Ui::CreateChild<LyricsView>(this))
	, _tabScroll(Ui::CreateChild<Ui::ScrollArea>(this, st::defaultScrollArea))
	, _queuePanel(Ui::CreateChild<Ui::RpWidget>(this))
	, _queueScroll(Ui::CreateChild<Ui::ScrollArea>(_queuePanel, st::defaultScrollArea))
	, _playbackBar(Ui::CreateChild<Ui::RpWidget>(this))
	, _seek(Ui::CreateChild<SeekBar>(_playbackBar, &_playbackColors))
	, _heart(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Add, 44))
	, _collapse(Ui::CreateChild<RoundGlyph>(
			this, Glyph::Collapse, 44, [=] { return headerControlBackground(); }))
	, _filter(Ui::CreateChild<RoundGlyph>(
			this, Glyph::Filter, 44, [=] { return headerControlBackground(); }))
	, _queueToggle(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Queue, 44))
	, _volume(Ui::CreateChild<VolumeControl>(_playbackBar, _playbackColors))
	, _tabTransition(Ui::CreateChild<Ui::RpWidget>(this))
	, _coverOverlay(Ui::CreateChild<Ui::RpWidget>(this))
	, _tab(openHome ? Tab::Home : Tab::Now)
	, _shuffle(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Shuffle, 44))
	, _previous(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Previous, 52))
	, _play(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Play, 52))
	, _next(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Next, 52))
	, _repeat(Ui::CreateChild<RoundGlyph>(_playbackBar, Glyph::Repeat, 44))
	, _canvas(this, [=] {
		update(computeLayout().cover);
	})
	, _upcomingCanvas(this, [] {}, true) {
		setFocusPolicy(Qt::TabFocus);
		setMouseTracking(true);
		_view->setBig(true);
		_view->setBackground([=](QPainter &p, QRect area) {
			paintBackgroundPart(p, area, _view->pos());
		});
		_view->setAttribute(Qt::WA_OpaquePaintEvent);
		_seek->setThin(true);
		_seek->enableTimePreview();
		_seek->setExpressive(true, false);
		_homeBack = Ui::CreateChild<NavItem>(
			this,
			u"‹  "_q + QString::fromUtf8(Mokagram::Lang::Tr("Назад")),
			false,
			nullptr,
			[=] { return headerControlBackground(); });
		_homeBack->setClickedCallback(crl::guard(this, [=] {
			if (_homeBackToSearch && !_homeReturnToArtist) {
				setTab(Tab::Search);
				_tabScroll->scrollToY(_searchReturnScroll);
				return;
			}
			if (_homeReturnToArtist) {
				openHomeArtist(_homeArtistProfile, _homeArtistSeed);
				return;
			}
			rememberNavigation();
			animateHomeBackExit();
			clearHomeSideGlow();
			_homeAlbumOpen = false;
			_homeArtistOpen = false;
			++_homeAlbumGeneration;
			relayout();
			rebuildTab();
			_tabScroll->scrollToY(_homeAlbumReturnScroll);
		}));
		_homeBack->hide();
		const auto viewport = _tabScroll->viewport();
		_homeScrollFade = new QGraphicsOpacityEffect(viewport);
		_homeScrollFade->setOpacity(1.);
		_homeScrollFade->setEnabled(false);
		viewport->setGraphicsEffect(_homeScrollFade);
		_homeArtistBar = Ui::CreateChild<Ui::RpWidget>(this);
		_homeArtistBar->setAttribute(Qt::WA_TransparentForMouseEvents);
		_homeArtistBarOpacity = new QGraphicsOpacityEffect(_homeArtistBar);
		_homeArtistBarOpacity->setOpacity(0.);
		_homeArtistBar->setGraphicsEffect(_homeArtistBarOpacity);
		_homeArtistBar->paintOn([=](QPainter &p) {
			const auto s = [](int value) { return style::ConvertScale(value); };
			auto hq = PainterHighQualityEnabler(p);
			const auto side = s(BoxParts::kOuterSide);
			const auto inner = _homeArtistBar->rect().adjusted(side, 0, -side, -s(kMusicListGap));
			p.setPen(Qt::NoPen);
			p.setBrush(st::boxBg);
			p.drawRoundedRect(inner, s(BoxParts::kCardRadius), s(BoxParts::kCardRadius));
			PaintMusicCard(p, inner);
			const auto text = inner.adjusted(s(16), 0, -s(16), 0);
			p.setFont(st::semiboldFont);
			p.setPen(st::windowFg);
			p.drawText(text, Qt::AlignVCenter | Qt::AlignLeft,
				st::semiboldFont->elided(_homeArtistProfile.name, text.width()));
		});
		_homeArtistBar->hide();
		AttachCommentsToPlayer(controller, _playbackBar, _seek, false);
		_filter->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Звуковой фильтр")));
		_filter->setClickedCallback([=] {
			_filterMenu = base::make_unique_q<Ui::PopupMenu>(this, st::popupMenuWithIcons);
			for (const auto &entry : Mokagram::Settings::Entries()) {
				if (entry.key != u"player.audio_filter"_q) {
					continue;
				}
				const auto current = Mokagram::Settings::Int(entry.key);
				const auto icons = std::array{
					&st::menuIconSoundOn,
					&st::menuIconBoosts,
					&st::menuIconStartStream,
					&st::menuIconSettings,
					&st::menuIconSoundSelect,
					&st::menuIconPalette,
					&st::menuIconPhone,
				};
				for (auto i = 0; i != entry.choices.size(); ++i) {
					const auto label = entry.choices[i]
						+ (i == current ? u"  ✓"_q : QString());
					_filterMenu->addAction(label, [=] {
						Mokagram::Settings::SetInt(u"player.audio_filter"_q, i);
					}, (i < icons.size()) ? icons[i] : nullptr);
				}
				break;
			}
			_filterMenu->popup(_filter->mapToGlobal(QPoint(0, _filter->height())));
		});
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto queueTitle = QString::fromUtf8(Mokagram::Lang::Tr("Очередь"));
		_queueToggle->setToolTip(queueTitle);
		_queueToggle->setAccessibleName(queueTitle);
		_queueToggle->setFocusPolicy(Qt::TabFocus);
		_queueToggle->setClickedCallback([=] {
			setQueuePanelOpen(!_queuePanelOpen);
		});
		_queuePanel->hide();
		_queueTabBar = Ui::CreateChild<Ui::RpWidget>(_queuePanel);
		_queueTabs[0] = Ui::CreateChild<NavItem>(_queueTabBar, queueTitle, true);
		_queueTabs[1] = Ui::CreateChild<NavItem>(
			_queueTabBar, PlaylistText("Недавно"), true);
		_queueTabBar->paintOn([=](QPainter &p) {
			const auto hq = PainterHighQualityEnabler(p);
			const auto area = _queueTabBar->rect();
			const auto inset = s(PlayerStyle::kSpace4);
			const auto segment = std::max(area.width() - 2 * inset, 0) / 2.;
			const auto at = _queueTabSlide.unitValue(_queueHistory ? 1. : 0.);
			p.setPen(Qt::NoPen);
			p.setBrush(st::boxBg);
			p.drawRoundedRect(area, area.height() / 2., area.height() / 2.);
			const auto active = QRectF(inset + segment * at, inset,
				segment, area.height() - 2 * inset);
			p.setBrush(anim::color(st::windowBgOver->c, st::windowFg->c, .08));
			p.drawRoundedRect(active, active.height() / 2., active.height() / 2.);
		});
		for (auto i = 0; i != int(_queueTabs.size()); ++i) {
			_queueTabs[i]->setIsPageTab(true);
			_queueTabs[i]->setActive(i == 0);
			_queueTabs[i]->setClickedCallback([=] { setQueueHistory(i == 1); });
		}
		_queueContent = _queueScroll->setOwnedWidget(
			object_ptr<Ui::VerticalLayout>(_queueScroll));
		const auto queueViewport = _queueScroll->viewport();
		_queueEdgeFade = new QGraphicsOpacityEffect(queueViewport);
		_queueEdgeFade->setOpacity(1.);
		_queueEdgeFade->setEnabled(false);
		queueViewport->setGraphicsEffect(_queueEdgeFade);
		_queueScroll->scrollTopChanges() | rpl::on_next([=] {
			updateQueueEdgeFade(_queueScroll, _queueEdgeFade);
		}, lifetime());
		_queueSlide.setEasingCurve(QEasingCurve::InOutCubic);
		connect(&_queueSlide, &QVariantAnimation::valueChanged, this, [=](const QVariant &value) {
			_queueProgress = value.toDouble();
			relayout();
		});
		const auto addLabel = [&](QString text, const style::FlatLabel &style, QMargins padding) {
			return _queueContent->add(object_ptr<Ui::SlideWrap<Ui::FlatLabel>>(
				_queueContent, object_ptr<Ui::FlatLabel>(_queueContent, std::move(text), style),
				padding));
		};
		static const auto sectionStyle = [] {
			auto result = st::defaultSubsectionTitle;
			result.textFg = st::windowFg;
			result.minWidth = 0;
			return result;
		}();
		_queueNowLabel = addLabel(PlaylistText("Сейчас играет"), sectionStyle,
			QMargins(s(PlayerStyle::kSpace8), s(PlayerStyle::kSpace2),
				s(PlayerStyle::kSpace8), s(PlayerStyle::kSpace2)));
		_queueNow = _queueContent->add(object_ptr<CompactQueueList>(_queueContent));
		_queueNow->paintOn([=](QPainter &p) {
			if (!_queueAdvanceExit.isNull()) {
				const auto progress = _queueAdvance.unitValue(1.);
				p.setOpacity(1. - progress);
				p.drawPixmap(0, -qRound(_queueNow->height() * progress), _queueAdvanceExit);
			}
		});
		_queueNextLabel = addLabel(PlaylistText("Далее в очереди"), sectionStyle,
			QMargins(s(PlayerStyle::kSpace8), s(PlayerStyle::kSpace12),
				s(PlayerStyle::kSpace8), s(PlayerStyle::kSpace2)));
		_queueListSlot = _queueContent->count();
		_queueList = _queueContent->add(object_ptr<CompactQueueList>(_queueContent));
		const auto queueRowPad = s(PlayerStyle::kSpace8);
		_queueEmpty = addLabel({}, st::boxDividerLabel, QMargins(queueRowPad, queueRowPad, queueRowPad, 0));
		_queueReorder = std::make_unique<Ui::VerticalLayoutReorder>(_queueList, _queueScroll);
		_queueReorder->updates() | rpl::on_next([=](Ui::VerticalLayoutReorder::Single change) {
			using State = Ui::VerticalLayoutReorder::State;
			const auto row = QPointer(dynamic_cast<Row*>(change.widget.get()));
			if (!row) {
				return;
			}
			if (change.state == State::Started) {
				stopQueueAdvance();
				row->setQueueDragging(true);
				return;
			}
			const auto revision = _queueLayoutRevision;
			const auto first = _queueUpcomingFirst;
			Ui::PostponeCall(this, [=] {
				if (row) {
					row->setQueueDragging(false);
				}
				if (change.state == State::Applied && _queueLayoutRevision == revision) {
					QueueMove(first + change.oldPosition, change.newPosition - change.oldPosition);
				}
			});
		}, lifetime());
		const auto queueSearchLabel = QString::fromUtf8(Mokagram::Lang::Tr("Поиск в очереди"));
		static const auto queueSearchStyle = [] {
			auto result = st::defaultInputField;
			const auto s = [](int value) { return style::ConvertScale(value); };
			const auto height = s(44);
			const auto line = std::max(result.style.lineHeight, result.style.font->height);
			const auto top = std::max((height - line) / 2, 0);
			result.textBg = result.textBgActive = st::boxBg;
			const auto side = s(PlayerStyle::kSpace16);
			result.textMargins = QMargins(side, top, side, std::max(height - line - top, 0));
			result.placeholderFg = result.placeholderFgActive = st::windowSubTextFg;
			result.placeholderScale = 0.;
			result.placeholderShift = 0;
			result.placeholderFont = result.style.font;
			result.borderRadius = s(14);
			result.heightMin = height;
			return result;
		}();
		_queueSearch = Ui::CreateChild<Ui::InputField>(
			_queuePanel, queueSearchStyle, rpl::single(queueSearchLabel));
		_queueSearch->setAccessibleName(queueSearchLabel);
		_queueSearch->changes() | rpl::on_next([=] {
			_queueScroll->scrollToY(0);
			refreshQueuePanel();
		}, lifetime());
		_queuePanel->paintOn([=](QPainter &p) {
			PaintMusicCard(p, _queuePanel->rect());
		});
		_shuffle->resize(s(36), s(36));
		_previous->resize(s(40), s(40));
		_play->resize(s(44), s(44));
		_next->resize(s(40), s(40));
		_repeat->resize(s(36), s(36));
		_seek->resize(_seek->width(), s(20));
		_tabContent = _tabScroll->setOwnedWidget(
			object_ptr<Ui::PaddingWrap<Ui::VerticalLayout>>(
				_tabScroll,
				object_ptr<Ui::VerticalLayout>(_tabScroll),
				QMargins()));
		_tabList = _tabContent->entity();
		_playbackBar->setMouseTracking(true);
		_playbackBar->paintOn([=](QPainter &p) {
			if (_tab == Tab::Home) {
				const auto hq = PainterHighQualityEnabler(p);
				p.translate(-_playbackBar->pos());
				const auto layout = computeLayout();
				paintPlaybackBarBackground(p, layout);
				paintPlaybackBarDetails(p, layout);
			}
		});
		style::PaletteChanged() | rpl::on_next([=] {
			updatePlaybackBarColors(true);
			repaintBackdrop();
			if (_tab == Tab::Search) relayout();
		}, lifetime());
		_coverOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
		_coverOverlay->hide();
		_coverOverlay->paintOn([=](QPainter &p) {
			const auto hq = PainterHighQualityEnabler(p);
			paintFullscreenCover(p, computeLayout());
		});
		_tabTransition->setAttribute(Qt::WA_TransparentForMouseEvents);
		_tabTransition->hide();
		_tabTransition->paintRequest(
		) | rpl::on_next([=] {
			if (_tabTransitionNow.isNull()) {
				return;
			}
			auto p = QPainter(_tabTransition);
			const auto progress = _tabTransitionSlide.unitValue(1.);
			const auto opacity = _tabTransitionFade.unitValue(1.);
			const auto shift = style::ConvertScale(28) * _tabTransitionDirection;
			p.drawPixmap(0, 0, _tabTransitionNow);
			p.setOpacity(1. - opacity);
			p.drawPixmap(
				int(base::SafeRound(-shift * progress)),
				0,
				_tabTransitionWas);
			p.setOpacity(opacity);
			p.drawPixmap(
				int(base::SafeRound(shift * (1. - progress))),
				0,
				_tabTransitionNow);
		}, lifetime());
		_homeMoreRetryTimer.setSingleShot(true);
		connect(&_homeMoreRetryTimer, &QTimer::timeout, this, [=] {
			scheduleHomeMoreIfNeeded();
		});
		_tabScroll->scrollTopChanges() | rpl::on_next([=](int top) {
			if (_tab == Tab::Home) {
				updateHomeArtistBar();
				if (_homeAlbumOpen) update();
			} else if (showsQueue()) {
				updateQueueEdgeFade(_tabScroll, _homeScrollFade);
			}
			if (!_rebuildingTab) {
				scheduleHomeMoreIfNeeded();
			}
		}, lifetime());
		const auto names = std::array{
			"Сейчас играет", "Главная", "Поиск", "Библиотека", "Плейлисты" };
		for (auto i = 0; i != int(names.size()); ++i) {
			const auto item = Ui::CreateChild<NavItem>(
				this,
				QString::fromUtf8(Mokagram::Lang::Tr(names[i])),
				false,
				[=] { return navColors(); });
			item->setClickedCallback([=] { setTab(Tab(i)); });
			_nav[i] = item;
		}
		_nav[int(_tab)]->setActive(true);
		Account::Changes() | rpl::on_next([=] {
			if (_tab == Tab::Library) {
				syncLibrary();
			}
			++_homeSoundCloudGeneration;
			_homeSoundCloudLoading = false;
			_homeSoundCloudLoaded = false;
			_homeSoundCloudError.clear();
			_homeTrending.clear();
			_homeWeeklyWave = {};
			_homeDailyDrops = {};
			if (_tab == Tab::Home && !_homeAlbumOpen) {
				_homeSection = 0;
				rebuildTab();
			}
		}, lifetime());
		Account::LikeWriteErrors() | rpl::on_next([=](QString error) {
			_controller->showToast(error);
		}, lifetime());
		_searchField = Ui::CreateChild<QLineEdit>(this);
		_searchField->setFrame(false);
		_searchField->setAccessibleName(QString::fromUtf8(Mokagram::Lang::Tr("Поиск музыки")));
		_searchIndicator = new SearchIndicator(_searchField);
		_searchRetry = new QPushButton(QString::fromUtf8(Mokagram::Lang::Tr("Повторить")), this);
		_searchRetry->setCursor(Qt::PointingHandCursor);
		_searchRetry->setStyleSheet(u"QPushButton { background: %1; color: %2; border: 0; border-radius: 8px; padding: 0 12px; } QPushButton:hover, QPushButton:focus { background: %3; }"_q
			.arg(st::windowBgOver->c.name(), st::windowFg->c.name(), st::windowActiveTextFg->c.name()));
		_searchRetry->hide();
		connect(_searchRetry, &QPushButton::clicked, this, [=] { runSearch(); });
		_searchField->setPlaceholderText(
			QString::fromUtf8(Mokagram::Lang::Tr("Поиск на SoundCloud")));
		_searchField->hide();
		_searchTimer.setSingleShot(true);
		_searchTimer.setInterval(kTypingDelay);
		connect(&_searchTimer, &QTimer::timeout, this, [=] { runSearch(); });
		connect(_searchField, &QLineEdit::textChanged, this, [=] {
			++_searchGen;
			_searchIndicator->setLoading(false);
			_searchPending = !_searchField->text().trimmed().isEmpty();
			_searchFailed = false;
			_searchTracks.clear();
			_catalogItems.clear();
			resetCatalogFilters();
			_searchStatus = _searchField->text().trimmed().isEmpty()
				? QString() : QString::fromUtf8(Mokagram::Lang::Tr("Ищу…"));
			if (_tab == Tab::Search) {
				rebuildTab();
				_tabScroll->scrollToY(0);
			}
			_searchTimer.start();
		});
		_chooseSearchChat = Ui::CreateChild<Ui::RoundButton>(this,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Выбрать чат"))),
			st::defaultLightButton);
		_chooseSearchChat->hide();
		_chooseSearchChat->setClickedCallback([=] {
			ShowMusicSourceBox(
				_controller,
				crl::guard(this, [=](not_null<Data::Thread*> thread) {
					_searchChat = thread->owningHistory()->peer->id;
					_chooseSearchChat->setText(rpl::single(
						QString::fromUtf8(Mokagram::Lang::Tr("Чат: %1"))
						.arg(thread->owningHistory()->peer->name())));
					runSearch();
					return true;
				}));
		});
		_sourceSwitch = Ui::CreateChild<Segmented>(
			this,
			std::vector<QString>{
				u"SoundCloud"_q,
				QString::fromUtf8(Mokagram::Lang::Tr("Из чатов")) },
			[=](int index) {
				_searchChats = (index == 1);
				resetCatalogFilters();
				_searchField->setPlaceholderText(QString::fromUtf8(Mokagram::Lang::Tr(
					_searchChats ? "Поиск по чатам" : "Поиск на SoundCloud")));
				relayout();
				runSearch();
			});
		const auto categories = std::array{
			std::pair{ SearchCategory::All, "Всё" },
			std::pair{ SearchCategory::Tracks, "Треки" },
			std::pair{ SearchCategory::Playlists, "Плейлисты" },
			std::pair{ SearchCategory::Albums, "Альбомы" },
			std::pair{ SearchCategory::Artists, "Исполнители" },
		};
		for (const auto &[category, text] : categories) {
			const auto chip = new QPushButton(PlaylistText(text), this);
			chip->setCursor(Qt::PointingHandCursor);
			chip->setCheckable(true);
			chip->setAutoExclusive(true);
			chip->setChecked(category == _searchCategory);
			chip->hide();
			const auto filter = category;
			connect(chip, &QPushButton::clicked, this, [=] {
				if (_searchCategory == filter) {
					return;
				}
				_searchCategory = filter;
				runSearch();
			});
			_searchFilters[int(category)] = chip;
		}
		_sourceSwitch->hide();
		QueueState().changes.events(
		) | rpl::on_next([=] {
			refreshQueueViews();
		}, lifetime());
		for (const auto button : { _collapse, _filter }) {
			button->setOverlay(true);
		}
		for (const auto button : { _heart, _queueToggle, _shuffle, _previous, _play, _next, _repeat }) {
			button->setPlaybackColors(_playbackColors);
		}
		_heart->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Нравится")));
		_collapse->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Свернуть")));
		_shuffle->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Перемешать")));
		_repeat->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Повтор")));

		const auto instance = Media::Player::instance();
		_collapse->setClickedCallback([=] { close(); });
		_heart->setClickedCallback([=] {
			ToggleTrackLike(PlayingTrack(), _controller, this);
		});
		const auto step = [=](int delta) {
			if (OwnsTrack(instance->current(AudioMsgId::Type::Song))) {
				Step(delta);
			} else if (delta > 0) {
				instance->next(AudioMsgId::Type::Song);
			} else {
				instance->previous(AudioMsgId::Type::Song);
			}
		};
		_previous->setClickedCallback([=] { step(-1); });
		_next->setClickedCallback([=] { step(1); });
		_play->setClickedCallback([=] {
			instance->playPause(AudioMsgId::Type::Song);
		});
		_shuffle->setClickedCallback([=] {
			auto &settings = Core::App().settings();
			settings.setPlayerOrderMode(
				settings.playerOrderMode() == Media::OrderMode::Shuffle
					? Media::OrderMode::Default
					: Media::OrderMode::Shuffle);
			Core::App().saveSettingsDelayed();
		});
		_repeat->setClickedCallback([=] {
			auto &settings = Core::App().settings();
			const auto mode = settings.playerRepeatMode();
			settings.setPlayerRepeatMode(mode == Media::RepeatMode::None
				? Media::RepeatMode::All
				: mode == Media::RepeatMode::All
				? Media::RepeatMode::One
				: Media::RepeatMode::None);
			Core::App().saveSettingsDelayed();
		});
		Core::App().settings().playerOrderModeValue(
		) | rpl::on_next([=] { _shuffle->update(); }, lifetime());
		Core::App().settings().playerRepeatModeValue(
		) | rpl::on_next([=] { _repeat->update(); }, lifetime());
		const auto seek = [=](float64 progress) {
			_view->followPlayback();
			instance->startSeeking(AudioMsgId::Type::Song);
			instance->finishSeeking(AudioMsgId::Type::Song, std::clamp(progress, 0., 1.));
		};
		_seek->sought() | rpl::on_next(seek, lifetime());
		_view->lineClicked() | rpl::on_next([=](qint64 ms) {
			const auto state = instance->getState(AudioMsgId::Type::Song);
			const auto lengthMs = state.length * 1000
				/ std::max<int64>(state.frequency, 1);
			if (lengthMs > 0) {
				seek(float64(ms) / float64(lengthMs));
			}
		}, lifetime());

		parent->events(
		) | rpl::filter([](not_null<QEvent*> e) {
			return e->type() == QEvent::Resize;
		}) | rpl::on_next([=] {
			setGeometry(parentWidget()->rect());
		}, lifetime());
		instance->updatedNotifier(
		) | rpl::on_next([=] { refresh(); }, lifetime());
		instance->tracksFinished(
		) | rpl::filter([=](AudioMsgId::Type type) {
			return type == AudioMsgId::Type::Song && !instance->current(type).audio();
		}) | rpl::on_next([=] { refresh(); }, lifetime());
		QueueState().changes.events(
		) | rpl::on_next([=] { refresh(); }, lifetime());
		CoversChanged() | rpl::on_next([=] { refresh(); }, lifetime());
		RecentChanged() | rpl::on_next([=] {
			if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen) {
				scheduleTabRebuild();
			}
			if (_queuePanelOpen && _queueHistory) {
				scheduleQueuePanelRefresh();
			}
		}, lifetime());
		PlaylistsChanged() | rpl::on_next([=] {
			if (_tab == Tab::Playlists || (_tab == Tab::Home && _homeArtistOpen)) {
				scheduleTabRebuild();
			}
		}, lifetime());
		LikesChanged() | rpl::on_next([=] {
			refresh();
			if (_tab == Tab::Library || _tab == Tab::Home) {
				scheduleTabRebuild();
			}
		}, lifetime());
		MotionChanges() | rpl::on_next([=] {
			_motionFor.clear();
			_motionCover = nullptr;
			_spotifyCanvas = nullptr;
			refresh();
		}, lifetime());
		LyricsAvailabilityChanges() | rpl::on_next([=] { relayout(); }, lifetime());
		Mokagram::Settings::Changed() | rpl::on_next([=](const QString &key) {
			if (key == u"lyrics.comments"_q) relayout();
			if (key == u"player.hide_youtube_fallback"_q) {
				if (_homeArtistOpen) {
					if (_tab == Tab::Home) {
						rebuildTab();
					}
					if (_homeArtistYouTubeDeferred && !Settings::Bool(key)) {
						loadHomeArtistYouTube(_homeAlbumGeneration);
					}
				}
				return;
			}
			if (key == u"player.hide_trending"_q) {
				const auto closeTrending = (_homeSection == kHomeTrendingSection)
					&& Mokagram::Settings::Bool(key);
				if (closeTrending) {
					_homeSection = 0;
					_homeAlbumReturnScroll = _homeFeedScroll;
				}
				if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen) {
					rebuildTab();
					if (closeTrending) {
						_tabScroll->scrollToY(_homeFeedScroll);
					}
				}
				return;
			}
			if (key == u"lyrics.provider"_q) {
				refresh();
			} else if (key.startsWith(u"player."_q)) {
				if (key == u"player.motion_covers"_q || key == u"player.spotify_canvas"_q) {
					_motionFor.clear();
					_motionCover = nullptr;
					_spotifyCanvas = nullptr;
				}
				if ((key == u"player.cover_colors"_q || key == u"player.dynamic_background"_q)
					&& !Mokagram::Settings::Bool(key)) {
					_homeSideGlowFade.stop();
				}
				refresh();
				updateAmbientTimer();
				repaintBackdrop();
			}
		}, lifetime());
		connect(&_ambientTimer, &QTimer::timeout, this, [=] {
			const auto now = crl::now();
			const auto elapsed = std::min((now - _ambientAt) / 1000., 0.1);
			_ambientAt = now;
			const auto backdrop = animatesBackdrop();
			if (backdrop || _homeSideGlowPulsing) {
				const auto previous = int(_backgroundPulse * 100.);
				updateBackdropEnergy(elapsed);
				if (_homeSideGlowPulsing && int(_backgroundPulse * 100.) != previous) {
					repaintHomeSideGlow();
				}
			}
			if (backdrop) {
				repaintAnimatedBackdrop(now);
			}
			if (_seek->waveAnimating()) {
				_seek->advanceWave(elapsed);
			}
		});
		_coverTitleTimer.setTimerType(Qt::PreciseTimer);
		// Request frames more often than a 60 Hz display presents them.
		_coverTitleTimer.setInterval(8);
		connect(&_coverTitleTimer, &QTimer::timeout, this, [=] {
			advanceTitleClock();
			const auto layout = computeLayout();
			if (_coverTitleMoves && _hover != InfoHover::Title) {
				update(metadataLines(layout.info, false).title);
			}
			if (_barTitleMoves && _hover != InfoHover::BarTitle) {
				const auto title = metadataLines(barInfoRect(layout), true).title;
				if (_tab == Tab::Home) {
					_playbackBar->update(title.translated(-_playbackBar->pos()));
				} else {
					update(title);
				}
			}
		});
		_seek->waveAnimationChanges(
		) | rpl::on_next([=] { updateAmbientTimer(); }, lifetime());
		connect(qApp, &QGuiApplication::applicationStateChanged, this, [=] {
			updateAmbientTimer();
		});

		PowerSaving::Changes(
		) | rpl::on_next([=] {
			if (!PlayerMotionEnabled()) {
				_homeSideGlowFade.stop();
			}
			updateAmbientTimer();
			repaintBackdrop();
		}, lifetime());

		qApp->installEventFilter(this);
		setGeometry(parent->rect());
		refresh();
		rebuildTab();
		show();
		raise();
		setFocus();

		if (PlayerMotionEnabled()) {
			const auto effect = new QGraphicsOpacityEffect(this);
			effect->setOpacity(0.);
			setGraphicsEffect(effect);
			_fade.start(0., 1., kEffects);
		}
	}

	[[nodiscard]] bool canReuse(
			not_null<Window::SessionController*> controller) const {
		return !_closing && _controller == controller;
	}

	[[nodiscard]] rpl::producer<int> likeToastOffset() const {
		return _playbackBar->geometryValue() | rpl::map([=](QRect bar) {
			return height() - bar.y();
		});
	}

	void showHome() {
		if (_tab != Tab::Home) {
			setTab(Tab::Home);
		} else if (_homeSection || _homeAlbumOpen) {
			rememberNavigation();
			resetHomeNavigation();
			relayout();
			rebuildTab();
			_tabScroll->scrollToY(0);
		}
	}

	void close() {
		if (_closing) {
			return;
		}
		_closing = true;
		_searchHintMenu.hide();
		stopHomeBackExit();
		_ambientTimer.stop();
		if (_cancelMotion) _cancelMotion();
		_motionCover = nullptr;
		_spotifyCanvas = nullptr;
		_canvas.refresh({}, {}, {}, false);
		_upcomingCanvas.refresh({}, {}, {}, false);
		if (!PlayerMotionEnabled()) {
			_fade.stop();
			hide();
			deleteLater();
			return;
		}
		if (!graphicsEffect()) {
			const auto effect = new QGraphicsOpacityEffect(this);
			effect->setOpacity(1.);
			setGraphicsEffect(effect);
		}
		_fade.start(1., 0., kFastEffects);
		if (!_fade.animating()) {
			hide();
			deleteLater();
		}
	}
	~FullscreenPlayer() override {
		if (_cancelMotion) _cancelMotion();
	}

protected:
	void showEvent(QShowEvent *event) override {
		RpWidget::showEvent(event);
		_canvas.setVisible(true);
		updateCoverTitleScroll();
		updateAmbientTimer();
	}
	void hideEvent(QHideEvent *event) override {
		_homeSideGlowFade.stop();
		_homeSideGlowPulsing = false;
		_searchHintMenu.hide();
		_ambientTimer.stop();
		_coverTitleTimer.stop();
		_titleElapsed = {};
		_canvas.setVisible(false);
		_upcomingCanvas.refresh({}, {}, {}, false);
		_preparedBackdrop = QImage();
		RpWidget::hideEvent(event);
	}
	bool eventFilter(QObject *watched, QEvent *event) override {
		const auto type = event->type();
		const auto mouse = type == QEvent::MouseButtonPress
			|| type == QEvent::MouseButtonRelease || type == QEvent::MouseButtonDblClick;
		if (type != QEvent::KeyPress && !mouse) {
			return RpWidget::eventFilter(watched, event);
		}
		const auto widget = qobject_cast<QWidget*>(watched);
		if (_closing || !isVisible() || !widget || (widget != this && !isAncestorOf(widget))
			|| widget->window() != window()) {
			return RpWidget::eventFilter(watched, event);
		}
		if (_tab != Tab::Search) {
			if (mouse) {
				const auto button = static_cast<QMouseEvent*>(event)->button();
				if (button == Qt::BackButton || button == Qt::ForwardButton) {
					if (type == QEvent::MouseButtonPress) {
						stepNavigation(button == Qt::BackButton);
					}
					return true;
				}
			}
			if (type == QEvent::KeyPress && !qobject_cast<QLineEdit*>(widget)
				&& widget != _queueSearch && !_queueSearch->isAncestorOf(widget)) {
				const auto key = static_cast<QKeyEvent*>(event);
				if (key->modifiers() == Qt::NoModifier) {
					Ui::AbstractButton *button = nullptr;
					switch (key->key()) {
					case Qt::Key_Space: button = _play.get(); break;
					case Qt::Key_E: button = _next.get(); break;
					case Qt::Key_Q: button = _previous.get(); break;
					case Qt::Key_W: button = _queueToggle.get(); break;
					case Qt::Key_L: button = _heart.get(); break;
					case Qt::Key_H: button = _nav[int(Tab::Home)]; break;
					case Qt::Key_N: button = _nav[int(Tab::Now)]; break;
					case Qt::Key_S: button = _nav[int(Tab::Search)]; break;
					case Qt::Key_P: button = _nav[int(Tab::Playlists)]; break;
					default: break;
					}
					if (button) {
						if (!key->isAutoRepeat() && !button->isDisabled()) {
							button->clicked(Qt::NoModifier, Qt::LeftButton);
						}
						return true;
					}
				}
			}
		}
		if (_tab == Tab::Search && type == QEvent::KeyPress) {
			const auto key = static_cast<QKeyEvent*>(event);
			if (watched == _searchField && key->key() == Qt::Key_Escape) {
				if (!_searchField->text().isEmpty()) _searchField->clear();
				else _searchField->clearFocus();
				return true;
			}
			const auto current = std::find(_searchResultWidgets.begin(), _searchResultWidgets.end(), watched);
			if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up) {
				if (watched == _searchField || current != _searchResultWidgets.end()) {
					if (_searchResultWidgets.empty()) return false;
					const auto index = current == _searchResultWidgets.end() ? -1 : int(current - _searchResultWidgets.begin());
					const auto next = key->key() == Qt::Key_Down ? index + 1 : index - 1;
					if (next < 0) _searchField->setFocus();
					else if (next < int(_searchResultWidgets.size())) {
						auto widget = _searchResultWidgets[next];
						widget->setFocus();
						const auto top = widget->mapTo(_tabContent, QPoint()).y();
						_tabScroll->scrollToY(top, top + widget->height());
					}
					return true;
				}
			}
			if (current != _searchResultWidgets.end() && key->key() == Qt::Key_Space) {
				return true;
			}
			if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
				if (watched == _searchField) {
					if (_searchTimer.isActive()) runSearch();
					else if (!_searchResultWidgets.empty() && !key->isAutoRepeat()) {
						const auto first = static_cast<Ui::AbstractButton*>(_searchResultWidgets.front());
						first->clicked(Qt::NoModifier, Qt::LeftButton);
					}
					return true;
				} else if (current != _searchResultWidgets.end()) {
					if (!key->isAutoRepeat()) {
						static_cast<Ui::AbstractButton*>(widget)->clicked(Qt::NoModifier, Qt::LeftButton);
					}
					return true;
				}
			}
		}
		return RpWidget::eventFilter(watched, event);
	}
	void keyPressEvent(QKeyEvent *e) override {
		if (e->key() == Qt::Key_Escape) {
			if (_queuePanelOpen) {
				setQueuePanelOpen(false);
			} else {
				close();
			}
			e->accept();
		} else {
			RpWidget::keyPressEvent(e);
		}
	}
	void mousePressEvent(QMouseEvent *e) override {
		setFocus();
		if (e->button() != Qt::LeftButton) {
			return;
		}
		_searchHintMenu.hide();
		const auto hover = infoAt(e->pos());
		if (hover == InfoHover::None) {
			return;
		}
		if (hover == InfoHover::BarCover) {
			openNowPlayingFromBar();
			return;
		}
		if (hover == InfoHover::Cover) {
			setHover(InfoHover::None);
			if (_coverMorph.animating()) {
				startCoverTransition(!_coverExpanding);
			} else {
				startCoverTransition(false);
			}
			return;
		}
		const auto track = PlayingTrack();
		const auto artist = track.artist.trimmed();
		if (hover == InfoHover::Title || hover == InfoHover::BarTitle) {
			setHover(InfoHover::None);
			openHomeTrack(track);
		} else if (!artist.isEmpty()) {
			setHover(InfoHover::None);
			chooseTrackArtist(track);
		}
	}
	void contextMenuEvent(QContextMenuEvent *e) override {
		const auto layout = computeLayout();
		const auto point = e->pos();
		if ((_tab == Tab::Now && layout.cover.contains(point))
			|| layout.barCover.contains(point)
			|| barInfoRect(layout).contains(point)) {
			ShowCurrentTrackMenu(_controller, e->globalPos(), this);
			e->accept();
		} else {
			RpWidget::contextMenuEvent(e);
		}
	}
	void mouseMoveEvent(QMouseEvent *e) override {
		setHover(infoAt(e->pos()));
	}
	void leaveEventHook(QEvent *e) override {
		setHover(InfoHover::None);
	}
	enum class InfoHover { None, Title, Artist, Cover, BarCover, BarTitle, BarArtist };
	void setHover(InfoHover hover) {
		if (_hover != hover) {
			if (_coverTitleTimer.isActive()) advanceTitleClock();
			_hover = hover;
			setCursor((hover == InfoHover::None)
				? style::cur_default
				: style::cur_pointer);
			const auto layout = computeLayout();
			update(layout.info);
			update(layout.bar);
		}
	}
	enum class Tab { Now, Home, Search, Library, Playlists, Count };
	struct Layout {
		bool verticalNav = false;
		bool compact = false;
		bool ultra = false;
		bool barShown = false;
		QRect nav;
		QRect homeBack;
		QRect center;
		QRect bar;
		int contentBottom = 0;
		int scrollTop = 0;
		QRect cover;
		QRect info;
		QRect lyrics;
		QRect queuePanel;
		QRect barCover;
		QRect barInfo;
		QRect seek;
		QRect timeNow;
		QRect timeTotal;
		QRect volume;
		QPoint heart;
		QPoint queue;
		QPoint collapse;
		int controlsLeft = 0;
		int controlsWidth = 0;
		int controlsTop = 0;
	};
	[[nodiscard]] bool showsQueue() const {
		return _tab == Tab::Now && _lyricsMissing;
	}
	[[nodiscard]] Layout computeLayout() const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		auto result = Layout();
		const auto windowWidth = width();
		using namespace PlayerStyle;
		const auto margin = s(windowWidth < s(560) ? kSpace8 : kSpace16);
		const auto gap = s(kSpace12);
		const auto pad = s(kSpace16);
		const auto now = (_tab == Tab::Now);
		const auto queueWidth = std::min(
			std::max(windowWidth - 2 * margin, 0),
			std::clamp(windowWidth / 4, s(280), s(360)));
		const auto panelWidth = std::min(queueWidth, std::max((windowWidth - 3 * margin) / 2, 0));
		const auto queueLeft = windowWidth - margin - panelWidth;
		const auto contentRight = windowWidth - margin - qRound((panelWidth + gap) * _queueProgress);
		const auto contentWidth = std::max(contentRight - margin, 0);

		const auto barHeight = s(kBarHeight);
		const auto barWidth = std::min(contentWidth, s(kBarMaxWidth));
		const auto barProgress = _barSlide.unitValue(_hasPlaybackTrack ? 1. : 0.);
		result.barShown = _hasPlaybackTrack || barProgress > 0.;
		// Список меняет размер один раз, каждый кадр двигается сама панель.
		result.contentBottom = height() - gap
			- (_hasPlaybackTrack ? margin + barHeight : 0);
		result.bar = QRect(
			margin + (contentWidth - barWidth) / 2,
			height() - int(base::SafeRound((margin + barHeight) * barProgress)),
			barWidth,
			barHeight);

		// Сверху ряд вкладок по центру основной области, справа кнопка «свернуть».
		const auto homeDetail = (_tab == Tab::Home && _homeAlbumOpen);
		const auto headerGap = s(kSpace8);
		const auto back = QRect(margin + _filter->width() + headerGap, margin, s(96), s(40));
		const auto reserve = margin + _collapse->width() + headerGap;
		const auto leftReserve = homeDetail ? back.x() + back.width() + headerGap : reserve;
		const auto tabsBelow = homeDetail && (windowWidth - leftReserve - reserve < s(240));
		const auto navLeft = tabsBelow ? margin : leftReserve;
		const auto navRight = tabsBelow ? margin : reserve;
		const auto navWidth = std::clamp(windowWidth - navLeft - navRight, 0, s(900));
		result.verticalNav = windowWidth < s(720) || (homeDetail && navWidth < s(540));
		result.nav = QRect(
			std::max((windowWidth - navWidth) / 2, navLeft),
			tabsBelow ? margin + s(40) + gap : margin,
			navWidth,
			s(result.verticalNav ? 80 : 40));
		result.collapse = QPoint(
			windowWidth - margin - _collapse->width(),
			margin + (s(40) - _collapse->height()) / 2);
		if (homeDetail) {
			result.homeBack = back;
		}
		const auto top = result.nav.y() + result.nav.height() + gap;
		const auto areaBottom = result.contentBottom;
		const auto areaHeight = std::max(areaBottom - top, 0);
		result.queuePanel = QRect(queueLeft, top, panelWidth, areaHeight);

		if (now) {
			auto lyricsLeft = margin;
			auto lyricsTop = top;
			if (contentRight + margin >= s(800)) {
				// Обложка слева, текст песни по центру остального экрана.
				const auto colWidth = std::clamp(int((contentRight + margin) * 0.29), s(190), s(460));
				const auto side = std::max(std::min({
					colWidth - 2 * pad,
					std::max(areaHeight - pad, 0) * 4 / 5,
					s(420) }), 0);
				const auto coverHeight = side * 5 / 4;
				const auto y = top + std::max((areaHeight - coverHeight) / 2, 0);
				result.cover = QRect(margin + (colWidth - side) / 2, y, side, coverHeight);
				lyricsLeft = margin + colWidth + s(kSpace32);
			} else {
				// В узком окне обложка над текстом, если по высоте хватает места для обоих.
				const auto coverHeight = std::min(areaHeight * 2 / 5, s(360));
				const auto side = std::min(contentWidth - 2 * pad, coverHeight * 4 / 5);
				if (side >= s(128)) {
					result.cover = QRect(margin + (contentWidth - side) / 2, top, side, side * 5 / 4);
					lyricsTop += result.cover.height() + gap;
				}
			}
			if (!result.cover.isEmpty()) {
				const auto cover = result.cover;
				const auto infoInset = std::min(s(kSpace24), cover.width() / 4);
				const auto infoHeight = kTrackTitle.lineHeight()
					+ s(kSpace2) + kTrackArtist.lineHeight();
				result.info = QRect(
					cover.x() + infoInset,
					cover.y() + std::max(cover.height() - infoHeight - s(kSpace24), 0),
					cover.width() - 2 * infoInset,
					std::min(infoHeight, cover.height()));
			}
			result.lyrics = QRect(
				lyricsLeft,
				lyricsTop,
				std::max(contentRight - lyricsLeft, 0),
				std::max(areaBottom - lyricsTop, 0));
			if (showsQueue()) {
				result.center = result.lyrics;
				result.scrollTop = result.center.y();
			}
		} else {
			result.center = QRect(margin, top, contentWidth, areaHeight);
			result.scrollTop = result.center.y()
				+ (_tab == Tab::Search ? searchHeaderHeight(contentWidth) : 0);
		}

		// Внизу: полоса управления воспроизведением.
		const auto &bar = result.bar;
		// Обложка концентрична капсуле: одинаковый отступ сверху, снизу и слева.
		const auto barInset = (bar.height() - s(kBarCover)) / 2;
		result.ultra = bar.width() < s(560);
		result.compact = bar.width() < s(900);
		const auto volumeWidth = s(28) + s(100);
		const auto leftWidth = result.ultra
			? 0
			: result.compact
			? barInset + s(kBarCover) + pad / 2
			: std::clamp(int(bar.width() * 0.3), s(280), s(380));
		const auto actionGap = s(kSpace2);
		const auto volumeGap = s(kSpace12);
		const auto rightWidth = pad + _queueToggle->width() + actionGap + _heart->width() + pad
			+ (result.compact ? 0 : volumeWidth + volumeGap);
		const auto sideWidth = std::max(leftWidth, rightWidth);
		const auto centerWidth = std::max(bar.width() - (result.ultra ? rightWidth + pad : 2 * sideWidth), 0);
		const auto centerLeft = result.ultra
			? bar.x() + pad
			: bar.x() + (bar.width() - centerWidth) / 2;
		result.controlsLeft = centerLeft;
		result.controlsWidth = centerWidth;
		result.controlsTop = bar.y()
			+ (bar.height() - _play->height() - _seek->height()) / 2;
		const auto seekTop = result.controlsTop + _play->height();
		const auto rowWidth = std::max(std::min(
			result.ultra ? bar.width() - 2 * pad : centerWidth - pad, s(568)), 0);
		const auto rowLeft = result.ultra
			? bar.x() + (bar.width() - rowWidth) / 2
			: centerLeft + (centerWidth - rowWidth) / 2;
		const auto timeWidth = s(44);
		const auto timeGap = s(kSpace4);
		result.timeNow = QRect(rowLeft, seekTop, timeWidth - timeGap, _seek->height());
		result.seek = QRect(
			rowLeft + timeWidth,
			seekTop,
			std::max(rowWidth - 2 * timeWidth, 0),
			_seek->height());
		result.timeTotal = QRect(
			rowLeft + rowWidth - timeWidth + timeGap,
			seekTop,
			timeWidth - timeGap,
			_seek->height());
		result.queue = QPoint(
			bar.x() + bar.width() - barInset - _queueToggle->width(),
			result.ultra ? result.controlsTop + (_play->height() - _queueToggle->height()) / 2
				: bar.y() + (bar.height() - _queueToggle->height()) / 2);
		result.heart = QPoint(
			result.queue.x() - actionGap - _heart->width(),
			result.ultra ? result.controlsTop + (_play->height() - _heart->height()) / 2
				: bar.y() + (bar.height() - _heart->height()) / 2);
		if (!result.compact) {
			result.volume = QRect(
				result.heart.x() - volumeGap - volumeWidth,
				bar.y() + (bar.height() - _volume->height()) / 2,
				volumeWidth,
				_volume->height());
		}
		if (!result.ultra) {
			result.barCover = QRect(
				bar.x() + barInset,
				bar.y() + barInset,
				s(kBarCover),
				s(kBarCover));
			const auto infoLeft = result.barCover.x() + result.barCover.width() + s(kSpace16);
			result.barInfo = QRect(
				infoLeft,
				result.barCover.y(),
				std::max(bar.x() + leftWidth - infoLeft - s(kSpace8), 0),
				s(kBarCover));
		}
		return result;
	}
	struct MetadataLines {
		QRect title;
		QRect artist;
	};

	[[nodiscard]] static MetadataLines metadataLines(QRect area, bool compact) {
		using namespace PlayerStyle;
		const auto titleHeight = (compact ? kCompactTitle : kTrackTitle).lineHeight();
		const auto artistHeight = (compact ? kCompactArtist : kTrackArtist).lineHeight();
		const auto gap = style::ConvertScale(kSpace2);
		const auto top = area.y()
			+ std::max((area.height() - titleHeight - gap - artistHeight) / 2, 0);
		return {
			QRect(area.x(), top, area.width(), titleHeight),
			QRect(area.x(), top + titleHeight + gap, area.width(), artistHeight),
		};
	}
	void advanceTitleClock() {
		const auto now = crl::now();
		const auto delta = now - _titleTickAt;
		_titleTickAt = now;
		if (_coverTitleMoves && _hover != InfoHover::Title) {
			_titleElapsed[0] += delta;
		}
		if (_barTitleMoves && _hover != InfoHover::BarTitle) {
			_titleElapsed[1] += delta;
		}
	}

	void updateCoverTitleScroll() {
		using namespace PlayerStyle;
		const auto layout = computeLayout();
		const auto coverTitle = metadataLines(layout.info, false).title;
		const auto barTitle = metadataLines(barInfoRect(layout), true).title;
		const auto moves = [&](QRect rect, TextStyle style) {
			return !rect.isEmpty()
				&& QFontMetricsF(style.musicFont()).horizontalAdvance(_title) > rect.width();
		};
		_coverTitleMoves = _tab == Tab::Now && moves(coverTitle, kTrackTitle);
		_barTitleMoves = layout.barShown && !layout.ultra && !layout.compact
			&& moves(barTitle, kCompactTitle);
		if (isVisible() && (_coverTitleMoves || _barTitleMoves)) {
			if (!_coverTitleTimer.isActive()) {
				_titleTickAt = crl::now();
				_coverTitleTimer.start();
			}
		} else {
			_coverTitleTimer.stop();
			_titleElapsed = {};
		}
	}

	void paintMovingTitle(
			QPainter &p,
			QRect rect,
			const QString &title,
			bool compact,
			bool current) const {
		using namespace PlayerStyle;
		const auto hover = compact ? InfoHover::BarTitle : InfoHover::Title;
		const auto font = (compact ? kCompactTitle : kTrackTitle).musicFont(current && _hover == hover);
		const auto sinceTick = (_coverTitleTimer.isActive() && _hover != hover)
			? crl::now() - _titleTickAt
			: 0;
		PaintMovingTitle(p, rect, title, font,
			current ? _titleElapsed[compact] + sinceTick : 0,
			devicePixelRatioF(), _titlePaths[compact]);
	}
	[[nodiscard]] QRect barInfoRect(const Layout &layout) const {
		const auto shown = _coverMorph.animating()
			? 1. - _coverTransition.expanded()
			: _plainAnim.unitValue(_tab == Tab::Now ? 0. : 1.);
		const auto shift = int((layout.barInfo.x() - layout.barCover.x()) * (1. - shown));
		return layout.barInfo.adjusted(-shift, 0, 0, 0);
	}
	// Где под курсором название или исполнитель (по ширине самого текста, не всей строки).
	[[nodiscard]] InfoHover infoAt(QPoint point) const {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto layout = computeLayout();
		if (_coverMorph.animating() && _coverTransition.rect().contains(point)) {
			return InfoHover::Cover;
		}
		using namespace PlayerStyle;
		const auto hitRect = [](QRect line, TextStyle style, const QString &text) {
			line.setWidth(std::min(line.width(), QFontMetrics(style.musicFont()).horizontalAdvance(text)));
			return line;
		};
		const auto lines = metadataLines(layout.info, false);
		auto title = lines.title;
		title.setWidth(std::min(title.width(), QFontMetrics(kTrackTitle.musicFont()).horizontalAdvance(_title)));
		const auto artist = hitRect(lines.artist, kTrackArtist, _artist);
		if (_tab != Tab::Now
			&& !layout.ultra
			&& width() >= s(760)
			&& layout.barCover.contains(point)) {
			return InfoHover::BarCover;
		}
		if (!layout.ultra && !layout.compact && layout.barInfo.width() > 0) {
			const auto info = barInfoRect(layout);
			const auto artistRect = hitRect(
				metadataLines(info, true).artist, kCompactArtist, _artist);
			if (artistRect.contains(point) && !_artist.trimmed().isEmpty()) {
				return InfoHover::BarArtist;
			} else if (info.contains(point) && !_title.trimmed().isEmpty()) {
				return InfoHover::BarTitle;
			}
		}
		if (layout.cover.isEmpty()) {
			return InfoHover::None;
		}
		return title.contains(point)
			? InfoHover::Title
			: (artist.contains(point) && !_artist.trimmed().isEmpty())
			? InfoHover::Artist
			: layout.cover.contains(point)
			? InfoHover::Cover
			: InfoHover::None;
	}

	void resizeEvent(QResizeEvent *e) override {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto layout = computeLayout();
		const auto barOrigin = layout.bar.topLeft();
		layoutPlaybackBar(layout);
		_collapse->move(layout.collapse);
		_filter->move(width() - layout.collapse.x() - _collapse->width(), layout.collapse.y());
		if (_homeBackOverlay && (!layout.homeBack.isEmpty() || e->size() != e->oldSize())) {
			stopHomeBackExit();
		}
		_homeBack->setGeometry(layout.homeBack);
		_homeBack->setVisible(!layout.homeBack.isEmpty());
		_queueToggle->move(layout.queue - barOrigin);
		_heart->move(layout.heart - barOrigin);
		for (const auto button : { _shuffle.get(), _previous.get(), _play.get(), _next.get(), _repeat.get(), _queueToggle.get() }) {
			button->setVisible(layout.barShown);
		}
		_shuffle->setVisible(layout.barShown && !layout.ultra);
		_repeat->setVisible(layout.barShown && !layout.ultra);
		_seek->setVisible(layout.barShown);
		_heart->setVisible(layout.barShown && _heartOwn);
		_volume->setVisible(layout.barShown && !layout.compact);
		if (!layout.compact) {
			_volume->setGeometry(layout.volume.translated(-barOrigin));
		}
		_seek->setGeometry(layout.seek.translated(-barOrigin));
		// The seek bar can restore its own background inside the solid capsule.
		// This keeps its animation from invalidating the scrolling feed below.
		const auto inset = s(1);
		_seek->setOpaqueBackground(_tab == Tab::Home && !layout.seek.isEmpty()
			&& Ui::SquirclePath(QRectF(layout.bar), s(kBarRadius)).contains(
				QRectF(layout.seek).adjusted(-inset, -inset, inset, inset)));
		_view->setVisible(_tab == Tab::Now && !showsQueue());
		_view->setGeometry(layout.lyrics);
		_tabScroll->setVisible(_tab != Tab::Now || (showsQueue() && !_queuePanelOpen));
		_queuePanelTarget = layout.queuePanel;
		layoutQueuePanel();
		const auto home = (_tab == Tab::Home);
		const auto scrollBottom = home
			? (_hasPlaybackTrack ? height() : layout.contentBottom)
			: layout.center.bottom() + 1 - s(12);
		const auto bottomPadding = (home && _hasPlaybackTrack) ? height() - layout.contentBottom : 0;
		const auto queueInset = s(PlayerStyle::kSpace8);
		_tabContent->setPadding(showsQueue()
			? QMargins(queueInset, queueInset, queueInset, queueInset)
			: QMargins(0, 0, 0, bottomPadding));
		const auto scrollWidth = ((home && _homeAlbumOpen) || _tab == Tab::Search || showsQueue())
			? std::min(layout.center.width(), s(kMusicListMaxWidth))
			: layout.center.width();
		_tabScroll->setGeometry(
			layout.center.x() + (layout.center.width() - scrollWidth) / 2,
			layout.scrollTop,
			scrollWidth,
			std::max(scrollBottom - layout.scrollTop, 0));
		if (_searchEmptySticker) {
			_searchEmptySticker->setGeometry(searchHintGeometry(layout));
		}
		if (_tabContent->width() != _tabScroll->width()) {
			_tabContent->resizeToWidth(_tabScroll->width());
		}
		updateHomeArtistBar();

		// Навигация: ряд пилюль сверху по центру.
		{
			const auto inset = s(4);
			const auto count = int(_nav.size());
			const auto inner = std::max(layout.nav.width() - 2 * inset, 0);
			const auto columns = layout.verticalNav ? 3 : count;
			for (auto i = 0; i != count; ++i) {
				const auto at = navColumn(i, columns);
				const auto left = layout.nav.x() + inset + int(inner * at / columns);
				const auto right = layout.nav.x() + inset + int(inner * (at + 1.) / columns);
				_nav[i]->setCentered(true);
				_nav[i]->setGeometry(left, layout.nav.y() + s(40) * (i / columns), right - left, s(40));
			}
		}
		{
			const auto inner = std::max(layout.center.width() - s(20), 0);
			const auto left = layout.center.x() + s(10);
			const auto fieldHeight = s(52);
			const auto fieldTop = layout.center.y() + s(8);
			_searchIndicator->setGeometry(s(18), (fieldHeight - s(20)) / 2, s(20), s(20));
			_searchRetry->setGeometry(left, layout.scrollTop + s(52), s(110), s(32));
			auto font = st::normalFont->f;
			font.setPixelSize(s(15));
			_searchField->setFont(font);
			const auto fieldStyle = u"QLineEdit { background: %3; color: %4; border: 1px solid transparent; border-radius: %1px; padding: 0 %2px 0 %6px; selection-background-color: %5; } QLineEdit:focus { border: 1px solid %5; }"_q
				.arg(fieldHeight / 2)
				.arg(s(20))
				.arg(st::windowBgOver->c.name())
				.arg(st::windowFg->c.name())
				.arg(st::windowActiveTextFg->c.name())
				.arg(s(50));
			if (_searchField->styleSheet() != fieldStyle) {
				_searchField->setStyleSheet(fieldStyle);
			}
			if (!_searchChats) {
				layoutCatalogHeader(layout.center);
			} else {
				const auto fieldWidth = std::min(inner, s(760));
				_searchField->setGeometry(left + (inner - fieldWidth) / 2, fieldTop, fieldWidth, fieldHeight);
				const auto switchTop = fieldTop + fieldHeight + s(12);
				const auto stacked = layout.center.width() < s(500);
				const auto switchWidth = stacked
					? std::min(inner, s(240))
					: std::min(std::max((inner - s(8)) / 2, 0), s(240));
				const auto groupWidth = stacked ? switchWidth : 2 * switchWidth + s(8);
				const auto groupLeft = left + (inner - groupWidth) / 2;
				_sourceSwitch->setGeometry(groupLeft, switchTop, switchWidth, s(40));
				_chooseSearchChat->setGeometry(
					stacked ? groupLeft : groupLeft + switchWidth + s(8),
					stacked ? switchTop + s(48) : switchTop,
					switchWidth,
					s(40));
			}
		}

		const auto row = { _shuffle, _previous, _play, _next, _repeat };
		auto widths = 0;
		auto count = 0;
		for (const auto button : row) {
			if (layout.ultra && (button == _shuffle || button == _repeat)) continue;
			widths += button->width();
			++count;
		}
		const auto gap = std::clamp((layout.controlsWidth - widths) / (count - 1),
			s(PlayerStyle::kSpace2), s(PlayerStyle::kSpace16));
		const auto top = layout.controlsTop - barOrigin.y();
		auto x = layout.controlsLeft - barOrigin.x()
			+ (layout.controlsWidth - widths - gap * (count - 1)) / 2;
		for (const auto button : row) {
			if (layout.ultra && (button == _shuffle || button == _repeat)) continue;
			button->move(x, top + (_play->height() - button->height()) / 2);
			x += button->width() + gap;
		}
		for (const auto button : { _collapse.get(), _filter.get(), _queueToggle.get(), _heart.get() }) {
			button->raise();
		}
		RpWidget::resizeEvent(e);
		updateCoverTitleScroll();
		scheduleHomeMoreIfNeeded();
		_coverOverlay->setGeometry(rect());
		if (_tab == Tab::Home) {
			updateAmbientTimer();
		}
		if (e->size() != e->oldSize() && _coverMorph.animating()) {
			if (!prepareCoverTransition(layout)) {
				_coverMorph.stop();
			}
			updateCoverTransition();
		}
	}

	void relayout() {
		auto event = QResizeEvent(size(), size());
		resizeEvent(&event);
		update();
	}

	void layoutPlaybackBar(const Layout &layout) {
		const auto previous = _playbackBar->geometry();
		_playbackBar->setGeometry(layout.bar);
		_playbackBar->setVisible(layout.barShown);
		if (_tab != Tab::Home && previous != layout.bar) {
			update(previous.united(layout.bar));
		}
	}

	void setPlaybackBarShown(bool shown) {
		if (_hasPlaybackTrack == shown) {
			return;
		}
		const auto from = _hasPlaybackTrack ? 1. : 0.;
		_hasPlaybackTrack = shown;
		if (!isVisible()) {
			_barSlide.stop();
		} else {
			_barSlide.start(from, shown ? 1. : 0., kFastSpatial);
		}
		if (!shown && _queuePanelOpen) {
			setQueuePanelOpen(false);
		}
		relayout();
	}

	[[nodiscard]] double navColumn(int index, int columns) const {
		const auto rowStart = index - index % columns;
		const auto rowSize = std::min(columns, int(_nav.size()) - rowStart);
		return index % columns + (columns - rowSize) / 2.;
	}

	void layoutQueuePanel() {
		const auto &target = _queuePanelTarget;
		if (target.isEmpty() || (!_queuePanelOpen && _queueProgress <= 0.)) {
			_queuePanel->hide();
			return;
		}
		const auto offset = qRound((width() - target.x()) * (1. - _queueProgress));
		const auto geometry = target.translated(offset, 0);
		const auto resize = (_queuePanel->size() != target.size());
		if (_queuePanel->geometry() != geometry) {
			_queuePanel->setGeometry(geometry);
			_queuePanel->update();
		}
		if (resize) {
			stopQueueAdvance();
			const auto s = [](int v) { return style::ConvertScale(v); };
			using namespace PlayerStyle;
			const auto tabsTop = s(kSpace12);
			const auto tabsHeight = s(40);
			const auto side = s(kSpace16);
			const auto tabWidth = std::max(target.width() - 2 * side, 0);
			_queueTabBar->setGeometry(side, tabsTop, tabWidth, tabsHeight);
			const auto inset = s(kSpace4);
			const auto inner = std::max(tabWidth - 2 * inset, 0);
			const auto first = inner / 2;
			_queueTabs[0]->setGeometry(inset, 0, first, tabsHeight);
			_queueTabs[1]->setGeometry(inset + first, 0, inner - first, tabsHeight);
			_queueSearch->resizeToWidth(std::max(tabWidth, 1));
			const auto gap = s(kSpace8);
			_queueSearch->moveToLeft(side, tabsTop + tabsHeight + gap, target.width());
			const auto scrollTop = _queueSearch->y() + _queueSearch->height() + gap;
			_queueScroll->setGeometry(
				gap, scrollTop, std::max(target.width() - 2 * gap, 0),
				std::max(target.height() - scrollTop - s(kSpace12), 0));
			if (_queueContent->width() != _queueScroll->width()) {
				_queueContent->resizeToWidth(_queueScroll->width());
			}
			updateQueueEdgeFade(_queueScroll, _queueEdgeFade);
		}
		if (_queuePanel->isHidden()) {
			_queuePanel->show();
		}
	}

	static void updateQueueEdgeFade(Ui::ScrollArea *scroll, QGraphicsOpacityEffect *effect) {
		const auto height = scroll->viewport()->height();
		const auto depth = std::min(style::ConvertScale(14), height / 2);
		const auto top = scroll->scrollTop();
		const auto above = top > 0;
		const auto below = top < scroll->scrollTopMax();
		effect->setEnabled(depth > 0 && (above || below));
		if (!effect->isEnabled()) return;

		const auto fraction = depth / double(height);
		auto mask = QLinearGradient(0., 0., 0., height);
		mask.setColorAt(0., above ? Qt::transparent : Qt::black);
		mask.setColorAt(fraction, Qt::black);
		mask.setColorAt(1. - fraction, Qt::black);
		mask.setColorAt(1., below ? Qt::transparent : Qt::black);
		effect->setOpacityMask(QBrush(mask));
	}

	void setQueuePanelOpen(bool open) {
		if (_queuePanelOpen == open) {
			return;
		}
		const auto from = _queueProgress;
		const auto to = open ? 1. : 0.;
		_queuePanelOpen = open;
		if (!open) {
			stopQueueAdvance();
		}
		_queueToggle->setQueueOpen(open);
		if (open) {
			_queuePanel->raise();
		} else if (const auto focused = QApplication::focusWidget()) {
			if (_queuePanel->isAncestorOf(focused)) {
				_queueToggle->setFocus();
			}
		}
		{
			const auto block = QSignalBlocker(&_queueSlide);
			_queueSlide.stop();
			_queueSlide.setDuration(std::max(120, qRound(360 * std::abs(to - from))));
			_queueSlide.setStartValue(from);
			_queueSlide.setEndValue(to);
			_queueSlide.setCurrentTime(0);
		}
		_queueSlide.start();
		relayout();
		if (open) {
			refreshQueuePanel();
		}
	}

	[[nodiscard]] static bool catalogSourceInline(int width) {
		return width >= style::ConvertScale(680);
	}

	[[nodiscard]] bool showCatalogFilterSkeleton() const {
		return _searchPending && !_searchChats
			&& !_searchFilterAvailable[int(SearchCategory::All)];
	}

	[[nodiscard]] bool catalogFiltersWrap(int width) const {
		return width < style::ConvertScale(480)
			&& (showCatalogFilterSkeleton()
				|| std::ranges::count(_searchFilterAvailable, true) > 3);
	}

	[[nodiscard]] static int catalogFiltersTop(int width) {
		return style::ConvertScale(catalogSourceInline(width) ? 70 : 114);
	}

	[[nodiscard]] int searchHeaderHeight(int width) const {
		if (_searchChats) {
			return style::ConvertScale((width < style::ConvertScale(500)) ? 172 : 124);
		}
		if (!_searchFilterAvailable[int(SearchCategory::All)] && !showCatalogFilterSkeleton()) {
			return style::ConvertScale(catalogSourceInline(width) ? 72 : 120);
		}
		return catalogFiltersTop(width)
			+ style::ConvertScale(catalogFiltersWrap(width) ? 86 : 46);
	}

	void layoutCatalogHeader(const QRect &area) {
		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto inner = std::max(area.width() - s(40), 0);
		const auto inlineSource = catalogSourceInline(area.width());
		const auto sourceWidth = std::min(inner, s(240));
		const auto gap = s(12);
		const auto fieldWidth = inlineSource
			? std::min(inner - sourceWidth - gap, s(760))
			: inner;
		const auto headerWidth = inlineSource ? fieldWidth + gap + sourceWidth : inner;
		const auto left = area.x() + s(20) + (inner - headerWidth) / 2;
		_searchField->setGeometry(left, area.y() + s(8), fieldWidth, s(52));
		_sourceSwitch->setGeometry(
			inlineSource ? left + fieldWidth + gap : left,
			area.y() + s(inlineSource ? 14 : 68),
			sourceWidth,
			s(40));

		const auto skeleton = showCatalogFilterSkeleton();
		if (!_searchFilterAvailable[int(SearchCategory::All)] && !skeleton) {
			return;
		}
		const auto light = IsLightMusicTheme();
		const auto selected = light ? MusicSelectionColor() : st::activeButtonBg->c;
		const auto selectedHover = light
			? anim::color(selected, st::windowFg->c, .06)
			: st::activeButtonBgOver->c;
		const auto selectedInk = light ? st::windowFg->c : st::activeButtonFg->c;
		const auto sheet = u"QPushButton { background: %1; color: %2; border: %7px solid transparent; border-radius: %8px; } "
			u"QPushButton:hover { background: %3; } QPushButton:pressed { background: %4; } "
			u"QPushButton:checked { background: %4; color: %5; } "
			u"QPushButton:checked:hover { background: %6; } "
			u"QPushButton:focus { border-color: %9; } QPushButton:checked:focus { border-color: %5; }"_q
			.arg(st::boxBg->c.name(), st::windowFg->c.name(), st::windowBgOver->c.name(),
				selected.name(), selectedInk.name(), selectedHover.name())
			.arg(s(2))
			.arg(s(16))
			.arg(st::windowActiveTextFg->c.name());
		const auto wrap = catalogFiltersWrap(area.width());
		auto x = left;
		auto y = area.y() + catalogFiltersTop(area.width());
		auto placed = 0;
		for (auto i = 0; i != int(_searchFilters.size()); ++i) {
			if (!_searchFilterAvailable[i] && !skeleton) {
				continue;
			}
			if (wrap && placed == 3) {
				x = left;
				y += s(40);
			}
			const auto chip = _searchFilters[i];
			if (chip->styleSheet() != sheet) {
				chip->setStyleSheet(sheet);
				chip->setFont(st::semiboldFont->f);
			}
			const auto width = st::semiboldFont->width(chip->text()) + s(24);
			chip->setGeometry(x, y, width, s(32));
			x += width + s(6);
			++placed;
		}
	}

	void setCatalogFilterAvailability(std::array<bool, 5> available) {
		if (_searchFilterAvailable != available) {
			_searchFilterAvailable = available;
			if (_tab == Tab::Search && !_searchChats) {
				relayout();
			}
		}
	}

	void resetCatalogFilters() {
		_searchCategory = SearchCategory::All;
		_searchFilters[int(SearchCategory::All)]->setChecked(true);
		setCatalogFilterAvailability({});
	}

	void updateCatalogFilters() {
		auto available = std::array<bool, 5>();
		available[int(SearchCategory::All)] = !_catalogItems.empty();
		for (const auto &item : _catalogItems) {
			const auto category = [&] {
				if (const auto album = std::get_if<Album>(&item)) {
					return (album->releaseKind == ReleaseKind::Unknown)
						? SearchCategory::Playlists
						: SearchCategory::Albums;
				}
				return std::holds_alternative<Track>(item)
					? SearchCategory::Tracks
					: SearchCategory::Artists;
			}();
			available[int(category)] = true;
		}
		setCatalogFilterAvailability(available);
	}

	void openCatalogCollection(Album album) {
		openHomeAlbum(std::move(album));
	}

	void setResultTrackLinks(not_null<Row*> row, QStringView hiddenArtist = {}) {
		row->setTextLinks(
			crl::guard(this, [=](Track track) {
				openHomeTrack(std::move(track));
			}),
			crl::guard(this, [=](const Track &track, const QString &name) {
				openTrackArtist(track, name);
			}),
			hiddenArtist);
	}

	void showArtistProfile(ArtistInfo artist, Track track = {}) {
		openHomeArtist(std::move(artist), std::move(track));
	}

	void openTrackArtist(const Track &track, const QString &name) {
		const auto generation = ++_artistLinkGeneration;
		const auto externalTrack = IsTelegramTrack(track)
			|| track.id.startsWith(u"yt:"_q);
		if (_tab == Tab::Home && _homeArtistOpen
			&& !_homeArtistProfile.id.isEmpty()
			&& name.simplified().compare(
				_homeArtistProfile.name.simplified(), Qt::CaseInsensitive) == 0
			&& track.artistId == _homeArtistProfile.id) {
			return;
		}
		const auto tab = _tab;
		const auto searchGeneration = _searchGen;
		const auto homeGeneration = _homeAlbumGeneration;
		const auto done = crl::guard(this, [=, seed = track](ArtistInfo artist) {
			if (generation != _artistLinkGeneration || tab != _tab
				|| searchGeneration != _searchGen
				|| homeGeneration != _homeAlbumGeneration) {
				return;
			}
			if (artist.id.isEmpty()) {
				if (!externalTrack || seed.id.startsWith(u"yt:"_q)) {
					_controller->showToast(PlaylistText("Профиль «%1» не найден").arg(name));
					return;
				}
				artist = { {}, name, seed.artistArtworkUrl, {} };
			}
			showArtistProfile(std::move(artist), seed);
		});
		if (track.id.startsWith(u"yt:"_q)) {
			if (track.artistId.startsWith(u"yt:"_q)
				&& TrackArtistNames(track).size() == 1) {
				done(ArtistInfo{ track.artistId, name, track.artistArtworkUrl, {} });
			} else {
				ResolveYouTubeMusicArtist(name, done);
			}
		} else {
			ResolveTrackArtist(track, name, done);
		}
	}

	void chooseTrackArtist(const Track &track) {
		const auto names = TrackArtistNames(track);
		if (names.isEmpty()) {
			return;
		} else if (names.size() == 1) {
			openTrackArtist(track, names.front());
			return;
		}
		_trackArtistsMenu = base::make_unique_q<Ui::PopupMenu>(this, st::popupMenuWithIcons);
		for (const auto &name : names) {
			_trackArtistsMenu->addAction(name, [=, seed = track] {
				openTrackArtist(seed, name);
			}, nullptr);
		}
		_trackArtistsMenu->popup(QCursor::pos());
	}

	void addCatalogArtist(const ArtistInfo &artist, MusicTrackList *results = nullptr) {
		const auto featured = (results == nullptr);
		auto play = Fn<void(not_null<Ui::RoundButton*>)>();
		if (featured) {
			play = [=](not_null<Ui::RoundButton*> button) {
				button->setDisabled(true);
				button->setText(rpl::single(PlaylistText("Загрузка…")));
				UserTracks(artist.id, 20, crl::guard(button, [=](std::vector<Track> tracks) {
					if (tracks.empty()) {
						button->setText(rpl::single(PlaylistText("Нет треков")));
						return;
					}
					button->setText(rpl::single(PlaylistText("Слушать")));
					button->setDisabled(false);
					QueueState().results = std::move(tracks);
					PlayFromResults(0);
				}));
			};
		}
		const auto list = featured ? _tabList : results;
		const auto side = style::ConvertScale(BoxParts::kOuterSide);
		list->add(object_ptr<CatalogEntityRow>(
			list,
			artist.name,
			PlaylistText("Исполнитель"),
			QString(),
			artist.avatarUrl,
			CatalogArtwork::Artist,
			crl::guard(this, [=] {
				showArtistProfile(artist);
			}),
			std::move(play)), featured ? QMargins(side, 0, side, style::ConvertScale(kMusicListGap)) : QMargins());
	}

	void fillCatalogResults() {
		const auto mixed = _searchCategory == SearchCategory::All;
		const auto query = _searchField->text().trimmed();
		auto featured = static_cast<const ArtistInfo*>(nullptr);
		if (mixed) {
			featured = std::get_if<ArtistInfo>(&_catalogItems.front());
			if (!featured) {
				for (const auto &item : _catalogItems) {
					const auto artist = std::get_if<ArtistInfo>(&item);
					if (artist && artist->name.trimmed().compare(query, Qt::CaseInsensitive) == 0) {
						featured = artist;
						break;
					}
				}
			}
			if (featured) {
				addCatalogArtist(*featured);
			}
		}

		auto playable = std::make_shared<std::vector<Track>>();
		for (const auto &item : _catalogItems) {
			if (const auto track = std::get_if<Track>(&item)) {
				playable->push_back(*track);
			}
		}
		QueueState().results = *playable;
		auto index = 0;
		if (featured && _catalogItems.size() == 1) {
			return;
		}
		const auto list = AddMusicTrackList(_tabList);
		for (const auto &item : _catalogItems) {
			if (const auto track = std::get_if<Track>(&item)) {
				const auto row = list->add(object_ptr<Row>(
					list, _controller, *track, index, RowMode::Results, playable, index));
				setResultTrackLinks(row);
				++index;
			} else if (const auto album = std::get_if<Album>(&item)) {
				const auto type = [&] {
					switch (album->releaseKind) {
					case ReleaseKind::Single: return PlaylistText("Сингл");
					case ReleaseKind::Ep: return PlaylistText("EP");
					case ReleaseKind::Compilation: return PlaylistText("Сборник");
					case ReleaseKind::Album: return PlaylistText("Альбом");
					case ReleaseKind::Unknown: return PlaylistText("Плейлист");
					}
					return QString();
				}();
				const auto playlist = (album->releaseKind == ReleaseKind::Unknown);
				const auto row = list->add(object_ptr<CatalogEntityRow>(
					list,
					album->title,
					album->artist,
					type,
					album->artworkUrl,
					playlist ? CatalogArtwork::Playlist : CatalogArtwork::Release,
					crl::guard(this, [=, album = *album] { openCatalogCollection(album); })));
				if (!playlist) {
					row->setAlbumBackdrop(*album);
					InstallAlbumContextMenu(row, _controller, *album);
				}
			} else if (const auto artist = std::get_if<ArtistInfo>(&item); artist && artist != featured) {
				addCatalogArtist(*artist, list);
			}
		}
	}

	void runSearch() {
		_searchTimer.stop();
		const auto query = _searchField->text().trimmed();
		const auto generation = ++_searchGen;
		_searchPending = !query.isEmpty() || (_searchChats && _searchChat);
		_searchIndicator->setLoading(_searchPending);
		_searchFailed = false;
		_searchTracks.clear();
		_catalogItems.clear();
		_chatSearch = nullptr;
		_searchStatus.clear();
		const auto finish = [=](bool empty, QString error) {
			_searchPending = false;
			_searchIndicator->setLoading(false);
			_searchFailed = !error.isEmpty();
			_searchStatus = !error.isEmpty()
				? error
				: empty
				? (query.isEmpty()
					? PlaylistText("Ничего не найдено")
					: PlaylistText("Ничего не найдено по запросу «%1»").arg(query))
				: QString();
			if (_tab == Tab::Search) {
				rebuildTab();
			}
		};
		if (_tab == Tab::Search) {
			rebuildTab();
			_tabScroll->scrollToY(0);
		}
		if (!_searchPending) {
			return;
		}
		if (!_searchChats) {
			SearchCatalog(query, _searchCategory, crl::guard(this, [=](
					std::vector<CatalogItem> items,
					QString error) {
				if (generation == _searchGen) {
					_catalogItems = std::move(items);
					if (_searchCategory == SearchCategory::All) {
						updateCatalogFilters();
					}
					finish(_catalogItems.empty(), std::move(error));
				}
			}));
		} else if (const auto session = QueueState().session.get()) {
			_chatSearch = std::make_unique<ChatMusicSearch>(
				session,
				crl::guard(this, [=](std::vector<Track> tracks, QString error) {
					if (generation == _searchGen) {
						_searchTracks = std::move(tracks);
						finish(_searchTracks.empty(), std::move(error));
					}
				}),
				_searchChat);
			_chatSearch->search(query);
		} else {
			finish(true, PlaylistText("Поиск по чатам недоступен"));
		}
	}

	[[nodiscard]] QRectF expandedCoverRect(const Layout &layout) const {
		const auto scale = 0.94 + 0.06 * _playAnim.value(_isPlaying ? 1. : 0.);
		const auto rect = QRectF(layout.cover);
		const auto size = rect.size() * scale;
		return QRectF(rect.center() - QPointF(size.width(), size.height()) / 2., size);
	}
	[[nodiscard]] double barCoverRadius(const Layout &layout) const {
		return style::ConvertScale(kBarRadius) - (layout.barCover.y() - layout.bar.y());
	}
	bool prepareCoverTransition(const Layout &layout) {
		if (layout.cover.isEmpty() || layout.barCover.isEmpty()) {
			return false;
		}
		const auto scale = 0.94 + 0.06 * _playAnim.value(_isPlaying ? 1. : 0.);
		_coverTransition.prepare(
			QRectF(layout.barCover),
			barCoverRadius(layout),
			expandedCoverRect(layout),
			style::ConvertScale(22) * scale);
		return true;
	}
	void startCoverTransition(bool expanding) {
		const auto running = _coverMorph.animating();
		_coverExpanding = expanding;
		if (!PlayerMotionEnabled()
			|| (!running && !prepareCoverTransition(computeLayout()))) {
			_coverMorph.stop();
			updateCoverTransition();
			return;
		}
		// Same geometry and progress in both directions; only the target flips.
		_coverMorph.start(expanding ? 0. : 1., expanding ? 1. : 0., kSpatial);
		_coverOverlay->setGeometry(rect());
		if (_coverMorph.animating()) {
			_coverOverlay->show();
			_coverOverlay->raise();
		}
		updateCoverTransition();
	}
	void updateCoverTransition() {
		if (_coverMorph.animating()) {
			_coverTransition.update(_coverMorph.unitValue(_coverExpanding ? 1. : 0.));
			_coverOverlay->update();
		} else {
			_coverOverlay->hide();
			if (!_coverExpanding && _tab == Tab::Now) {
				setTab(Tab::Home);
				_plainAnim.stop();
				_playbackBar->update();
				if (_tab == Tab::Home && PlayerMotionEnabled()) {
					const auto effect = new QGraphicsOpacityEffect(_tabScroll);
					effect->setOpacity(0.);
					_tabScroll->setGraphicsEffect(effect);
					_homeEntrance.start(0., 1., kEffects);
				}
			}
		}
		update();
	}
	void openNowPlayingFromBar() {
		setTab(Tab::Now);
		setHover(InfoHover::None);
		startCoverTransition(true);
	}
	void animateHomeBackExit() {
		if (!_homeBack->isVisible() || anim::Disabled()
			|| PowerSaving::On(PowerSaving::kAnimations)) {
			return;
		}
		const auto image = Ui::GrabWidget(_homeBack);
		if (image.isNull()) {
			return;
		}
		const auto overlay = _homeBackOverlay.emplace(this);
		overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
		overlay->setGeometry(_homeBack->geometry());
		overlay->paintOn([=](QPainter &p) {
			const auto hq = PainterHighQualityEnabler(p);
			const auto progress = _homeBackExit.unitValue(1.);
			const auto center = QRectF(overlay->rect()).center();
			const auto scale = 1. - 0.03 * progress;
			p.setOpacity(1. - progress);
			p.translate(center);
			p.scale(scale, scale);
			p.translate(-center);
			p.drawPixmap(0, 0, image);
		});
		_homeBackExit.start(0., 1., kFastEffects);
		overlay->show();
		overlay->raise();
	}
	void stopHomeBackExit() {
		_homeBackExit.stop();
		_homeBackOverlay.reset();
	}
	void resetHomeNavigation() {
		clearHomeSideGlow();
		_homeSection = 0;
		_homeAlbumOpen = false;
		_homeArtistOpen = false;
		_homeReturnToArtist = false;
		_homeBackToSearch = false;
		++_artistLinkGeneration;
		++_homeAlbumGeneration;
	}
	struct NavigationPage {
		Tab tab;
		int scroll = 0;
		int homeSection = 0;
		bool homeBackToSearch = false;
		int searchReturnScroll = 0;
		QString playlist;
		std::variant<std::monostate, Track, Album, std::pair<ArtistInfo, Track>> detail;
	};

	[[nodiscard]] NavigationPage navigationPage() const {
		auto page = NavigationPage{
			_tab, _tabScroll->scrollTop(), _homeSection,
			_homeBackToSearch, _searchReturnScroll, _openedPlaylist, {},
		};
		if (_tab == Tab::Home && _homeAlbumOpen) {
			if (_homeArtistOpen) {
				page.detail = std::pair(_homeArtistProfile, _homeArtistSeed);
			} else if (_homeAlbumIsTrack && !_homeAlbumTracks.empty()) {
				page.detail = _homeAlbumTracks.front();
			} else {
				page.detail = _homeAlbum;
			}
		}
		return page;
	}

	void rememberNavigation() {
		if (_restoringNavigation) return;
		_navigationBack.push_back(navigationPage());
		_navigationForward.clear();
		if (_navigationBack.size() > 64) {
			_navigationBack.erase(_navigationBack.begin());
		}
	}

	void stepNavigation(bool back) {
		auto &from = back ? _navigationBack : _navigationForward;
		auto &to = back ? _navigationForward : _navigationBack;
		if (from.empty()) return;
		to.push_back(navigationPage());
		auto page = std::move(from.back());
		from.pop_back();

		_restoringNavigation = true;
		setTab(page.tab);
		if (page.tab == Tab::Home) {
			resetHomeNavigation();
			_homeSection = page.homeSection;
			_homeBackToSearch = page.homeBackToSearch;
			_searchReturnScroll = page.searchReturnScroll;
			if (auto track = std::get_if<Track>(&page.detail)) {
				openHomeTrack(std::move(*track));
			} else if (auto album = std::get_if<Album>(&page.detail)) {
				openHomeAlbum(std::move(*album));
			} else if (auto artist = std::get_if<std::pair<ArtistInfo, Track>>(&page.detail)) {
				openHomeArtist(std::move(artist->first), std::move(artist->second));
			} else {
				relayout();
				rebuildTab();
			}
		} else if (page.tab == Tab::Playlists && _openedPlaylist != page.playlist) {
			_openedPlaylist = std::move(page.playlist);
			rebuildTab();
		}
		_tabScroll->scrollToY(page.scroll);
		_restoringNavigation = false;
	}

	void setTab(Tab tab, bool record = true) {
		if (_coverMorph.animating()) {
			if (tab == Tab::Now) {
				startCoverTransition(true);
				return;
			}
			// Other navigation still takes effect synchronously for its callers.
			_coverMorph.stop();
			_coverOverlay->hide();
		}
		if (_tab == tab) {
			if (tab == Tab::Library) {
				syncLibrary();
			}
			return;
		}
		if (record) rememberNavigation();
		stopHomeBackExit();
		const auto from = _tab;
		const auto animateTransition = _tab != Tab::Now
			&& tab != Tab::Now
			&& PlayerMotionEnabled();
		const auto oldLayout = computeLayout();
		const auto transitionTop = oldLayout.center.isEmpty()
			? oldLayout.lyrics.y()
			: oldLayout.center.y();
		const auto transitionArea = QRect(
			0,
			transitionTop,
			width(),
			std::max(oldLayout.contentBottom - transitionTop, 0));
		auto was = animateTransition
			? Ui::GrabWidget(this, transitionArea)
			: QPixmap();
		clearTabTransition();
		_homeEntrance.stop();
		_tabScroll->setGraphicsEffect(nullptr);
		_plainAnim.start(_tab == Tab::Now ? 0. : 1., tab == Tab::Now ? 0. : 1., kEffects);
		const auto navFrom = double(int(_tab));
		_tab = tab;
		updatePlaybackBarColors();
		_backdropDirty = true;
		updateAmbientTimer();
		if (oldLayout.verticalNav) {
			_navSlide.stop();
		} else {
			_navSlide.start(navFrom, double(int(tab)), kFastSpatial);
		}
		update(computeLayout().nav);
		resetHomeNavigation();
		for (auto i = 0; i != int(_nav.size()); ++i) {
			_nav[i]->setActive(i == int(tab));
		}
		if (tab == Tab::Library) {
			syncLibrary();
		} else if (tab == Tab::Playlists) {
			syncPlaylists();
		}
		rebuildTab();
		auto event = QResizeEvent(size(), size());
		resizeEvent(&event);
		refresh();
		if (animateTransition && !was.isNull()) {
			_tabTransitionWas = std::move(was);
			_tabTransitionNow = Ui::GrabWidget(this, transitionArea);
			if (!_tabTransitionNow.isNull()) {
				_tabTransitionDirection = (int(tab) > int(from)) ? 1 : -1;
				_tabTransition->setGeometry(transitionArea);
				_tabTransition->show();
				_tabTransition->raise();
				_queuePanel->raise();
				_tabTransitionSlide.start(0., 1., kFastSpatial);
				_tabTransitionFade.start(0., 1., kSlowEffects);
			}
		}
	}
	void clearTabTransition() {
		_tabTransitionSlide.stop();
		_tabTransitionFade.stop();
		_tabTransition->hide();
		_tabTransitionWas = QPixmap();
		_tabTransitionNow = QPixmap();
	}
	void updateTabTransition() {
		if (_tabTransitionSlide.animating() || _tabTransitionFade.animating()) {
			_tabTransition->update();
		} else {
			clearTabTransition();
		}
	}
	void syncLibrary() {
		if (!Account::IsLoggedIn()
			|| _librarySyncLoading
			|| (_librarySyncSucceeded
				&& crl::now() - _librarySyncSucceeded < 15 * 1000)) {
			return;
		}
		_librarySyncLoading = true;
		Account::SyncLikes(crl::guard(this, [=](int, int, QString error) {
			_librarySyncLoading = false;
			if (error.isEmpty()) {
				_librarySyncSucceeded = crl::now();
			} else if (_tab == Tab::Library) {
				_controller->showToast(error);
			}
		}));
	}
	void syncPlaylists() {
		if (!Account::IsLoggedIn()
			|| _playlistsSyncLoading
			|| (_playlistsSyncSucceeded
				&& crl::now() - _playlistsSyncSucceeded < 15 * 1000)) {
			return;
		}
		_playlistsSyncLoading = true;
		Account::SyncPlaylists(crl::guard(this, [=](int, int, QString error) {
			_playlistsSyncLoading = false;
			if (error.isEmpty() && Account::IsLoggedIn()) {
				_playlistsSyncSucceeded = crl::now();
			} else if (!Account::IsLoggedIn()) {
				_playlistsSyncSucceeded = 0;
			} else if (_tab == Tab::Playlists) {
				_controller->showToast(error);
			}
			if (_tab == Tab::Playlists) {
				scheduleTabRebuild();
			}
		}));
	}
	void loadHomeAlbums(int page) {
		if (page < 0 || page >= int(_homePages.size())) {
			return;
		}
		auto artist = QString();
		for (const auto &track : _homePages[page].tracks) {
			const auto candidate = track.artist.trimmed();
			if (!candidate.isEmpty() && _homeArtists.insert(candidate.toCaseFolded()).second) {
				artist = candidate;
				break;
			}
		}
		if (artist.isEmpty()) {
			return;
		}
		_homePages[page].artist = artist;
		SearchAlbums(artist, 12, crl::guard(this, [=](std::vector<Album> albums, QString error) {
			if (page >= int(_homePages.size())) {
				return;
			}
			for (auto &album : albums) {
				if (!album.id.isEmpty() && _homeAlbumIds.insert(album.id).second) {
					_homePages[page].albums.push_back(std::move(album));
				}
			}
			if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen) {
				rebuildTab();
			}
		}));
	}

	void loadHomeSoundCloudSections() {
		if (_homeSoundCloudLoading || _homeSoundCloudLoaded
			|| _homeSection || _homeAlbumOpen) {
			return;
		}
		_homeSoundCloudLoading = true;
		_homeSoundCloudError.clear();
		const auto generation = ++_homeSoundCloudGeneration;
		const auto pending = std::make_shared<int>(2);
		const auto finished = [=] {
			if (generation != _homeSoundCloudGeneration) {
				return;
			}
			if (--*pending) {
				return;
			}
			_homeSoundCloudLoading = false;
			_homeSoundCloudLoaded = true;
			if (_homeTrending.empty()
				&& _homeWeeklyWave.id.isEmpty()
				&& _homeDailyDrops.id.isEmpty()
				&& _homeSoundCloudError.isEmpty()) {
				_homeSoundCloudError = QString::fromUtf8(Mokagram::Lang::Tr(
					"SoundCloud не вернул подборки для этого аккаунта."));
			}
			if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen) {
				rebuildTab();
			}
		};
		LoadDiscoverPlaylists(crl::guard(this, [=](
				std::vector<Album> albums,
				QString error) {
			if (generation != _homeSoundCloudGeneration) {
				return;
			}
			if (!error.isEmpty()) {
				_homeSoundCloudError = error;
			}
			for (auto &album : albums) {
				const auto title = album.title.simplified().toCaseFolded();
				if (title == u"weekly wave"_q) {
					_homeWeeklyWave = std::move(album);
				} else if (title == u"daily drops"_q) {
					_homeDailyDrops = std::move(album);
				}
			}
			finished();
		}));
		LoadTrending(24, crl::guard(this, [=](
				std::vector<Track> tracks,
				QString error) {
			if (generation != _homeSoundCloudGeneration) {
				return;
			}
			if (!error.isEmpty() && _homeSoundCloudError.isEmpty()) {
				_homeSoundCloudError = error;
			}
			_homeTrending = std::move(tracks);
			finished();
		}));
	}

	[[nodiscard]] QColor homeArtistBannerColor() {
		return (_tab == Tab::Home && _homeArtistOpen && _homeAlbumHeader)
			? _homeAlbumHeader->artistBannerColor()
			: QColor();
	}

	[[nodiscard]] QColor homeDetailAccent() {
		const auto bannerColor = homeArtistBannerColor();
		return bannerColor.isValid()
			? bannerColor
			: ((_tab == Tab::Home && _homeAlbumOpen) ? _homeAccent : QColor());
	}

	[[nodiscard]] NavColors navColors() {
		const auto plainTrack = st::windowBgOver->c;
		const auto light = IsLightMusicTheme();
		const auto plainSelected = light
			? MusicSelectionColor()
			: st::windowActiveTextFg->c;
		const auto detail = homeDetailAccent();
		const auto accent = detail.isValid() ? detail : _accent;
		if (!accent.isValid()) {
			return { plainTrack, plainSelected };
		}
		const auto plain = detail.isValid() ? 0. : _plainAnim.unitValue(_tab == Tab::Now ? 0. : 1.);
		return {
			anim::color(TintedNavTrack(accent), plainTrack, plain),
			light ? plainSelected : anim::color(accent, plainSelected, plain),
		};
	}

	[[nodiscard]] QColor headerControlBackground() {
		const auto accent = homeDetailAccent();
		return accent.isValid() ? TintedNavTrack(accent) : st::windowBgOver->c;
	}

	[[nodiscard]] QColor homeSideGlowColor() const {
		if (!_homeSideGlowFade.animating()) {
			return _homeSideGlow;
		}
		auto from = _homeSideGlowFrom;
		auto to = _homeSideGlow;
		if (!from.alpha()) {
			from = to;
			from.setAlpha(0);
		} else if (!to.alpha()) {
			to = from;
			to.setAlpha(0);
		}
		return anim::color(from, to, _homeSideGlowFade.value(1.));
	}

	void repaintHomeSideGlow() {
		const auto side = HomeSideGlowWidth(width());
		const auto top = _tabScroll->y();
		update(QRect(0, top, side, height() - top));
		update(QRect(width() - side, top, side, height() - top));
	}

	void setHomeSideGlow(QColor accent) {
		const auto target = accent.isValid() ? accent : QColor(Qt::transparent);
		if (_homeSideGlow == target) {
			return;
		}
		_homeSideGlowFrom = homeSideGlowColor();
		_homeSideGlow = target;
		_homeSideGlowFade.stop();
		if (isVisible() && PlayerMotionEnabled()
			&& HomeSideGlowWidth(width()) > style::ConvertScale(kHomeSideGlowMin)
			&& Mokagram::Settings::Bool(u"player.cover_colors"_q)
			&& Mokagram::Settings::Bool(u"player.dynamic_background"_q)) {
			_homeSideGlowFade.start(
				[=] { repaintHomeSideGlow(); },
				0., 1., 250, anim::easeOutCubic);
		}
		updateAmbientTimer();
	}

	void clearHomeSideGlow() {
		_homeSideGlowFade.stop();
		_homeSideGlow = Qt::transparent;
	}

	void loadHomeAccent(const QString &url, int generation) {
		_homeAccent = QColor();
		update();
		LoadImage(ArtUrl(url, u"t500x500"_q), crl::guard(this, [=](QImage image) {
			if (generation != _homeAlbumGeneration
				|| (_homeArtistOpen && _homeAlbum.artworkUrl != url)) {
				return;
			}
			_homeAccent = ComputeAccent(image);
			setHomeSideGlow(_homeAccent);
			_queuePanel->update();
			update();
		}));
	}

	void mergeHomeArtistAlbum(Album album) {
		const auto nativeYouTube = _homeArtistProfile.id.startsWith(u"yt:"_q);
		const auto key = [](const QString &title) {
			return title.normalized(QString::NormalizationForm_C).simplified().toCaseFolded();
		};
		const auto titleKey = key(album.title);
		const auto i = std::ranges::find_if(_homeArtistAlbums, [&](const Album &other) {
			return other.id == album.id || (other.releaseKind == album.releaseKind
				&& key(other.title) == titleKey
				&& (!other.releaseYear || !album.releaseYear
					|| other.releaseYear == album.releaseYear));
		});
		if (i == _homeArtistAlbums.end()) {
			_homeArtistAlbums.push_back(std::move(album));
		} else if (i->id.startsWith(u"yt:"_q) != nativeYouTube
			&& album.id.startsWith(u"yt:"_q) == nativeYouTube) {
			*i = std::move(album);
		}
	}

	void loadHomeArtistYouTube(int generation) {
		if (!_homeArtistOpen || _homeArtistProfile.name.isEmpty()) {
			return;
		}
		const auto nativeYouTube = _homeArtistProfile.id.startsWith(u"yt:"_q);
		_homeArtistYouTubeDeferred = !nativeYouTube
			&& Settings::Bool(u"player.hide_youtube_fallback"_q);
		if (_homeArtistYouTubeDeferred || _homeArtistYouTubeLoading) {
			return;
		}
		_homeArtistYouTubeLoading = true;
		const auto load = crl::guard(this, [=](ArtistInfo artist) {
			if (generation != _homeAlbumGeneration || !_homeArtistOpen) return;
			if (artist.id.isEmpty()) {
				_homeArtistYouTubeLoading = false;
				return;
			}
			LoadYouTubeMusicArtistCatalog(artist.id, crl::guard(this, [=](
					std::vector<Track> tracks, std::vector<Album> albums, QString error) {
				if (generation != _homeAlbumGeneration || !_homeArtistOpen) return;
				_homeArtistYouTubeLoading = false;
				if (nativeYouTube) {
					_homeArtistTracks = std::move(tracks);
				} else {
					_homeArtistYouTubeTracks = std::move(tracks);
				}
				for (auto &album : albums) {
					mergeHomeArtistAlbum(std::move(album));
				}
				if (_tab == Tab::Home) {
					if (!error.isEmpty()) _controller->showToast(error);
					scheduleTabRebuild();
				}
			}));
		});
		if (nativeYouTube) {
			load(_homeArtistProfile);
		} else {
			ResolveYouTubeMusicArtist(_homeArtistProfile.name, load);
		}
	}

	void loadHomeArtist(
			const QString &name,
			const QString &artistId,
			const QString &trackId,
			const QString &albumId,
			int generation) {
		_homeArtistAlbums.clear();
		_homeArtistTracks.clear();
		_homeArtistYouTubeTracks.clear();
		_homeArtistYouTubeDeferred = false;
		_homeArtistYouTubeLoading = false;
		if (_homeArtistOpen) {
			loadHomeArtistYouTube(generation);
		}
		if (artistId.startsWith(u"yt:"_q) || albumId.startsWith(u"yt:"_q)) {
			return;
		}
		if (_homeArtistOpen && !_homeArtistSeed.id.isEmpty()) {
			_homeArtistTracks.push_back(_homeArtistSeed);
		}
		const auto artistPage = _homeArtistOpen;
		const auto searchAlbums = [=] {
			if (name.isEmpty()) return;
			SearchAlbums(name, 50, crl::guard(this, [=](std::vector<Album> albums, QString) {
				if (generation != _homeAlbumGeneration) {
					return;
				}
				const auto artist = name.simplified().toCaseFolded();
				for (auto &album : albums) {
					const auto matchesId = !artistId.isEmpty() && artistId != u"0"_q
						&& artistId == album.artistId;
					const auto matchesName = album.artist.simplified().toCaseFolded() == artist;
					if ((!matchesId && !matchesName) || album.id == albumId) {
						continue;
					}
					mergeHomeArtistAlbum(std::move(album));
				}
				if (_tab == Tab::Home && _homeAlbumOpen) {
					rebuildTab();
				}
			}));
		};
		if (artistPage && !artistId.isEmpty()) {
			LoadArtistAlbums(artistId, crl::guard(this, [=](std::vector<Album> albums) {
				if (generation != _homeAlbumGeneration) {
					return;
				}
				for (auto &album : albums) {
					mergeHomeArtistAlbum(std::move(album));
				}
				if (_tab == Tab::Home && !_homeArtistAlbums.empty()) rebuildTab();
				searchAlbums();
			}));
		} else {
			searchAlbums();
		}
		if (!artistId.isEmpty()) {
			UserTracks(artistId, artistPage ? 0 : 12, crl::guard(this, [=](std::vector<Track> tracks) {
				if (generation != _homeAlbumGeneration) {
					return;
				}
				if (!_homeArtistOpen) {
					std::erase_if(tracks, [&](const Track &other) {
						return other.id == trackId;
					});
				}
				const auto hasPlayable = std::ranges::any_of(tracks, CanPlayTrack);
				if (_homeArtistOpen) {
					if (!hasPlayable && !_homeArtistSeed.id.isEmpty()
						&& std::ranges::none_of(tracks, [&](const Track &track) {
							return track.id == _homeArtistSeed.id;
						})) {
						tracks.push_back(_homeArtistSeed);
					}
					std::stable_sort(tracks.begin(), tracks.end(), [](const Track &a, const Track &b) {
						return a.playbackCount > b.playbackCount;
					});
				}
				_homeArtistTracks = std::move(tracks);
				if (_tab == Tab::Home && _homeAlbumOpen) {
					rebuildTab();
				}
			}), _homeArtistOpen);
		}
	}

	// Похожее по звучанию: жанр и слова из названия трека, а не его автор.
	void loadHomeSimilar(const Track &seed, int generation) {
		_homeSimilarTracks.clear();
		_homeSimilarLoading = true;
		auto avoid = std::vector<Track>();
		avoid.push_back(seed);
		for (const auto &track : _homeAlbumTracks) {
			avoid.push_back(track);
		}
		for (const auto &track : _homeArtistTracks) {
			avoid.push_back(track);
		}
		for (const auto &track : _homeArtistYouTubeTracks) {
			avoid.push_back(track);
		}
		BuildSimilar(seed, avoid, 24, crl::guard(this, [=](std::vector<Track> tracks, QString) {
			if (generation != _homeAlbumGeneration) {
				return;
			}
			_homeSimilarLoading = false;
			_homeSimilarTracks = std::move(tracks);
			if (_tab == Tab::Home && _homeAlbumOpen) rebuildTab();
		}));
	}

	void updateHomeArtistBar() {
		const auto home = (_tab == Tab::Home);
		const auto artist = home && _homeArtistOpen && _homeAlbumHeader;
		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto height = s(56);
		const auto scrollTop = _tabScroll->scrollTop();
		const auto remaining = artist ? _homeAlbumHeader->height() - scrollTop : 2 * height;
		const auto ratio = std::clamp((2 * height - remaining) / double(height), 0., 1.);
		const auto opacity = ratio * ratio * (3. - 2. * ratio);
		if (artist) {
			_homeAlbumHeader->setArtistIdentityOpacity(1. - opacity);
		}
		const auto viewport = _tabScroll->viewport();
		const auto barHeight = (opacity > 0.) ? height + s(kMusicListGap) : 0;
		_homeArtistBar->setVisible(barHeight > 0);
		if (barHeight > 0) {
			_homeArtistBar->setGeometry(QRect(
				viewport->mapTo(this, QPoint()), QSize(viewport->width(), barHeight)));
			_homeArtistBarOpacity->setOpacity(opacity);
			_homeArtistBar->raise();
			_homeArtistBar->update();
		}
		if (showsQueue()) {
			_homeScrollFadeKey = {};
			updateQueueEdgeFade(_tabScroll, _homeScrollFade);
			return;
		}
		updateHomeScrollFade(home && scrollTop > 0 ? qRound(barHeight * opacity) : -1);
	}

	void updateHomeScrollFade(int top) {
		const auto viewport = _tabScroll->viewport();
		const auto size = viewport->size();
		const auto depth = style::ConvertScale(28);
		const auto feather = style::ConvertScale(16);
		const auto bar = _playbackBar->geometry().translated(-viewport->mapTo(this, QPoint()));
		const auto fadeBounds = bar.marginsAdded(QMargins(feather, feather, feather, feather));
		const auto bottom = (_tab == Tab::Home && _hasPlaybackTrack && bar.width() > 0
			&& _tabScroll->scrollTop() < _tabScroll->scrollTopMax()
			&& fadeBounds.intersects(viewport->rect()))
			? fadeBounds
			: QRect();
		const auto enabled = !size.isEmpty() && (top >= 0 || !bottom.isEmpty());
		_homeScrollFade->setEnabled(enabled);
		if (!enabled) return;

		const auto key = std::tuple(size, top, bottom, depth, feather);
		if (_homeScrollFadeKey == key) return;
		_homeScrollFadeKey = key;
		auto topFade = QLinearGradient(0., top, 0., top + depth);
		topFade.setColorAt(0., Qt::transparent);
		topFade.setColorAt(1., Qt::black);
		if (bottom.isEmpty()) {
			_homeScrollFade->setOpacityMask(QBrush(topFade));
			return;
		}

		auto cutout = QImage(bottom.size(), QImage::Format_ARGB32_Premultiplied);
		cutout.fill(Qt::transparent);
		{
			auto p = QPainter(&cutout);
			auto hq = PainterHighQualityEnabler(p);
			p.setCompositionMode(QPainter::CompositionMode_Source);
			const auto capsule = QRectF(bar.translated(-bottom.topLeft()));
			const auto radius = style::ConvertScale(kBarRadius);
			const auto layers = 16;
			for (auto i = 1; i <= layers; ++i) {
				const auto ratio = i / double(layers);
				const auto spread = feather * (1. - ratio);
				const auto alpha = ratio * ratio * (3. - 2. * ratio);
				p.fillPath(
					Ui::SquirclePath(capsule.adjusted(-spread, -spread, spread, spread), radius + spread),
					QColor(0, 0, 0, qRound(255. * alpha)));
			}
		}
		auto mask = QImage(size, QImage::Format_ARGB32_Premultiplied);
		{
			auto p = QPainter(&mask);
			p.setCompositionMode(QPainter::CompositionMode_Source);
			p.fillRect(mask.rect(), top >= 0 ? QBrush(topFade) : QBrush(Qt::black));
			p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
			p.drawImage(bottom.topLeft(), cutout);
		}
		_homeScrollFade->setOpacityMask(QBrush(mask));
	}

	void beginHomeDetail() {
		rememberNavigation();
		const auto fromSearch = (_tab == Tab::Search);
		if (fromSearch) {
			_searchReturnScroll = _tabScroll->scrollTop();
		}
		setTab(Tab::Home, false);
		if (fromSearch) {
			_homeBackToSearch = true;
		}
	}

	void openHomeArtist(ArtistInfo artist, Track track) {
		beginHomeDetail();
		if (!_homeReturnToArtist) {
			_homeArtistFeedScroll = _tabScroll->scrollTop();
			_homeArtistLikesOpen = false;
			_homeArtistVisibleTracks = kArtistTrackPageSize;
			_discographyFilter = DiscographyFilter::All;
			_discographyExpanded = false;
			*_discographyOffset = 0;
		}
		_homeReturnToArtist = false;
		_homeAlbumReturnScroll = _homeArtistFeedScroll;
		_homeArtistOpen = true;
		_homeAlbumOpen = true;
		_homeAlbumIsTrack = false;
		_homeArtistSeed = track;
		_homeArtistProfile = artist;
		const auto generation = ++_homeAlbumGeneration;
		_homeArtistBannerUrl = artist.bannerUrl;
		const auto art = artist.avatarUrl.isEmpty() ? track.artworkUrl : artist.avatarUrl;
		_homeAlbum = { {}, artist.name, {}, art, 0 };
		_homeAlbumTracks.clear();
		_homeAlbumNote.clear();
		_homeAlbumLoading = false;
		loadHomeAccent(art, generation);
		const auto updateArtist = crl::guard(this, [=](ArtistInfo info) {
			if (generation != _homeAlbumGeneration || !_homeArtistOpen) {
				return;
			}
			if (info.id.isEmpty()) {
				return;
			}
			if (!info.avatarUrl.isEmpty() && _homeAlbum.artworkUrl != info.avatarUrl) {
				_homeAlbum.artworkUrl = info.avatarUrl;
				loadHomeAccent(info.avatarUrl, generation);
			}
			if (!info.avatarUrl.isEmpty()) {
				_homeArtistProfile.avatarUrl = info.avatarUrl;
			}
			_homeArtistProfile.bannerUrl = info.bannerUrl;
			_homeArtistBannerUrl = info.bannerUrl;
			if (_tab == Tab::Home) {
				rebuildTab();
			}
		});
		if (artist.id.startsWith(u"yt:"_q)) {
			LoadYouTubeMusicArtistInfo(artist.id, updateArtist);
		} else if (!artist.id.isEmpty()) {
			LoadArtistInfo(artist.id, updateArtist, true);
		}
		loadHomeArtist(artist.name, artist.id, {}, {}, generation);
		if (track.id.isEmpty()) {
			_homeSimilarTracks.clear();
			_homeSimilarLoading = false;
		} else {
			loadHomeSimilar(track, generation);
		}
		syncLibrary();
		syncPlaylists();
		relayout();
		rebuildTab();
		_tabScroll->scrollToY(0);
	}

	void openHomeTrack(Track track) {
		beginHomeDetail();
		_homeArtistOpen = false;
		_homeAlbumReturnScroll = _tabScroll->scrollTop();
		_homeAlbumOpen = true;
		_homeAlbumIsTrack = true;
		const auto generation = ++_homeAlbumGeneration;
		loadHomeAccent(track.artworkUrl, generation);
		loadHomeArtist(track.artist, track.artistId, track.id, {}, generation);
		loadHomeSimilar(track, generation);
		_homeAlbum = { {}, track.title, track.artist, track.artworkUrl, 1 };
		_homeAlbumTracks = { std::move(track) };
		_homeAlbumLoading = false;
		_homeAlbumNote.clear();
		relayout();
		rebuildTab();
		_tabScroll->scrollToY(0);
	}

	void openHomeAlbum(Album album) {
		beginHomeDetail();
		_homeReturnToArtist = _homeArtistOpen;
		_homeArtistOpen = false;
		_homeAlbumIsTrack = false;
		_homeAlbumReturnScroll = _tabScroll->scrollTop();
		_homeAlbumOpen = true;
		relayout();
		_homeAlbum = album;
		RecordRecentAlbum(album);
		loadHomeAlbum();
		_tabScroll->scrollToY(0);
	}

	void loadHomeAlbum() {
		_homeAlbumTracks.clear();
		_homeReleaseArtistAvatar.clear();
		_homeAlbumLoading = true;
		_homeAlbumNote = QString::fromUtf8(Mokagram::Lang::Tr("Загружаю альбом…"));
		const auto generation = ++_homeAlbumGeneration;
		loadHomeAccent(_homeAlbum.artworkUrl, generation);
		loadHomeArtist(_homeAlbum.artist, _homeAlbum.artistId, {}, _homeAlbum.id, generation);
		rebuildTab();
		AlbumTracks(_homeAlbum.id, crl::guard(this, [=](std::vector<Track> tracks, QString error) {
			if (generation != _homeAlbumGeneration) {
				return;
			}
			_homeAlbumLoading = false;
			for (auto &track : tracks) {
				if (track.artworkUrl.isEmpty()) track.artworkUrl = _homeAlbum.artworkUrl;
			}
			_homeAlbumTracks = std::move(tracks);
			const auto artist = std::ranges::find_if(_homeAlbumTracks, [&](const Track &track) {
				return !track.artistArtworkUrl.isEmpty()
					&& (!_homeAlbum.artistId.isEmpty()
						? _homeAlbum.artistId == track.artistId
						: _homeAlbum.artist.trimmed().compare(
							track.artist.trimmed(), Qt::CaseInsensitive) == 0);
			});
			if (artist != _homeAlbumTracks.end()) {
				_homeReleaseArtistAvatar = artist->artistArtworkUrl;
			} else if (_homeAlbum.artistId.startsWith(u"yt:"_q)) {
				LoadYouTubeMusicArtistInfo(_homeAlbum.artistId, crl::guard(this, [=](ArtistInfo info) {
					if (generation != _homeAlbumGeneration || info.avatarUrl.isEmpty()) return;
					_homeReleaseArtistAvatar = std::move(info.avatarUrl);
					if (_tab == Tab::Home && _homeAlbumOpen) scheduleTabRebuild();
				}));
			}
			if (!_homeAlbumTracks.empty() && !_homeAlbumTracks.front().artistId.isEmpty()) {
				const auto artistId = _homeAlbumTracks.front().artistId;
				if (_homeAlbum.artistId.isEmpty()) {
					_homeAlbum.artistId = artistId;
				}
				const auto updateRelated = crl::guard(this, [=](std::vector<Track> related) {
					if (generation != _homeAlbumGeneration) {
						return;
					}
					std::erase_if(related, [&](const Track &track) {
						return std::any_of(_homeAlbumTracks.begin(), _homeAlbumTracks.end(),
							[&](const Track &albumTrack) { return albumTrack.id == track.id; });
					});
					_homeArtistTracks = std::move(related);
					if (_tab == Tab::Home && _homeAlbumOpen) rebuildTab();
				});
				if (artistId.startsWith(u"yt:"_q)) {
					LoadYouTubeMusicArtistCatalog(artistId, crl::guard(this, [=](
							std::vector<Track> related, std::vector<Album> albums, QString) {
						if (generation != _homeAlbumGeneration) return;
						for (auto &album : albums) {
							if (album.id != _homeAlbum.id) {
								mergeHomeArtistAlbum(std::move(album));
							}
						}
						updateRelated(std::move(related));
					}));
				} else {
					UserTracks(artistId, 12, updateRelated);
				}
			}
			_homeAlbumNote = !error.isEmpty() ? error : _homeAlbumTracks.empty()
				? QString::fromUtf8(Mokagram::Lang::Tr("В альбоме нет доступных треков"))
				: QString();
			if (_tab == Tab::Home && _homeAlbumOpen) {
				rebuildTab();
			}
		}));
	}

	void scheduleHomeMoreIfNeeded() {
		if (_homeMoreCheckQueued || _homeMoreLoading || _homeMoreRetryTimer.isActive()
			|| _homePages.empty() || _tab != Tab::Home || _homeSection
			|| _homeAlbumOpen || _tabScroll->height() <= 0
			|| _tabScroll->scrollTopMax() - _tabScroll->scrollTop() > style::ConvertScale(240)) {
			return;
		}
		_homeMoreCheckQueued = true;
		crl::on_main(this, [=] {
			_homeMoreCheckQueued = false;
			if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen
				&& !_homeMoreRetryTimer.isActive() && !_homeMoreLoading
				&& _tabScroll->height() > 0
				&& _tabScroll->scrollTopMax() - _tabScroll->scrollTop() <= style::ConvertScale(240)) {
				loadMoreHome();
			}
		});
	}

	void loadMoreHome() {
		if (_homeMoreLoading || _homePages.empty() || _homeMoreRetryTimer.isActive()) {
			return;
		}
		_homeMoreLoading = true;
		auto context = StreamContext();
		context.avoidPreviousArtists = false;
		for (const auto &page : _homePages) {
			context.avoid.insert(context.avoid.end(), page.tracks.begin(), page.tracks.end());
		}
		if (!context.avoid.empty()) {
			context.current = context.avoid.back();
		}
		BuildStream(std::move(context), 12, crl::guard(this, [=](std::vector<Track> tracks, QString note) {
			_homeMoreLoading = false;
			if (tracks.empty()) {
				_homeMoreFailures = std::min(_homeMoreFailures + 1, 6);
				_homeMoreRetryTimer.start(1000 * (1 << _homeMoreFailures));
			} else {
				_homeMoreFailures = 0;
				_homePages.push_back({ std::move(tracks), {}, {} });
				loadHomeAlbums(int(_homePages.size()) - 1);
			}
			if (_tab == Tab::Home && !_homeSection && !_homeAlbumOpen) {
				rebuildTab();
			}
		}));
	}

	void addHomeDiscography() {
		if (_homeArtistAlbums.empty()) return;

		const auto s = [](int value) { return style::ConvertScale(value); };
		auto releases = _homeArtistAlbums;
		std::erase_if(releases, [&](const Album &release) {
			if (release.id.startsWith(u"yt:"_q)
				&& !_homeArtistProfile.id.startsWith(u"yt:"_q)
				&& Settings::Bool(u"player.hide_youtube_fallback"_q)) {
				return true;
			}
			switch (_discographyFilter) {
			case DiscographyFilter::All: return false;
			case DiscographyFilter::Albums:
				return release.releaseKind != ReleaseKind::Album
					&& release.releaseKind != ReleaseKind::Compilation;
			case DiscographyFilter::SinglesAndEps:
				return release.releaseKind != ReleaseKind::Single
					&& release.releaseKind != ReleaseKind::Ep;
			}
			Unexpected("Discography filter.");
		});
		constexpr auto kPreviewCount = size_t(6);
		const auto generation = _homeAlbumGeneration;
		auto toggle = Fn<void()>();
		if (releases.size() > kPreviewCount) {
			toggle = [=] {
				crl::on_main(this, [=] {
					if (_tab == Tab::Home && generation == _homeAlbumGeneration) {
						_discographyExpanded = !_discographyExpanded;
						rebuildTab();
					}
				});
			};
		}
		_tabList->add(object_ptr<HomeDiscographyBar>(
			_tabList,
			_discographyFilter,
			_discographyExpanded,
			[=](DiscographyFilter filter) {
				if (_discographyFilter == filter) return;
				crl::on_main(this, [=] {
					if (_tab == Tab::Home && generation == _homeAlbumGeneration) {
						_discographyFilter = filter;
						_discographyExpanded = false;
						*_discographyOffset = 0;
						rebuildTab();
					}
				});
			},
			std::move(toggle)), QMargins(s(14), s(20), s(14), s(8)));
		if (releases.empty()) {
			_tabList->add(object_ptr<Ui::FlatLabel>(
				_tabList, PlaylistText("В этой категории нет релизов"), st::boxLabel),
				QMargins(s(14), s(8), s(14), s(8)));
			return;
		}

		if (!_discographyExpanded && releases.size() > kPreviewCount) {
			releases.resize(kPreviewCount);
		}
		_tabList->add(object_ptr<HomeMixedGrid>(
			_tabList, _controller, std::vector<Track>(), releases,
			crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
			nullptr, _discographyExpanded, _discographyOffset));
	}

	[[nodiscard]] QRect searchHintGeometry(const Layout &layout) const {
		const auto areaHeight = std::max(layout.center.bottom() - layout.scrollTop, 0);
		const auto availableHeight = std::max(areaHeight - style::ConvertScale(72), 0);
		const auto width = std::min({
			style::ConvertScale(StickerHint::kVideoMaxWidth),
			std::max(layout.center.width(), 0),
			availableHeight * StickerHint::kVideoWidth / StickerHint::kVideoHeight,
		});
		const auto height = width * StickerHint::kVideoHeight / StickerHint::kVideoWidth;
		const auto top = layout.scrollTop
			+ std::max((areaHeight - height - style::ConvertScale(56)) / 2, 0);
		return QRect(layout.center.center().x() - width / 2, top, width, height);
	}

	void scheduleTabRebuild() {
		if (_tabRebuildPending) {
			return;
		}
		_tabRebuildPending = true;
		crl::on_main(this, [=] {
			if (std::exchange(_tabRebuildPending, false) && !_closing) {
				rebuildTab();
			}
		});
	}

	void rebuildTab() {
		_tabRebuildPending = false;
		_rebuildingTab = true;
		const auto rebuildingGuard = gsl::finally([&] { _rebuildingTab = false; });
		const auto keep = (_rebuiltTab == _tab) ? _tabScroll->scrollTop() : 0;
		_rebuiltTab = _tab;
		_searchResultWidgets.clear();
		_tabQueueList = nullptr;
		_tabList->clear();
		_homeBack->setVisible(_tab == Tab::Home && _homeAlbumOpen);
		_tabNote.clear();
		if (_tab != Tab::Playlists) {
			_playlistsSticker = nullptr;
		}
		const auto searching = (_tab == Tab::Search);
		const auto emptySearch = searching && _searchField->text().trimmed().isEmpty()
			&& !(_searchChats && _searchChat);
		if (!emptySearch) {
			_searchHintMenu.hide();
			_searchEmptySticker = nullptr;
		} else if (!_searchEmptySticker) {
			_searchEmptySticker = base::make_unique_q<StickerHint>(
				this,
				&_controller->session(),
				[=](QPoint global) {
					_searchHintMenu.show(this, this, _controller, global);
				},
				StickerHint::Mode::Fullscreen);
			_searchEmptySticker->setGeometry(searchHintGeometry(computeLayout()));
			_searchEmptySticker->show();
		}
		const auto fresh = searching && !_searchField->isVisible();
		_searchField->setVisible(searching);
		_sourceSwitch->setVisible(searching);
		_chooseSearchChat->setVisible(searching && _searchChats);
		for (auto i = 0; i != int(_searchFilters.size()); ++i) {
			_searchFilters[i]->setVisible(searching && !_searchChats && _searchFilterAvailable[i]);
		}
		_searchRetry->setVisible(searching && _searchFailed);
		if (fresh) {
			_searchField->setFocus();
		}
		const auto tr = [](const char *text) {
			return QString::fromUtf8(Mokagram::Lang::Tr(text));
		};
		const auto addRows = [&](const std::vector<Track> &tracks, RowMode mode = RowMode::Results, QString playlistId = {}, int visibleLimit = -1) {
			auto playable = std::make_shared<std::vector<Track>>();
			for (const auto &track : tracks) {
				if (CanPlayTrack(track)) {
					playable->push_back(track);
				}
			}
			QueueState().results = *playable;
			auto resultIndex = 0;
			const auto count = (visibleLimit < 0)
				? int(tracks.size()) : std::min(int(tracks.size()), visibleLimit);
			const auto list = AddMusicTrackList(_tabList);
			const auto hiddenArtist = IsDetailTrackRow(mode)
				? QStringView(_homeArtistOpen ? _homeArtistProfile.name : _homeAlbum.artist).trimmed()
				: QStringView();
			for (auto i = 0; i != count; ++i) {
				const auto index = CanPlayTrack(tracks[i]) ? resultIndex++ : -1;
				const auto row = list->add(object_ptr<Row>(
					list,
					_controller,
					tracks[i],
					i,
					mode,
					playable,
					index,
					nullptr,
					playlistId));
				setResultTrackLinks(row, hiddenArtist);
			}
		};
		switch (_tab) {
		case Tab::Now:
			if (showsQueue()) {
				fillTabQueue();
			}
			break;
		case Tab::Home: {
			const auto &queue = QueueState();
			const auto liked = LikedTracks();
			if (_homeAlbumOpen) {
				const auto count = int(_homeAlbumTracks.size());
				const auto facts = (!_homeArtistOpen && !_homeAlbumIsTrack)
					? HomeReleaseFacts(
						_homeAlbum.releaseYear,
						std::max(count, _homeAlbum.trackCount),
						_homeAlbumTracks,
						_homeAlbumNote.isEmpty())
					: QString();
				const auto avatar = _homeArtistOpen ? QString()
					: (_homeAlbumIsTrack && count > 0) ? _homeAlbumTracks.front().artistArtworkUrl
					: _homeReleaseArtistAvatar;
				auto openArtist = Fn<void()>();
				if (!_homeArtistOpen && (!_homeAlbum.artistId.isEmpty() || count > 0)) {
					openArtist = crl::guard(this, [=] {
						if (!_homeAlbum.artistId.isEmpty()) {
							showArtistProfile({
								_homeAlbum.artistId, _homeAlbum.artist, {}, {} });
							return;
						}
						const auto artistTrack = std::find_if(
							_homeAlbumTracks.begin(),
							_homeAlbumTracks.end(),
							[=](const Track &track) {
								return track.artist.trimmed().compare(
									_homeAlbum.artist.trimmed(), Qt::CaseInsensitive) == 0;
							});
						if (artistTrack != _homeAlbumTracks.end()) {
							chooseTrackArtist(*artistTrack);
						} else if (!_homeAlbumTracks.empty()) {
							chooseTrackArtist(_homeAlbumTracks.front());
						}
					});
				}
				const auto albumHeader = _tabList->add(object_ptr<HomeAlbumHeader>(
					_tabList, _homeAlbum, facts, avatar, _homeAlbumIsTrack,
					crl::guard(this, [=](QImage image, QRect source) {
						source.moveTopLeft(mapFromGlobal(source.topLeft()));
						Ui::CreateChild<HomeCoverPreview>(this, std::move(image), source);
					}), _homeArtistOpen, _homeArtistBannerUrl, std::move(openArtist),
					crl::guard(this, [=] {
						updateHomeArtistBar();
						_queuePanel->update();
						update();
					})), QMargins(0, 0, 0, style::ConvertScale(8)));
				_homeAlbumHeader = albumHeader;
				if (!_homeAlbumIsTrack && !_homeArtistOpen) InstallAlbumContextMenu(albumHeader, _controller, _homeAlbum);

				if (_homeArtistOpen) {
					const auto popular = HomeArtistPopular(
						_homeArtistTracks, _homeArtistYouTubeTracks);
					if (!popular.empty()) {
						_tabList->add(object_ptr<HomeShelfHeader>(_tabList, tr("Популярное")));
						addRows(popular, RowMode::DetailTrack, {}, _homeArtistVisibleTracks);
						const auto count = int(popular.size());
						if (_homeArtistVisibleTracks < count) {
							const auto side = style::ConvertScale(BoxParts::kOuterSide);
							const auto gap = style::ConvertScale(8);
							const auto generation = _homeAlbumGeneration;
							_tabList->add(object_ptr<HomeShelfAction>(
								_tabList,
								tr("Показать ещё"),
								tr("Популярное"),
								[=] {
									crl::on_main(this, [=] {
										if (_tab != Tab::Home || !_homeArtistOpen
											|| generation != _homeAlbumGeneration) return;
										_homeArtistVisibleTracks = std::min(
											_homeArtistVisibleTracks + kArtistTrackPageSize, count);
										rebuildTab();
									});
								}), QMargins(side, gap, side, gap), style::al_right);
						}
					}
					auto artistLikes = std::vector<Track>();
					const auto &artistId = _homeArtistProfile.id;
					const auto artistName = _homeArtistProfile.name.trimmed();
					for (const auto &track : liked) {
						const auto matches = (!artistId.isEmpty() && artistId == track.artistId)
							|| std::ranges::any_of(TrackArtistNames(track), [&](const QString &credit) {
								return credit.compare(artistName, Qt::CaseInsensitive) == 0;
							});
						if (matches) artistLikes.push_back(track);
					}
					auto likedReleases = std::vector<Album>();
					const auto playlists = Playlists();
					for (const auto &album : _homeArtistAlbums) {
						if (std::ranges::any_of(playlists, [&](const Playlist &playlist) {
							return playlist.remote && playlist.sourceId == album.id;
						})) {
							likedReleases.push_back(album);
						}
					}
					if (!artistLikes.empty() || !likedReleases.empty()) {
						auto counts = QStringList();
						if (!artistLikes.empty()) {
							counts.push_back(tr("Треков: %1").arg(artistLikes.size()));
						}
						if (!likedReleases.empty()) {
							counts.push_back(tr("Релизов: %1").arg(likedReleases.size()));
						}
						const auto s = [](int value) { return style::ConvertScale(value); };
						const auto generation = _homeAlbumGeneration;
						_tabList->add(object_ptr<HomeArtistLikesCard>(
							_tabList,
							_homeAlbum.artworkUrl,
							counts.join(u" · "_q),
							_homeArtistLikesOpen,
							crl::guard(this, [=] {
								crl::on_main(this, [=] {
									if (_tab == Tab::Home && generation == _homeAlbumGeneration) {
										_homeArtistLikesOpen = !_homeArtistLikesOpen;
										rebuildTab();
									}
								});
							})), QMargins(s(14), s(20), s(14), s(8)));
						if (_homeArtistLikesOpen) {
							if (!artistLikes.empty()) addRows(artistLikes, RowMode::DetailTrack);
							if (!likedReleases.empty()) {
								_tabList->add(object_ptr<HomeMixedGrid>(
									_tabList, _controller, std::vector<Track>(), likedReleases,
									crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
									nullptr, true, nullptr));
							}
						}
					}
					addHomeDiscography();
					const auto similarArtists = HomeSimilarArtists(
						_homeArtistProfile,
						_homeSimilarTracks);
					if (!similarArtists.empty() || _homeSimilarLoading) {
						_tabList->add(object_ptr<HomeShelfHeader>(
							_tabList, tr("Похожие исполнители")));
						if (!similarArtists.empty()) {
							_tabList->add(object_ptr<HomeMixedGrid>(
								_tabList,
								_controller,
								std::vector<Track>(),
								std::vector<Album>(),
								crl::guard(this, [=](Album album) {
									openHomeAlbum(std::move(album));
								}),
								crl::guard(this, [=](Track track) {
									openHomeTrack(std::move(track));
								}),
								false,
								std::make_shared<int>(0),
								similarArtists,
								crl::guard(this, [=](Track track) {
									chooseTrackArtist(track);
								})));
						} else {
							_tabList->add(object_ptr<HomeLoadingView>(
								_tabList,
								tr("Ищу похожих исполнителей…")));
						}
					}
					break;
				}
				if (!_homeAlbumNote.isEmpty()) {
					if (_homeAlbumLoading) {
						_tabList->add(object_ptr<HomeLoadingView>(
							_tabList, _homeAlbumNote, HomeLoadingLayout::TrackList));
					} else {
						_tabList->add(object_ptr<HomeRetryView>(_tabList, _homeAlbumNote,
							crl::guard(this, [=] { loadHomeAlbum(); })));
					}
				} else if (!_homeAlbumTracks.empty()) {
					_tabList->add(object_ptr<HomeShelfHeader>(_tabList, tr("Треки")));
					addRows(_homeAlbumTracks, _homeAlbumIsTrack ? RowMode::DetailTrack : RowMode::AlbumTrack);
				}
				if (!_homeArtistAlbums.empty() || !_homeArtistTracks.empty()) {
					_tabList->add(object_ptr<HomeShelfHeader>(
						_tabList, tr("Ещё от %1").arg(_homeAlbum.artist)));
					const auto &tracks = _homeArtistAlbums.empty()
						? _homeArtistTracks
						: std::vector<Track>();
					_tabList->add(object_ptr<HomeMixedGrid>(
						_tabList, _controller, tracks, _homeArtistAlbums,
						crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
						crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); }),
						false, std::make_shared<int>(0)));
				}
				// Не только автор: то же звучание у других исполнителей.
				if (!_homeSimilarTracks.empty() || _homeSimilarLoading) {
					if (!_homeSimilarTracks.empty()) {
						_tabList->add(object_ptr<HomeShelfHeader>(
							_tabList, tr("Похожее по звучанию")));
						_tabList->add(object_ptr<HomeMixedGrid>(
							_tabList, _controller, _homeSimilarTracks, std::vector<Album>(),
							crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
							crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); }),
							false, std::make_shared<int>(0)));
					} else {
						_tabList->add(object_ptr<HomeLoadingView>(
							_tabList, tr("Ищу похожее по звучанию…")));
					}
				}
				break;
			}
			loadHomeSoundCloudSections();
			if (_homeSection) {
				const auto page = (_homeSection - 100) / 2;
				const auto pageTracks = _homeSection >= 100
					&& !((_homeSection - 100) % 2)
					&& page < int(_homePages.size());
				const auto title = (_homeSection == 2) ? tr("Избранное")
					: (_homeSection == kHomeTrendingSection) ? tr("Trending")
					: (_homeSection == 4) ? tr("Weekly Wave")
					: (_homeSection == 5) ? tr("Daily Drops")
					: tr("Для вас");
				_tabList->add(object_ptr<HomeSectionHeader>(
					_tabList, title, crl::guard(this, [=] {
						rememberNavigation();
						_homeSection = 0;
						rebuildTab();
						_tabScroll->scrollToY(_homeFeedScroll);
					})));
				const auto emptyTracks = std::vector<Track>();
				const auto &tracks = pageTracks ? _homePages[page].tracks
					: (_homeSection == 2) ? liked
					: (_homeSection == kHomeTrendingSection) ? _homeTrending
					: (_homeSection == 4 || _homeSection == 5) ? emptyTracks
					: queue.forYou;
				auto albums = pageTracks ? _homePages[page].albums : std::vector<Album>();
				if (_homeSection == 4 && !_homeWeeklyWave.id.isEmpty()) {
					albums.push_back(_homeWeeklyWave);
				} else if (_homeSection == 5 && !_homeDailyDrops.id.isEmpty()) {
					albums.push_back(_homeDailyDrops);
				}
				if (tracks.empty() && albums.empty()) {
					_tabList->add(object_ptr<Ui::FlatLabel>(_tabList,
						_homeSection == 2
							? tr("Добавьте треки в избранное с помощью сердечка.")
							: tr("Нет доступной музыки в этой подборке."),
						st::boxLabel));
				} else {
					const auto offset = pageTracks ? _homePages[page].shelfOffset
						: (_homeSection == 2) ? _homeLikedOffset
						: (_homeSection == kHomeTrendingSection) ? _homeTrendingOffset
						: (_homeSection == 4) ? _homeWeeklyWaveOffset
						: (_homeSection == 5) ? _homeDailyDropsOffset
						: std::shared_ptr<int>();
					_tabList->add(object_ptr<HomeMixedGrid>(_tabList, _controller,
						tracks, albums,
						crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
						crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); }),
						true, offset,
						pageTracks ? HomeRecommendationArtists(tracks) : std::vector<Track>(),
						crl::guard(this, [=](Track track) { chooseTrackArtist(track); }),
						!pageTracks || page == 0));
				}
				break;
			}
			const auto section = [&](QString title, int id, const std::vector<Track> &tracks) {
				if (tracks.empty()) return;
				_tabList->add(object_ptr<HomeShelfHeader>(
					_tabList, title, crl::guard(this, [=] {
						_homeFeedScroll = _tabScroll->scrollTop();
						rememberNavigation();
						_homeSection = id;
						rebuildTab();
						_tabScroll->scrollToY(0);
					})));
				_tabList->add(object_ptr<HomeMixedGrid>(_tabList, _controller, tracks,
					std::vector<Album>(),
					crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
					crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); }),
					false, _homeLikedOffset));
			};
			const auto playlistSection = [&](
					QString title,
					int id,
					const Album &album,
					std::shared_ptr<int> offset) {
				if (album.id.isEmpty()) return;
				_tabList->add(object_ptr<HomeShelfHeader>(
					_tabList, std::move(title), crl::guard(this, [=] {
						_homeFeedScroll = _tabScroll->scrollTop();
						rememberNavigation();
						_homeSection = id;
						rebuildTab();
						_tabScroll->scrollToY(0);
					})));
				_tabList->add(object_ptr<HomeMixedGrid>(
					_tabList,
					_controller,
					std::vector<Track>(),
					std::vector<Album>{ album },
					crl::guard(this, [=](Album album) {
						openHomeAlbum(std::move(album));
					}),
					crl::guard(this, [=](Track track) {
						openHomeTrack(std::move(track));
					}),
					false,
					std::move(offset)));
			};
			const auto recent = RecentItems();
			if (!recent.empty()) {
				_tabList->add(object_ptr<HomeShelfHeader>(_tabList, tr("Недавно")));
				_tabList->add(object_ptr<HomeRecentGrid>(_tabList, _controller, recent,
					crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
					crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); })));
			}
			if (_homeSoundCloudLoading) {
				_tabList->add(object_ptr<HomeLoadingView>(
					_tabList, tr("Загружаю подборки SoundCloud…")));
			}
			playlistSection(
				tr("Weekly Wave"),
				4,
				_homeWeeklyWave,
				_homeWeeklyWaveOffset);
			if (!Mokagram::Settings::Bool(u"player.hide_trending"_q)) {
				section(tr("Trending"), kHomeTrendingSection, _homeTrending);
			}
			playlistSection(
				tr("Daily Drops"),
				5,
				_homeDailyDrops,
				_homeDailyDropsOffset);
			if (_homeSoundCloudLoaded
				&& _homeTrending.empty()
				&& _homeWeeklyWave.id.isEmpty()
				&& _homeDailyDrops.id.isEmpty()) {
				_tabList->add(object_ptr<HomeRetryView>(
					_tabList,
					_homeSoundCloudError.isEmpty()
						? tr("Подборки SoundCloud недоступны.")
						: _homeSoundCloudError,
					crl::guard(this, [=] {
						_homeSoundCloudLoaded = false;
						_homeTrending.clear();
						_homeWeeklyWave = {};
						_homeDailyDrops = {};
						loadHomeSoundCloudSections();
					})));
			}
			if (_homePages.empty() && !queue.forYou.empty()) {
				_homePages.push_back({ queue.forYou, {}, {} });
				crl::on_main(this, [=] { loadHomeAlbums(0); });
			}
			for (auto i = 0; i != int(_homePages.size()); ++i) {
				const auto &page = _homePages[i];
				const auto title = i ? tr("Ещё для вас") : tr("Для вас");
				_tabList->add(object_ptr<HomeShelfHeader>(
					_tabList, title, crl::guard(this, [=] {
						_homeFeedScroll = _tabScroll->scrollTop();
						rememberNavigation();
						_homeSection = 100 + 2 * i;
						rebuildTab();
						_tabScroll->scrollToY(0);
					})));
				_tabList->add(object_ptr<HomeMixedGrid>(_tabList, _controller,
					page.tracks, page.albums,
					crl::guard(this, [=](Album album) { openHomeAlbum(std::move(album)); }),
					crl::guard(this, [=](Track track) { openHomeTrack(std::move(track)); }),
					false, page.shelfOffset, HomeRecommendationArtists(page.tracks),
					crl::guard(this, [=](Track track) { chooseTrackArtist(track); }),
					i == 0));
				if (!i) section(tr("Избранное"), 2, liked);
			}
			if (_homePages.empty() && !_homeLoaded) {
				_tabList->add(object_ptr<HomeLoadingView>(
					_tabList, tr("Подбираю музыку под ваш вкус…")));
				if (!_homeLoading) {
					_homeLoading = true;
					crl::on_main(this, [=] {
						BuildForYou(30, crl::guard(this, [=](std::vector<Track> tracks, QString note) {
							auto &queue = QueueState();
							queue.forYou = std::move(tracks);
							queue.forYouNote = note;
							_homeLoading = false;
							_homeLoaded = true;
							if (_tab == Tab::Home) rebuildTab();
						}));
					});
				}
			} else if (_homePages.empty()) {
				const auto message = queue.forYouNote.isEmpty()
					? tr("Добавьте треки в избранное, чтобы получить рекомендации.")
					: queue.forYouNote;
				_tabList->add(object_ptr<HomeRetryView>(_tabList, message, crl::guard(this, [=] {
					_homeLoaded = false;
					rebuildTab();
				})));
			}
			if (_homePages.empty()) {
				section(tr("Избранное"), 2, liked);
			}
		} break;
		case Tab::Search:
			if (emptySearch) {
				break;
			} else if (_searchPending) {
				const auto category = _searchChats
					? std::nullopt
					: std::make_optional(_searchCategory);
				const auto side = style::ConvertScale(BoxParts::kOuterSide);
				_tabList->add(object_ptr<SearchSkeleton>(_tabList, _tabScroll, category),
					QMargins(side, 0, side, 0));
			} else if (!_searchChats && !_catalogItems.empty()) {
				fillCatalogResults();
			} else if (_searchTracks.empty()) {
				_tabNote = _searchStatus.isEmpty()
					? tr("Введи название или исполнителя.")
					: _searchStatus;
			} else {
				addRows(_searchTracks);
				if (_searchChats && _chatSearch && _chatSearch->hasMore()) {
					const auto more = _tabList->add(object_ptr<Ui::RoundButton>(
						_tabList,
						rpl::single(tr("Показать ещё")),
						st::defaultLightButton));
					more->setClickedCallback([=] {
						if (_chatSearch && _chatSearch->hasMore()) {
							more->setDisabled(true);
							_chatSearch->searchMore();
						}
					});
				}
			}
			break;
		case Tab::Library: {
			// Аккаунт первым: лайки из SoundCloud и свои — один список.
			_tabList->add(object_ptr<SoundCloudAccountBar>(
				_tabList,
				[=] { ShowSoundCloudLogin(nullptr); },
				[=] { Mokagram::Music::Account::Logout(); }),
				QMargins(12, 4, 12, 8));
			const auto liked = LikedTracks();
			if (liked.empty()) {
				_tabNote = tr("Пока пусто. Нажми сердечко у трека, и он появится здесь.");
			} else {
				addRows(liked);
			}
		} break;
		case Tab::Playlists: {
			const auto playlists = Playlists();
			const auto found = std::find_if(playlists.begin(), playlists.end(), [=](const Playlist &p) {
				return p.id == _openedPlaylist;
			});
			if (found != playlists.end()) {
				_tabList->add(object_ptr<HomeSectionHeader>(_tabList, found->name, [=] {
					rememberNavigation();
					_openedPlaylist.clear();
					rebuildTab();
				}));
				if (found->tracks.empty()) {
					_tabNote = found->remote
						? tr("В этом плейлисте SoundCloud нет доступных треков.")
						: tr("Добавь треки через их контекстное меню.");
				} else {
					addRows(
						found->tracks,
						RowMode::Results,
						found->remote ? QString() : found->id);
				}
			} else {
				_openedPlaylist.clear();
				_tabList->add(object_ptr<HomeSectionHeader>(_tabList, tr("Создать плейлист"), [=] {
					PlaylistNameBox(_controller, tr("Создать плейлист"), {}, crl::guard(this, [=](QString name) {
						rememberNavigation();
						_openedPlaylist = CreatePlaylist(name);
					}));
				}, true));
				_tabList->add(object_ptr<SoundCloudAccountBar>(
					_tabList,
					[=] { ShowSoundCloudLogin(nullptr); },
					[=] { Mokagram::Music::Account::Logout(); },
					true),
					QMargins(
						style::ConvertScale(12),
						style::ConvertScale(4),
						style::ConvertScale(12),
						style::ConvertScale(8)));
				for (const auto &playlist : playlists) {
					_tabList->add(object_ptr<PlaylistRow>(_tabList, _controller, playlist, [=](QString id) {
						rememberNavigation();
						_openedPlaylist = std::move(id);
						rebuildTab();
					}));
				}
				if (playlists.empty()) {
					_tabNote = _playlistsSyncLoading
						? tr("Загружаю плейлисты SoundCloud…")
						: tr("Создай плейлист или добавь трек через контекстное меню.");
				}
			}
		} break;
		case Tab::Count: Unexpected("Invalid music tab");
		}
		if (_tab == Tab::Search) {
			if (_tabScroll->y() != computeLayout().scrollTop) {
				relayout();
			}
			const auto registerResult = [&](QWidget *widget) {
				if (widget->isEnabled() && (dynamic_cast<Row*>(widget)
					|| dynamic_cast<CatalogEntityRow*>(widget))) {
					_searchResultWidgets.push_back(widget);
				}
			};
			for (auto i = 0; i != _tabList->count(); ++i) {
				const auto widget = _tabList->widgetAt(i).get();
				if (const auto group = dynamic_cast<MusicTrackList*>(widget)) {
					for (auto j = 0; j != group->count(); ++j) {
						registerResult(group->widgetAt(j));
					}
				} else {
					registerResult(widget);
				}
			}
		}
		_tabContent->resizeToWidth(_tabScroll->width());
		if (!showsQueue()) {
			_tabScroll->scrollToY(keep);
		}
		updateHomeArtistBar();
		_queuePanel->update();
		update();
		scheduleHomeMoreIfNeeded();
	}
	void refreshQueueViews() {
		if (showsQueue()) {
			fillTabQueue();
			update();
		}
		if (_queuePanelOpen && !_queueHistory) {
			scheduleQueuePanelRefresh();
		}
	}

	void fillTabQueue() {
		_tabNote.clear();
		const auto playingId = PlayingTrack().id;
		const auto focusTrack = !_tabQueueList || _tabQueuePlayingId != playingId;
		const auto [tracks, mode] = queueTracks(playingId);
		if (!_tabQueueList) {
			_tabQueueList = _tabList->add(object_ptr<CompactQueueList>(_tabList));
		}
		const auto list = _tabQueueList;
		fillQueueRows(list, tracks, mode, 0, 1, {});
		_tabContent->resizeToWidth(_tabScroll->width());
		const auto current = mode == RowMode::Queue
			? QueueState().index
			: int(ranges::find(tracks, playingId, &Track::id) - begin(tracks));
		if (focusTrack && current >= 0 && current < list->count()) {
			_tabQueuePlayingId = playingId;
			const auto first = std::max(current - 1, 0);
			_tabScroll->scrollToY(first ? list->y() + list->widgetAt(first)->y() : 0);
		}
		updateQueueEdgeFade(_tabScroll, _homeScrollFade);
		if (list->count() == 0) {
			_tabNote = QString::fromUtf8(Mokagram::Lang::Tr("Очередь пуста."));
		}
	}

	void setQueueHistory(bool history) {
		if (_queueHistory == history) {
			return;
		}
		_queueHistory = history;
		_queueTabSlide.start(history ? 0. : 1., history ? 1. : 0., kFastSpatial);
		_queueTabBar->update();
		_queueTabs[0]->setActive(!history);
		_queueTabs[1]->setActive(history);
		const auto label = PlaylistText(history ? "Поиск в истории" : "Поиск в очереди");
		_queueSearch->setPlaceholder(rpl::single(label));
		_queueSearch->setAccessibleName(label);
		_queueScroll->scrollToY(0);
		refreshQueuePanel();
	}

	void scheduleQueuePanelRefresh() {
		if (std::exchange(_queuePanelRefreshPending, true)) {
			return;
		}
		Ui::PostponeCall(this, [=] {
			if (std::exchange(_queuePanelRefreshPending, false) && _queuePanelOpen) {
				refreshQueuePanel(true);
			}
		});
	}

	struct QueueAdvance {
		QPixmap outgoing;
		int consumed = 0;
		int nextTop = -1;
	};

	[[nodiscard]] QueueAdvance prepareQueueAdvance(
			const Track &playing,
			std::span<const Track> tracks,
			int first,
			int step) {
		const auto current = first - step;
		const auto scrollTop = _queueScroll->scrollTop();
		if (!PlayerMotionEnabled() || !_queuePanel->isVisible()
			|| !_queueNow->count() || _queueNow->geometry().bottom() < scrollTop
			|| current < 0 || current >= int(tracks.size())
			|| tracks[current].id != playing.id
			|| static_cast<Row*>(_queueNow->widgetAt(0).get())->isQueueTrack(playing.id, 0)) {
			return {};
		}
		const auto visibleBottom = scrollTop + _queueScroll->height();
		for (auto i = 0; i != _queueList->count(); ++i) {
			const auto row = static_cast<Row*>(_queueList->widgetAt(i).get());
			if (_queueList->y() + row->y() >= visibleBottom) {
				break;
			}
			if (!row->isQueueTrack(playing.id, current)) {
				continue;
			}
			auto result = QueueAdvance{ Ui::GrabWidget(_queueNow), i + 1 };
			if (result.consumed < _queueList->count()
				&& first >= 0 && first < int(tracks.size())) {
				const auto next = static_cast<Row*>(_queueList->widgetAt(result.consumed).get());
				if (next->isQueueTrack(tracks[first].id, first)) {
					result.nextTop = _queueList->y() + next->y();
				}
			}
			return result;
		}
		return {};
	}

	void applyQueueAdvance(double remaining) {
		_queueNow->setVerticalShift(0, qRound(_queueNow->height() * remaining));
		_queueContent->setVerticalShift(_queueListSlot, qRound(_queueAdvanceListShift * remaining));
	}

	void updateQueueAdvance() {
		applyQueueAdvance(1. - _queueAdvance.unitValue(1.));
		if (!_queueAdvance.animating()) {
			_queueAdvanceExit = QPixmap();
		}
		_queueNow->update();
	}

	void stopQueueAdvance() {
		if (!_queueAdvanceExit.isNull()) {
			_queueAdvance.stop();
			updateQueueAdvance();
		}
	}

	void refreshQueuePanel(bool animateAdvance = false) {
		_queuePanelRefreshPending = false;
		++_queueLayoutRevision;
		_queueReorder->cancel();
		const auto terms = _queueSearch->getLastText().simplified().split(' ', Qt::SkipEmptyParts);
		const auto playing = _queueHistory ? Track() : PlayingTrack();
		const auto hasCurrent = !playing.id.isEmpty();
		auto advance = QueueAdvance();
		if (_queueHistory) {
			stopQueueAdvance();
			auto recent = RecentItems();
			std::vector<Track> tracks;
			tracks.reserve(recent.size());
			for (auto &item : recent) {
				if (!item.isAlbum) {
					tracks.push_back(std::move(item.track));
				}
			}
			fillQueueRows(_queueList, tracks, RowMode::QueueHistory, 0, 1, terms);
		} else {
			const auto [tracks, mode] = queueTracks(playing.id);
			const auto queued = (mode == RowMode::Queue);
			const auto order = Core::App().settings().playerOrderMode();
			const auto step = (queued || order == Media::OrderMode::Reverse) ? 1 : -1;
			const auto queueIndex = QueueState().index;
			const auto current = [&] {
				if (queued && (!hasCurrent || (queueIndex >= 0
					&& queueIndex < int(tracks.size()) && tracks[queueIndex].id == playing.id))) {
					return queueIndex;
				}
				const auto at = ranges::find(tracks, playing.id, &Track::id);
				return (at != end(tracks)) ? int(at - begin(tracks)) : -1;
			}();
			// Do not consume the selected row until playback actually starts.
			const auto first = queued ? current + 1 : (current < 0 ? -1 : current + step);
			if (animateAdvance && hasCurrent && terms.empty()) {
				advance = prepareQueueAdvance(playing, tracks, first, step);
			}
			const auto firstValid = first >= 0 && first < int(tracks.size());
			const auto keepAdvance = _queueAdvance.animating() && animateAdvance && terms.empty()
				&& _queueNow->count()
				&& static_cast<Row*>(_queueNow->widgetAt(0).get())->isQueueTrack(playing.id, 0)
				&& (firstValid
					? _queueList->count() && static_cast<Row*>(_queueList->widgetAt(0).get())
						->isQueueTrack(tracks[first].id, first)
					: !_queueList->count());
			if (keepAdvance) {
				applyQueueAdvance(0.);
			} else {
				stopQueueAdvance();
			}
			for (auto i = 0; i != advance.consumed; ++i) {
				delete _queueList->widgetAt(0).get();
			}
			fillQueueRows(_queueList, tracks, queued ? RowMode::QueueUpcoming : RowMode::Context,
				first, step, terms);
			if (queued) {
				_queueUpcomingFirst = first;
				if (terms.empty() && current == queueIndex) {
					_queueReorder->start();
				}
			}
		}
		_queueNowLabel->toggle(hasCurrent, anim::type::instant);
		_queueNextLabel->toggle(!_queueHistory, anim::type::instant);
		fillQueueRows(_queueNow, std::span(&playing, hasCurrent ? 1 : 0),
			RowMode::QueueCurrent, 0, 1, {});
		const auto empty = (_queueList->count() == 0);
		_queueEmpty->toggle(empty, anim::type::instant);
		if (empty) {
			_queueEmpty->entity()->setText(PlaylistText(!terms.empty() ? "Ничего не найдено"
				: _queueHistory ? "Нет недавно прослушанных треков" : "Нет следующих треков"));
		}
		_queueContent->resizeToWidth(_queueScroll->width());
		updateQueueEdgeFade(_queueScroll, _queueEdgeFade);
		if (!advance.outgoing.isNull()) {
			_queueAdvanceListShift = (advance.nextTop >= 0 && _queueList->count())
				? std::max(advance.nextTop - _queueList->y() - _queueList->widgetAt(0)->y(), 0)
				: 0;
			_queueAdvanceExit = std::move(advance.outgoing);
			_queueAdvance.start(0., 1., kEffects);
		}
		if (_queueAdvance.animating() || !_queueAdvanceExit.isNull()) {
			updateQueueAdvance();
		}
	}

	[[nodiscard]] std::pair<const std::vector<Track>&, RowMode> queueTracks(
			const QString &playingId) {
		const auto &queue = QueueState();
		const auto inQueue = playingId.isEmpty()
			|| ranges::contains(queue.tracks, playingId, &Track::id);
		if (!inQueue) {
			// Играет трек из чата: очередь это музыка того же чата.
			const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
			const auto peer = current.contextId().peer;
			if (peer && peer != _contextPeer) {
				_contextPeer = peer;
				ContextTracks().clear();
				if (const auto session = QueueState().session.get()) {
					_context = std::make_unique<ChatMusicSearch>(
						session,
						crl::guard(this, [=](std::vector<Track> list, QString) {
							ContextTracks() = std::move(list);
							refreshQueueViews();
						}),
						peer);
					_context->search(QString());
				}
			}
		}
		return inQueue
			? std::pair<const std::vector<Track>&, RowMode>(queue.tracks, RowMode::Queue)
			: std::pair<const std::vector<Track>&, RowMode>(ContextTracks(), RowMode::Context);
	}

	void fillQueueRows(
			not_null<Ui::VerticalLayout*> list,
			std::span<const Track> tracks,
			RowMode mode,
			int first,
			int step,
			const QStringList &terms) {
		auto shown = 0;
		for (auto i = first; i >= 0 && i < int(tracks.size()); i += step) {
			const auto &track = tracks[i];
			if (!terms.empty()) {
				const auto text = track.title + u" "_q + track.artist;
				if (!std::ranges::all_of(terms, [&](const auto &term) {
					return text.contains(term, Qt::CaseInsensitive);
				})) {
					continue;
				}
			}
			const auto rowIndex = shown++;
			if (rowIndex < list->count()) {
				const auto existing = list->widgetAt(rowIndex).get();
				const auto row = dynamic_cast<Row*>(existing);
				if (row && row->matchesQueueTrack(track, mode, i)) {
					continue;
				}
				delete existing;
			}
			list->insert(rowIndex, object_ptr<Row>(
				list,
				_controller,
				track,
				i,
				mode));
		}
		while (list->count() > shown) {
			delete list->widgetAt(list->count() - 1).get();
		}
	}

	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		auto hq = PainterHighQualityEnabler(p);
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto plainBackground = _plainAnim.unitValue(_tab == Tab::Now ? 0. : 1.);
		if (plainBackground < 1.) {
			paintBackgroundPart(p, e->rect());
		} else {
			_preparedBackdrop = QImage();
		}
		// Вне «Сейчас играет» экран в цветах клиента, как остальные окна.
		if (plainBackground > 0.) {
			p.setOpacity(plainBackground);
			p.fillRect(e->rect(), st::boxBg);
			p.setOpacity(1.);
		}
		const auto layout = computeLayout();
		if (_tab == Tab::Home && _homeAlbumOpen
			&& Mokagram::Settings::Bool(u"player.cover_colors"_q)) {
			PaintHomeSideGlow(p, QRect(0, layout.scrollTop, width(),
				layout.contentBottom - layout.scrollTop), homeSideGlowColor(),
				IsLightMusicTheme(), _homeSideGlowPulsing ? _backgroundPulse : 0.);
		}
		if (_tab == Tab::Search && showCatalogFilterSkeleton()) {
			auto color = st::windowSubTextFg->c;
			color.setAlphaF(0.18);
			p.setPen(Qt::NoPen);
			p.setBrush(color);
			p.save();
			p.setClipRect(layout.center, Qt::IntersectClip);
			for (const auto chip : _searchFilters) {
				p.drawRoundedRect(chip->geometry(), s(16), s(16));
			}
			p.restore();
		}
		// Одна общая дорожка и движущаяся пилюля вместо отдельных фонов вкладок.
		p.setPen(Qt::NoPen);
		const auto colors = navColors();
		p.setBrush(colors.track);
		p.drawRoundedRect(layout.nav, s(20), s(20));
		const auto inset = s(4);
		const auto segment = std::max(layout.nav.width() - 2 * inset, 0) / float64(layout.verticalNav ? 3 : _nav.size());
		const auto last = double(_nav.size() - 1);
		const auto at = layout.verticalNav
			? navColumn(int(_tab), 3)
			: std::clamp(_navSlide.value(double(int(_tab))), 0., last);
		const auto stretch = layout.verticalNav
			? 0.
			: segment * std::min(
				0.18 * std::min(std::abs(_navSlide.velocity()) / std::sqrt(kFastSpatial.stiffness), 1.),
				2. * std::min(at, last - at));
		const auto active = QRectF(
			layout.nav.x() + inset + segment * at - stretch / 2.,
			layout.nav.y() + inset + (layout.verticalNav ? s(40) * (int(_tab) / 3) : 0),
			segment + stretch,
			s(40) - 2 * inset);
		p.setBrush(colors.selected);
		p.drawRoundedRect(active, active.height() / 2., active.height() / 2.);
		if (showsQueue() && !_queuePanelOpen && _tabQueueList && _tabQueueList->count()) {
			PaintMusicCard(p, _tabScroll->geometry());
		}
		// Открытая вкладка лежит на карточке; на «Сейчас играет» обложка и текст прямо на фоне.
		if ((_tab != Tab::Now || (showsQueue() && !_queuePanelOpen))
			&& !(_tab == Tab::Home && _homeAlbumOpen)) {
			if (!_tabNote.isEmpty()) {
				p.setFont(PlayerStyle::kTrackArtist.musicFont());
				p.setPen(MusicSecondaryColor());
				const auto noteWidth = std::max(_tabScroll->width() - s(2 * BoxParts::kOuterSide), 0);
				p.drawText(
					QRect(_tabScroll->x() + s(BoxParts::kOuterSide), layout.scrollTop + ((_tab == Tab::Playlists) ? s(64) : 0), noteWidth, s(80)),
					Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
					_tabNote);
			}
			if (_tab == Tab::Search && _searchEmptySticker) {
				const auto sticker = _searchEmptySticker->geometry();
				p.setFont(PlayerStyle::kTrackArtist.musicFont());
				p.setPen(MusicSecondaryColor());
				p.drawText(QRect(layout.center.x() + s(10), sticker.bottom() + s(12),
					std::max(layout.center.width() - s(20), 0), s(44)),
					Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
					QString::fromUtf8(Mokagram::Lang::Tr("Кажется тут ничего нет.. напиши что нибудь в поиск")));
			}

		}

		// Обложка: сжимается на паузе, при смене трека плавно перетекает в новую.
		if (layout.barShown && _tab != Tab::Home) {
			paintPlaybackBarBackground(p, layout);
		}
		const auto cover = expandedCoverRect(layout).toAlignedRect().intersected(rect());
		if (!_coverMorph.animating() && e->region().intersects(cover)) {
			// Partial repaints must reuse the frame already shown on the rest of the cover.
			const auto fullCover = QRegion(cover).subtracted(e->region()).isEmpty();
			paintFullscreenCover(p, layout, fullCover);
		}
		if (!layout.barShown || _tab == Tab::Home) {
			return;
		}

		paintPlaybackBarDetails(p, layout);
	}

	void paintFullscreenCover(
			QPainter &p,
			const Layout &layout,
			bool fullCover = true) {
		const auto s = [](int v) { return style::ConvertScale(v); };
		const auto fade = _infoFade.unitValue(1.);
		const auto flying = _coverMorph.animating();
		const auto expanded = flying ? _coverTransition.expanded() : 1.;
		const auto coverRect = flying ? _coverTransition.rect() : expandedCoverRect(layout);
		auto rounded = QPainterPath();
		if (!flying && !coverRect.isEmpty()) {
			rounded = Ui::SquirclePath(coverRect, style::ConvertScale(22)
				* (0.94 + 0.06 * _playAnim.value(_isPlaying ? 1. : 0.)), 0.);
		}
		const auto &coverPath = flying ? _coverTransition.path() : rounded;
		const auto drawCover = [&](const QPixmap &pixmap, double opacity, bool current) {
			if (opacity <= 0. || coverRect.isEmpty()) {
				return;
			}
			p.save();
			p.setOpacity(opacity);
			p.setClipPath(coverPath, Qt::IntersectClip);
			if (pixmap.isNull()) {
				p.fillRect(coverRect, anim::color(QColor(0x32, 0x32, 0x32),
					QColor(0x23, 0x23, 0x25), expanded));
			} else {
				p.drawPixmap(coverRect, pixmap, CoverSourceRect(pixmap.size(), coverRect.size()));
			}
			if (current && expanded > 0.) {
				p.setOpacity(opacity * expanded);
				if (fullCover) {
					_spotifyPaletteFrame = motionFrame(_spotifyCanvas, layout.cover.size());
					_coverVideoFrame = _spotifyPaletteFrame;
					if (_coverVideoFrame.isNull()) {
						_coverVideoFrame = _canvas.frameForPaint();
					}
					if (_coverVideoFrame.isNull()) {
						_coverVideoFrame = motionFrame(_motionCover, layout.cover.size());
					}
				}
				if (!_coverVideoFrame.isNull()) {
					p.drawImage(coverRect, _coverVideoFrame,
						CoverSourceRect(_coverVideoFrame.size(), coverRect.size()));
				} else if (pixmap.isNull()) {
					st::menuIconSoundOn.paintInCenter(p, coverRect.toAlignedRect(), QColor(0xa1, 0xa1, 0xa6));
				}
			}
			p.restore();
		};
		drawCover(_coverWas, 1. - fade, false);
		drawCover(_cover, fade, true);
		p.save();
		p.setClipPath(coverPath, Qt::IntersectClip);
		p.setOpacity(expanded);
		auto shade = QLinearGradient(coverRect.topLeft(), coverRect.bottomLeft());
		shade.setColorAt(0., QColor(0, 0, 0, 20));
		shade.setColorAt(0.5, QColor(0, 0, 0, 0));
		shade.setColorAt(0.72, QColor(0, 0, 0, 80));
		shade.setColorAt(1., QColor(0, 0, 0, 230));
		p.fillRect(coverRect, shade);
		if (!layout.cover.isEmpty()) {
			p.translate(coverRect.topLeft());
			p.scale(coverRect.width() / layout.cover.width(), coverRect.height() / layout.cover.height());
			p.translate(-layout.cover.topLeft());
			_canvas.paintLoading(p, QRect(
				layout.cover.x() + s(24),
				layout.cover.y() + s(16),
				std::max(layout.cover.width() - s(48), 0),
				s(4)), _accent);
		}

		// Название и исполнитель; под курсором подчёркнуты и ведут на их страницы.
		using namespace PlayerStyle;
		const auto lines = metadataLines(layout.info, false);
		const auto drawInfo = [&](
				const QString &title,
				const QString &artist,
				double opacity,
				bool current) {
			if (opacity <= 0. || layout.cover.isEmpty()) {
				return;
			}
			p.setPen(QColor(255, 255, 255, int(255 * opacity)));
			paintMovingTitle(p, lines.title, title, false, current);
			const auto artistHover = current && _hover == InfoHover::Artist;
			const auto font = kTrackArtist.musicFont(artistHover);
			p.setFont(font);
			p.setPen(QColor(255, 255, 255, int((artistHover ? 255 : 210) * opacity)));
			p.drawText(
				lines.artist,
				Qt::AlignLeft | Qt::AlignVCenter,
				QFontMetrics(font).elidedText(artist, Qt::ElideRight, lines.artist.width()));
		};
		drawInfo(_wasTitle, _wasArtist, 1. - fade, false);
		drawInfo(_title, _artist, fade, true);
		p.restore();
	}


	void paintPlaybackBarBackground(QPainter &p, const Layout &layout) const {
		p.setPen(Qt::NoPen);
		p.fillPath(
			Ui::SquirclePath(QRectF(layout.bar), style::ConvertScale(kBarRadius)),
			_playbackColors.surface);
	}

	void paintPlaybackBarDetails(QPainter &p, const Layout &layout) const {
		using namespace PlayerStyle;
		// На «Сейчас играет» обложки в капсуле нет: там большая.
		const auto fade = _infoFade.unitValue(1.);
		if (!layout.ultra && _tab != Tab::Now) {
			p.save();
			p.setClipPath(Ui::SquirclePath(QRectF(layout.barCover), barCoverRadius(layout)), Qt::IntersectClip);
			const auto draw = [&](const QPixmap &pixmap, double opacity) {
				p.setOpacity(opacity);
				if (pixmap.isNull()) {
					p.fillRect(layout.barCover, st::windowBgOver);
				} else {
					p.drawPixmap(QRectF(layout.barCover), pixmap,
						CoverSourceRect(pixmap.size(), layout.barCover.size()));
				}
			};
			draw(_coverWas, 1. - fade);
			draw(_cover, fade);
			p.restore();
		}
		if (!layout.ultra && !layout.compact && layout.barInfo.width() > 0) {
			// Без маленькой обложки текст сдвигается влево на её место.
			const auto info = barInfoRect(layout);
			const auto lines = metadataLines(info, true);
			p.setPen(_playbackColors.onSurface);
			paintMovingTitle(p, lines.title, _title, true, true);
			const auto font = kCompactArtist.musicFont(_hover == InfoHover::BarArtist);
			p.setFont(font);
			p.setPen(_hover == InfoHover::BarArtist
				? _playbackColors.onSurface
				: _playbackColors.onSurfaceVariant);
			p.drawText(
				lines.artist,
				Qt::AlignLeft | Qt::AlignVCenter,
				QFontMetrics(font).elidedText(_artist, Qt::ElideRight, lines.artist.width()));
		}

		// Время по сторонам полосы перемотки.
		p.setFont(kTimeLabel.musicFont());
		p.setPen(_playbackColors.onSurfaceVariant);
		p.drawText(layout.timeNow, Qt::AlignRight | Qt::AlignVCenter, _timeNow);
		p.drawText(layout.timeTotal, Qt::AlignLeft | Qt::AlignVCenter, _timeTotal);
	}

private:
	QImage motionFrame(Media::Clip::ReaderPointer &reader, QSize size) {
		if (!reader || !reader->ready() || size.isEmpty()) return {};
		const auto request = Media::Clip::FrameRequest{
			.frame = QSize(reader->width(), reader->height()).scaled(
				size, Qt::KeepAspectRatioByExpanding),
			.outer = size,
			.factor = style::DevicePixelRatio(),
		};
		if (!reader->started()) reader->start(request);
		const auto animate = _isPlaying && isVisible() && Mokagram::Perf::UserIsLooking()
			&& !PowerSaving::On(PowerSaving::kAnimations);
		return reader->current(request, animate ? crl::now() : 0);
	}

	void setMotion(Media::Clip::ReaderPointer &reader, const QString &path) {
		reader = nullptr;
		if (path.isEmpty()) return;
		const auto target = &reader;
		reader = Media::Clip::MakeReader(path, [=](Media::Clip::Notification) {
			if (*target && (*target)->state() == Media::Clip::State::Error) {
				QFile::remove(path);
				target->setBad();
			}
			if (_tab == Tab::Now) {
				update(computeLayout().cover);
			}
		});
	}

	QImage canvasBackgroundFrame() const {
		if (_canvasBackground.isNull()) return {};
		auto result = _canvasBackgroundFrom.copy();
		auto painter = QPainter(&result);
		painter.setOpacity(_canvasFade.unitValue(1.));
		painter.drawImage(result.rect(), _canvasBackground);
		return result;
	}

	void setCanvasPalette(QImage background, QColor accent) {
		repaintBackdrop();
		_canvasBackgroundFrom = _canvasBackground.isNull()
			? CoverBackgroundForPlayback() : canvasBackgroundFrame();
		if (_canvasBackgroundFrom.isNull()) {
			_canvasBackgroundFrom = QImage(QSize(32, 32), QImage::Format_ARGB32_Premultiplied);
			_canvasBackgroundFrom.fill(QColor(0x14, 0x14, 0x15));
		}
		_canvasBackground = std::move(background);
		_canvasAccentFrom = _accent;
		_canvasAccentTo = accent;
		_canvasFade.stop();
		_canvasFade.start(0., 1., kAmbientEffects);
		updatePlaybackBarColors();
	}

	void updatePlaybackBarColors(bool paletteChanged = false) {
		auto accent = QColor();
		const auto now = (_tab == Tab::Now);
		if (now && Mokagram::Settings::Bool(u"player.cover_colors"_q)) {
			accent = _coverAccent.isValid() || _canvasBackground.isNull()
				? _coverAccent
				: anim::color(_canvasAccentFrom, _canvasAccentTo, _canvasFade.unitValue(1.));
		}
		if (!paletteChanged && _playbackBarAccent == accent && _playbackBarIsNow == now) return;
		_playbackBarAccent = accent;
		_playbackBarIsNow = now;
		_playbackColors = MakePlaybackBarColors(accent, now);
		repaintPlaybackBar();
		for (const auto child : _playbackBar->children()) {
			if (const auto widget = qobject_cast<QWidget*>(child)) {
				widget->update();
			}
		}
	}

	void repaintPlaybackBar() {
		if (_tab == Tab::Home) {
			_playbackBar->update();
		} else {
			update(_playbackBar->geometry());
		}
	}

	void repaintBackdrop() {
		_backdropDirty = true;
		updatePlaybackBarColors();
		update();
		_view->update();
		_queuePanel->update();
	}

	void repaintAnimatedBackdrop(crl::time now, bool finished = false) {
		const auto frame = now / kBackdropFrameInterval;
		if (frame != _animatedBackdropFrame || finished) {
			_animatedBackdropFrame = frame;
			repaintBackdrop();
		}
	}

	void updateBackdropEnergy(double elapsed) {
		const auto state = Media::Player::instance()->getState(AudioMsgId::Type::Song);
		const auto &bands = state.visualizerLevels;
		const auto bass = state.waitingForData
			? 0.
			: 0.75 * bands[0] + 0.25 * bands[1];
		const auto accent = std::max(0., bass - _backgroundBass);
		_backgroundBass += (bass - _backgroundBass) * (1. - std::exp(-elapsed * 2.));
		const auto target = std::clamp(bass * 0.2 + accent * 2.5, 0., 1.);
		const auto release = std::exp(-elapsed * 7.);
		_backgroundPulse = std::max(target, _backgroundPulse * release);
	}

	void updateBackdropPalette() {
		if (_tab == Tab::Now && crl::now() >= _canvasPaletteNext) {
			_canvasPaletteNext = crl::now() + 3000;
			auto frame = _spotifyPaletteFrame;
			if (frame.isNull()) frame = _canvas.frame();
			if (!frame.isNull()) {
				auto background = Images::BlurLargeImage(
					frame.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
						.convertToFormat(QImage::Format_ARGB32_Premultiplied), 10);
				{
					auto shade = QPainter(&background);
					shade.fillRect(background.rect(), QColor(0x14, 0x14, 0x15, 180));
				}
				setCanvasPalette(std::move(background), ComputeAccent(frame));
			} else if (!_canvasBackground.isNull()) {
				setCanvasPalette(CoverBackgroundForPlayback(), CurrentCoverAccent());
			}
		}
	}

	void paintBackgroundPart(QPainter &p, QRect area, QPoint origin = {}) {
		if (size().isEmpty() || area.isEmpty()) return;
		updateBackdropPalette();
		const auto ratio = devicePixelRatioF();
		const auto pixels = QSize(
			int(std::ceil(width() * ratio)),
			int(std::ceil(height() * ratio)));
		if (_preparedBackdrop.size() != pixels
			|| _preparedBackdrop.devicePixelRatio() != ratio) {
			_preparedBackdrop = QImage(pixels, QImage::Format_RGB32);
			_preparedBackdrop.setDevicePixelRatio(ratio);
			_backdropDirty = true;
		}
		if (_preparedBackdrop.isNull()) {
			p.fillRect(area, MusicBackdropBase());
			return;
		}
		if (_backdropDirty) {
			prepareBackdrop();
			_backdropDirty = false;
		}
		p.save();
		p.setClipRect(area, Qt::IntersectClip);
		p.drawImage(-origin, _preparedBackdrop);
		p.restore();
	}

	void prepareBackdrop() {
		if (!_canvasBackground.isNull()) {
			const auto accent = anim::color(
				_canvasAccentFrom, _canvasAccentTo, _canvasFade.unitValue(1.));
			_accent = accent;
		}
		// Keep the blurred backdrop small, but retain colour precision while
		// compositing the gradients so dark transitions do not become steps.
		const auto extent = style::ConvertScale(256);
		const auto rasterSize = size().scaled(
			QSize(extent, extent), Qt::KeepAspectRatio);
		if (_backdropRaster.size() != rasterSize) {
			_backdropRaster = QImage(rasterSize, QImage::Format_RGBA64_Premultiplied);
		}
		_backdropRaster.fill(MusicBackdropBase());
		{
			auto raster = QPainter(&_backdropRaster);
			auto hq = PainterHighQualityEnabler(raster);
			raster.scale(
				rasterSize.width() / double(width()),
				rasterSize.height() / double(height()));
			paintBackdrop(raster);
		}
		// Keep the grain at display size. Translucent black and white approximate
		// SoftLight on the dark backdrop and use the cheaper SourceOver blend.
		static const auto grain = [] {
			const auto extent = style::ConvertScale(128);
			auto image = QImage(extent, extent, QImage::Format_ARGB32_Premultiplied);
			auto random = uint32_t(0x6d2b79f5);
			for (auto y = 0; y != extent; ++y) {
				const auto line = reinterpret_cast<QRgb*>(image.scanLine(y));
				for (auto x = 0; x != extent; ++x) {
					random ^= random << 13;
					random ^= random >> 17;
					random ^= random << 5;
					line[x] = (random & 1) ? qRgba(3, 3, 3, 3) : qRgba(0, 0, 0, 14);
				}
			}
			return QBrush(image);
		}();
		auto p = QPainter(&_preparedBackdrop);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		const auto area = QRectF(QPointF(), _preparedBackdrop.deviceIndependentSize());
		p.drawImage(area, _backdropRaster);
		if (!IsLightMusicTheme()) {
			p.fillRect(area, grain);
		}
	}

	void paintBackdrop(QPainter &p) {
		const auto area = rect();
		const auto light = IsLightMusicTheme();
		const auto base = MusicBackdropBase();
		const auto pulse = animatesBackdrop()
			&& !PowerSaving::On(PowerSaving::kAnimations)
			&& QGuiApplication::applicationState() == Qt::ApplicationActive
			? _backgroundPulse
			: 0.;
		if (Mokagram::Settings::Bool(u"player.cover_colors"_q)) {
			if (_canvasBackground.isNull()) {
				_backdrop.paint(p, area, area);
			} else {
				p.drawImage(area, canvasBackgroundFrame());
			}
			auto shade = light ? base : QColor(0x10, 0x10, 0x12);
			shade.setAlpha(light ? 234 : 100);
			p.fillRect(area, shade);
			if (_accent.isValid() && Mokagram::Settings::Bool(u"player.dynamic_background"_q)) {
				const auto phase = crl::now() / 14000.;
				const auto center = QPointF(width() * (0.5 + 0.25 * std::sin(phase)),
					height() * (0.5 + 0.25 * std::cos(phase * 0.7)));
				auto glow = QRadialGradient(center, std::max(width(), height()) * 0.7);
				auto color = _accent;
				color.setAlpha(light ? int(16 + 8 * pulse) : int(60 + 24 * pulse));
				glow.setColorAt(0., color);
				color.setAlpha(0);
				glow.setColorAt(1., color);
				p.fillRect(area, glow);
			}
		} else {
			p.fillRect(area, base);
		}
		const auto layout = computeLayout();
		if (!layout.cover.isEmpty() && _accent.isValid()) {
			const auto cover = QRectF(layout.cover);
			auto glow = QRadialGradient(
				cover.center(),
				std::max(cover.width(), cover.height()) * 0.8);
			auto color = _accent;
			color.setAlpha(light ? int(14 + 10 * pulse) : int(45 + 38 * pulse));
			glow.setColorAt(0., color);
			color.setAlpha(0);
			glow.setColorAt(1., color);
			p.fillRect(area, glow);
		}
	}

	void refresh() {
		_backdrop.refresh([=] { repaintBackdrop(); }, isVisible());
		const auto track = PlayingTrack();
		if (_canvasPaletteFor != track.id) {
			_canvasPaletteFor = track.id;
			_backgroundBass = _backgroundPulse = 0.;
			_canvasFade.stop();
			_canvasBackground = QImage();
			_canvasBackgroundFrom = QImage();
			_spotifyPaletteFrame = QImage();
			_canvasPaletteNext = 0;
			_coverAccent = {};
			_accent = {};
			repaintBackdrop();
		}
		if (_closing) {
			return;
		}
		setPlaybackBarShown(!track.id.isEmpty());
		if (track.id.isEmpty()) {
			_playbackArtworkFor = AudioMsgId();
			_playbackArtworkLifetime.destroy();
			_playbackArtworkMedia = nullptr;
			if (_isPlaying) repaintBackdrop();
			_isPlaying = false;
			_seek->setExpressive(true, false);
			_seek->setDuration(0);
			updateAmbientTimer();
			if (_cancelMotion) _cancelMotion();
			_cancelMotion = nullptr;
			_motionFor.clear();
			_motionCover = nullptr;
			_spotifyCanvas = nullptr;
			_canvas.refresh({}, {}, {}, false);
			_upcomingCanvas.refresh({}, {}, {}, false);
			return;
		}
		const auto instance = Media::Player::instance();
		const auto current = instance->current(AudioMsgId::Type::Song);
		const auto state = instance->getState(AudioMsgId::Type::Song);
		const auto frequency = std::max<int64>(state.frequency, 1);
		if (_playbackArtworkFor != current) {
			_playbackArtworkFor = current;
			_playbackArtworkLifetime.destroy();
			_playbackArtworkMedia = nullptr;
			if (const auto document = current.audio(); document && !OwnsTrack(current)) {
				_playbackArtworkMedia = document->createMediaView();
				if (document->hasThumbnail() && !_playbackArtworkMedia->thumbnail()) {
					document->session().downloaderTaskFinished(
					) | rpl::on_next([=] {
						if (_playbackArtworkMedia && _playbackArtworkMedia->thumbnail()) {
							_playbackArtworkLifetime.destroy();
							refresh();
						}
					}, _playbackArtworkLifetime);
					_playbackArtworkMedia->thumbnailWanted(current.contextId());
				}
			}
			if (CoverForPlayback(current).isNull()) {
				details::LoadPlaybackArtwork(track);
			}
		}

		// Обложка могла догрузиться позже трека.
		const auto cover = CoverForPlayback(current);
		const auto coverKey = cover.cacheKey();
		const auto changed = (_title != track.title)
			|| (_artist != track.artist)
			|| (_coverKey != coverKey);
		if (changed) {
			const auto first = _title.isEmpty() && _artist.isEmpty() && _cover.isNull();
			if (!first) {
				_wasTitle = _title;
				_wasArtist = _artist;
				_coverWas = _cover;
				_infoFade.stop();
				_infoFade.start(0., 1., kSlowEffects);
			}
			if (_title != track.title) _titleElapsed = {};
			_title = track.title;
			_artist = track.artist;
			_coverKey = coverKey;
			constexpr auto kMaxCoverSize = 1280;
			_cover = QPixmap::fromImage(
				std::max(cover.width(), cover.height()) > kMaxCoverSize
					? cover.scaled(kMaxCoverSize, kMaxCoverSize,
						Qt::KeepAspectRatio, Qt::SmoothTransformation)
					: cover);
			_coverAccent = ComputeAccent(cover);
			if (_canvasBackground.isNull()) _accent = _coverAccent;
			updateCoverTitleScroll();
			if (_tab == Tab::Home) {
				_playbackBar->update();
			}
			repaintBackdrop();
		}
		const auto playing = state.state == Media::Player::State::Playing
			|| state.state == Media::Player::State::Starting
			|| state.state == Media::Player::State::Resuming;
		const auto publicCanvas = Mokagram::Settings::Bool(u"player.public_canvas"_q);
		_canvas.refresh(
			publicCanvas && !_spotifyCanvas ? track.id : QString(),
			track.title, track.artist, playing && _tab == Tab::Now);
		const auto &queue = QueueState();
		const auto &settings = Core::App().settings();
		const auto next = queue.index + 1;
		if (publicCanvas && playing && _tab == Tab::Now && isVisible()
			&& !_canvas.loading() && !anim::Disabled()
			&& !PowerSaving::On(PowerSaving::kAnimations)
			&& QGuiApplication::applicationState() == Qt::ApplicationActive
			&& queue.index >= 0 && track.id == queue.currentId
			&& next < int(queue.tracks.size())
			&& settings.playerOrderMode() != Media::OrderMode::Shuffle
			&& settings.playerRepeatMode() != Media::RepeatMode::One) {
			const auto &upcoming = queue.tracks[next];
			_upcomingCanvas.refresh(upcoming.id, upcoming.title, upcoming.artist, false);
		} else {
			_upcomingCanvas.refresh({}, {}, {}, false);
		}
		if (_isPlaying != playing) {
			_isPlaying = playing;
			repaintBackdrop();
			updateAmbientTimer();
			_playAnim.start(playing ? 0. : 1., playing ? 1. : 0., kFastSpatial);
		}
		_play->setPlaying(playing);
		_seek->setExpressive(true, playing);
		if (_motionFor != track.id) {
			if (_cancelMotion) _cancelMotion();
			_motionFor = track.id;
			_motionCover = nullptr;
			_spotifyCanvas = nullptr;
			const auto id = track.id;
			_cancelMotion = FetchMotion(id, track.title, track.artist, track.durationMs,
				crl::guard(this, [=](MotionFiles files) {
					if (_motionFor != id) return;
					if (Mokagram::Settings::Bool(u"player.motion_covers"_q)) {
						setMotion(_motionCover, files.artwork);
					}
					if (Mokagram::Settings::Bool(u"player.spotify_canvas"_q)) {
						setMotion(_spotifyCanvas, files.canvas);
						if (_spotifyCanvas) _canvas.refresh({}, {}, {}, false);
					}
					update();
				}));
		}
		_heartOwn = !track.id.isEmpty();
		_heart->setVisible(_heartOwn);
		_heart->setLiked(IsLiked(track.id));
		_seek->setProgress(state.length > 0
			? state.position / float64(state.length)
			: 0.);
		_seek->setDuration(state.length * 1000 / frequency);
		const auto now = FormatSeekTime(state.position / frequency);
		const auto total = FormatSeekTime(state.length / frequency);
		const auto nowChanged = (_timeNow != now);
		const auto totalChanged = (_timeTotal != total);
		if (nowChanged || totalChanged) {
			_timeNow = now;
			_timeTotal = total;
			const auto layout = computeLayout();
			auto dirty = QRegion();
			if (nowChanged) dirty += layout.timeNow;
			if (totalChanged) dirty += layout.timeTotal;
			if (_tab == Tab::Home) {
				_playbackBar->update(dirty.translated(-_playbackBar->pos()));
			} else {
				update(dirty);
			}
		}
		_view->setPosition(
			state.position * 1000 / frequency,
			playing && !state.waitingForData,
			state.id.changeablePlaybackSpeed()
				? Core::App().settings().audioPlaybackSpeed() : 1.);
		if (_queueFor != track.id) {
			_queueFor = track.id;
			refreshQueueViews();
		}
		const auto provider = Settings::Int(u"lyrics.provider"_q);
		if (_lyricsFor != track.id || _lyricsProvider != provider) {
			_lyricsFor = track.id;
			_lyricsProvider = provider;
			if (std::exchange(_lyricsMissing, false) && _tab == Tab::Now) {
				relayout();
				rebuildTab();
			}
			_view->setCommentsTrack(QString());
			ResolveCommentsTrack(track, crl::guard(this, [=, id = track.id](QString sc) {
				if (_lyricsFor == id) _view->setCommentsTrack(sc);
			}));
			_view->setLoading(track.id);
			const auto id = track.id;
			FetchLyrics(
				id,
				track.title,
				track.artist,
				track.durationMs,
				crl::guard(this, [=](Lyrics lyrics) {
					if (_lyricsFor == id && _lyricsProvider == provider) {
						_lyricsMissing = !lyrics.found || lyrics.lines.empty();
						_view->setLyrics(std::move(lyrics));
						if (_tab == Tab::Now) {
							relayout();
							rebuildTab();
						}
					}
				}));
		}
	}

	const not_null<Window::SessionController*> _controller;
	const not_null<LyricsView*> _view;
	const not_null<Ui::ScrollArea*> _tabScroll;
	const not_null<Ui::RpWidget*> _queuePanel;
	const not_null<Ui::ScrollArea*> _queueScroll;
	QGraphicsOpacityEffect *_queueEdgeFade = nullptr;
	Ui::RpWidget *_queueTabBar = nullptr;
	std::array<NavItem*, 2> _queueTabs = {};
	PlayerSpring _queueTabSlide{ [this] { _queueTabBar->update(); } };
	Ui::InputField *_queueSearch = nullptr;
	Ui::VerticalLayout *_queueContent = nullptr;
	Ui::SlideWrap<Ui::FlatLabel> *_queueNowLabel = nullptr;
	Ui::VerticalLayout *_queueNow = nullptr;
	Ui::SlideWrap<Ui::FlatLabel> *_queueNextLabel = nullptr;
	Ui::VerticalLayout *_queueList = nullptr;
	Ui::SlideWrap<Ui::FlatLabel> *_queueEmpty = nullptr;
	int _queueListSlot = 0;
	int _queueAdvanceListShift = 0;
	QPixmap _queueAdvanceExit;
	PlayerSpring _queueAdvance{ [this] { updateQueueAdvance(); } };
	std::unique_ptr<Ui::VerticalLayoutReorder> _queueReorder;
	int _queueLayoutRevision = 0;
	int _queueUpcomingFirst = 0;
	bool _queuePanelRefreshPending = false;
	bool _queueHistory = false;
	bool _queuePanelOpen = false;
	QRect _queuePanelTarget;
	QVariantAnimation _queueSlide;
	double _queueProgress = 0.;
	PlaybackBarColors _playbackColors = MakePlaybackBarColors();
	QColor _playbackBarAccent;
	QColor _coverAccent;
	bool _playbackBarIsNow = false;
	const not_null<Ui::RpWidget*> _playbackBar;
	bool _hasPlaybackTrack = false;
	PlayerSpring _barSlide{ [this] {
		layoutPlaybackBar(computeLayout());
		updateHomeArtistBar();
	} };
	const not_null<SeekBar*> _seek;
	const not_null<RoundGlyph*> _heart;
	const not_null<RoundGlyph*> _collapse;
	NavItem *_homeBack = nullptr;
	base::unique_qptr<Ui::RpWidget> _homeBackOverlay;
	PlayerSpring _homeBackExit{ [this] {
		if (_homeBackExit.animating()) {
			_homeBackOverlay->update();
		} else {
			_homeBackOverlay.reset();
		}
	} };
	Ui::RpWidget *_homeArtistBar = nullptr;
	QGraphicsOpacityEffect *_homeScrollFade = nullptr;
	std::tuple<QSize, int, QRect, int, int> _homeScrollFadeKey;
	QGraphicsOpacityEffect *_homeArtistBarOpacity = nullptr;
	const not_null<RoundGlyph*> _filter;
	base::unique_qptr<Ui::PopupMenu> _filterMenu;
	const not_null<RoundGlyph*> _queueToggle;
	const not_null<VolumeControl*> _volume;
	const not_null<Ui::RpWidget*> _tabTransition;
	const not_null<Ui::RpWidget*> _coverOverlay;
	Ui::PaddingWrap<Ui::VerticalLayout> *_tabContent = nullptr;
	Ui::VerticalLayout *_tabList = nullptr;
	CompactQueueList *_tabQueueList = nullptr;
	QString _tabQueuePlayingId;
	QPointer<HomeAlbumHeader> _homeAlbumHeader;
	std::array<NavItem*, int(Tab::Count)> _nav = {};
	PlayerSpring _navSlide{ [this] { update(computeLayout().nav); } };
	PlayerSpring _tabTransitionSlide{ [this] { updateTabTransition(); } };
	PlayerSpring _tabTransitionFade{ [this] { updateTabTransition(); } };
	QPixmap _tabTransitionWas;
	QPixmap _tabTransitionNow;
	int _tabTransitionDirection = 0;
	PlayerSpring _homeEntrance{ [this] {
		if (_homeEntrance.animating()) {
			static_cast<QGraphicsOpacityEffect*>(_tabScroll->graphicsEffect())
				->setOpacity(_homeEntrance.unitValue(1.));
		} else {
			_tabScroll->setGraphicsEffect(nullptr);
		}
	} };
	QLineEdit *_searchField = nullptr;
	SearchIndicator *_searchIndicator = nullptr;
	QPushButton *_searchRetry = nullptr;
	bool _searchFailed = false;
	bool _searchPending = false;
	std::vector<QWidget*> _searchResultWidgets;
	Ui::RoundButton *_chooseSearchChat = nullptr;
	PeerId _searchChat;
	Segmented *_sourceSwitch = nullptr;
	std::array<QPushButton*, 5> _searchFilters = {};
	std::array<bool, 5> _searchFilterAvailable = {};
	SearchCategory _searchCategory = SearchCategory::All;
	std::vector<CatalogItem> _catalogItems;
	int _searchReturnScroll = 0;
	bool _homeBackToSearch = false;
	QTimer _searchTimer;
	std::unique_ptr<ChatMusicSearch> _chatSearch;
	std::vector<Track> _searchTracks;
	QString _searchStatus;
	int _searchGen = 0;
	bool _searchChats = false;
	bool _lyricsMissing = false;
	Tab _tab;
	std::vector<NavigationPage> _navigationBack;
	std::vector<NavigationPage> _navigationForward;
	bool _restoringNavigation = false;
	std::optional<Tab> _rebuiltTab;
	bool _librarySyncLoading = false;
	crl::time _librarySyncSucceeded = 0;
	bool _playlistsSyncLoading = false;
	crl::time _playlistsSyncSucceeded = 0;
	QString _tabNote;
	QString _openedPlaylist;
	bool _heartOwn = false;
	struct HomePage {
		std::vector<Track> tracks;
		QString artist;
		std::vector<Album> albums;
		std::shared_ptr<int> shelfOffset = std::make_shared<int>(0);
	};
	std::vector<HomePage> _homePages;
	std::vector<Track> _homeTrending;
	Album _homeWeeklyWave;
	Album _homeDailyDrops;
	std::shared_ptr<int> _homeWeeklyWaveOffset = std::make_shared<int>(0);
	std::shared_ptr<int> _homeTrendingOffset = std::make_shared<int>(0);
	std::shared_ptr<int> _homeDailyDropsOffset = std::make_shared<int>(0);
	QString _homeSoundCloudError;
	std::shared_ptr<int> _homeLikedOffset = std::make_shared<int>(0);
	std::set<QString> _homeArtists;
	std::set<QString> _homeAlbumIds;
	std::vector<Track> _homeAlbumTracks;
	std::vector<Track> _homeArtistTracks;
	std::vector<Track> _homeArtistYouTubeTracks;
	bool _homeArtistYouTubeDeferred = false;
	bool _homeArtistYouTubeLoading = false;
	int _homeArtistVisibleTracks = kArtistTrackPageSize;
	std::vector<Album> _homeArtistAlbums;
	QString _homeArtistBannerUrl;
	DiscographyFilter _discographyFilter = DiscographyFilter::All;
	std::shared_ptr<int> _discographyOffset = std::make_shared<int>(0);
	bool _discographyExpanded = false;
	// Похожее по звучанию: отдельный ряд от автора играющего трека.
	std::vector<Track> _homeSimilarTracks;
	bool _homeSimilarLoading = false;
	QColor _homeAccent;
	QColor _homeSideGlow = Qt::transparent;
	QColor _homeSideGlowFrom = Qt::transparent;
	Ui::Animations::Simple _homeSideGlowFade;
	Album _homeAlbum;
	Track _homeArtistSeed;
	ArtistInfo _homeArtistProfile;
	base::unique_qptr<Ui::PopupMenu> _trackArtistsMenu;
	int _artistLinkGeneration = 0;
	QString _homeAlbumNote;
	QString _homeReleaseArtistAvatar;
	QTimer _homeMoreRetryTimer;
	int _homeSection = 0;
	int _homeFeedScroll = 0;
	int _homeAlbumReturnScroll = 0;
	int _homeArtistFeedScroll = 0;
	int _homeAlbumGeneration = 0;
	int _homeMoreFailures = 0;
	bool _homeAlbumOpen = false;
	bool _homeArtistOpen = false;
	bool _homeArtistLikesOpen = false;
	bool _homeReturnToArtist = false;
	bool _homeAlbumIsTrack = false;
	bool _homeAlbumLoading = false;
	bool _homeMoreLoading = false;
	bool _homeMoreCheckQueued = false;
	bool _rebuildingTab = false;
	bool _tabRebuildPending = false;
	bool _homeLoading = false;
	bool _homeLoaded = false;
	bool _homeSoundCloudLoading = false;
	bool _homeSoundCloudLoaded = false;
	int _homeSoundCloudGeneration = 0;
	std::unique_ptr<ChatMusicSearch> _context;
	PeerId _contextPeer;
	QString _queueFor;
	const not_null<RoundGlyph*> _shuffle;
	const not_null<RoundGlyph*> _previous;
	const not_null<RoundGlyph*> _play;
	const not_null<RoundGlyph*> _next;
	const not_null<RoundGlyph*> _repeat;

	CoverBackground _backdrop;
	CanvasPlayer _canvas;
	CanvasPlayer _upcomingCanvas;
	QImage _canvasBackground;
	QImage _canvasBackgroundFrom;
	QColor _canvasAccentFrom;
	QColor _canvasAccentTo;
	PlayerSpring _canvasFade{ [this] {
		repaintAnimatedBackdrop(crl::now(), !_canvasFade.animating());
	} };
	QImage _spotifyPaletteFrame;
	QImage _coverVideoFrame;
	QString _canvasPaletteFor;
	crl::time _canvasPaletteNext = 0;
	QImage _preparedBackdrop;
	QImage _backdropRaster;
	bool _backdropDirty = true;
	double _backgroundBass = 0.;
	double _backgroundPulse = 0.;
	crl::time _animatedBackdropFrame = -1;
	[[nodiscard]] bool animatesBackdrop() const {
		return _isPlaying && _tab == Tab::Now && !anim::Disabled()
			&& Mokagram::Settings::Bool(u"player.dynamic_background"_q);
	}
	void updateAmbientTimer() {
		const auto active = !_closing && isVisible()
			&& QGuiApplication::applicationState() == Qt::ApplicationActive
			&& !PowerSaving::On(PowerSaving::kAnimations);
		const auto homePulse = active && _isPlaying && !anim::Disabled()
			&& _tab == Tab::Home && _homeAlbumOpen && _homeSideGlow.alpha()
			&& HomeSideGlowWidth(width()) > style::ConvertScale(kHomeSideGlowMin)
			&& Mokagram::Settings::Bool(u"player.cover_colors"_q)
			&& Mokagram::Settings::Bool(u"player.dynamic_background"_q);
		if (_homeSideGlowPulsing != homePulse) {
			_homeSideGlowPulsing = homePulse;
			_backgroundBass = _backgroundPulse = 0.;
			repaintHomeSideGlow();
		}
		const auto wave = _seek->waveAnimating();
		const auto needed = active && (animatesBackdrop() || homePulse || wave);
		const auto interval = (wave || homePulse) ? kSeekFrameInterval : kBackdropFrameInterval;
		if (needed && (!_ambientTimer.isActive() || _ambientTimer.interval() != interval)) {
			_ambientAt = crl::now();
			_ambientTimer.start(interval);
		} else if (!needed) {
			_ambientTimer.stop();
		}
	}
	bool _homeSideGlowPulsing = false;
	QTimer _ambientTimer;
	crl::time _ambientAt = 0;
	Media::Clip::ReaderPointer _motionCover;
	Media::Clip::ReaderPointer _playlistsSticker;
	base::unique_qptr<StickerHint> _searchEmptySticker;
	HintStickerMenu _searchHintMenu;
	Media::Clip::ReaderPointer _spotifyCanvas;
	QString _motionFor;
	Fn<void()> _cancelMotion;
	QColor _accent;
	PlayerSpring _fade{ [this] {
		if (_fade.animating()) {
			static_cast<QGraphicsOpacityEffect*>(graphicsEffect())
				->setOpacity(_fade.unitValue(_closing ? 0. : 1.));
		} else if (_closing) {
			hide();
			deleteLater();
		} else {
			setGraphicsEffect(nullptr);
		}
	} };
	PlayerSpring _playAnim{ [this] { update(computeLayout().cover); } };
	PlayerSpring _coverMorph{ [this] { updateCoverTransition(); } };
	CoverTransition _coverTransition;
	bool _coverExpanding = false;
	PlayerSpring _plainAnim{ [this] {
		update();
		_playbackBar->update();
		_queuePanel->update();
		if (!_plainAnim.animating()) updateCoverTitleScroll();
	} };
	PlayerSpring _infoFade{ [this] {
		update();
		_playbackBar->update();
		_coverOverlay->update();
	} };
	QPixmap _cover;
	AudioMsgId _playbackArtworkFor;
	std::shared_ptr<Data::DocumentMedia> _playbackArtworkMedia;
	rpl::lifetime _playbackArtworkLifetime;
	mutable std::array<MovingTitlePath, 2> _titlePaths;
	QTimer _coverTitleTimer;
	std::array<crl::time, 2> _titleElapsed = {};
	crl::time _titleTickAt = 0;
	bool _coverTitleMoves = false;
	bool _barTitleMoves = false;
	QPixmap _coverWas;
	qint64 _coverKey = 0;
	QString _title;
	QString _artist;
	QString _wasTitle;
	QString _wasArtist;
	QString _timeNow = u"0:00"_q;
	QString _timeTotal = u"0:00"_q;
	QString _lyricsFor;
	int _lyricsProvider = 0;
	bool _isPlaying = true;
	bool _closing = false;
	InfoHover _hover = InfoHover::None;

};

[[nodiscard]] QPointer<FullscreenPlayer> &FullscreenInstance() {
	static auto result = QPointer<FullscreenPlayer>();
	return result;
}

bool ToggleTrackLike(
		Track track,
		not_null<Window::SessionController*> controller,
		QWidget *source) {
	if (track.id.isEmpty()) {
		return false;
	}
	const auto fullscreen = FullscreenInstance();
	const auto inFullscreen = fullscreen
		&& fullscreen->canReuse(controller)
		&& source
		&& (source == fullscreen.data() || fullscreen->isAncestorOf(source));
	const auto parent = QPointer<QWidget>(inFullscreen
		? static_cast<QWidget*>(fullscreen.data())
		: controller->widget()->bodyWidget());
	const auto liked = !IsLiked(track.id);
	if (!SetLiked(track, liked)) {
		return false;
	} else if (!liked || !parent || !parent->isVisible()) {
		return true;
	}

	static auto lastToast = base::weak_ptr<Ui::Toast::Instance>();
	if (const auto previous = lastToast.get()) {
		previous->hide();
	}
	const auto s = [](int value) { return style::ConvertScale(value); };
	const auto undo = QString::fromUtf8(Mokagram::Lang::Tr("Отменить лайк"));
	const auto actionWidth = st::defaultLinkButton.font->width(undo);
	lastToast = Ui::Toast::Show(parent.data(), {
		.text = { QString::fromUtf8(Mokagram::Lang::Tr("Трек добавлен в избранное")) },
		.singleline = true,
		.iconLottie = u"toast/mokagram_track_liked"_q,
		.iconLottieSize = QSize(s(28), s(28)),
		.iconPadding = QMargins(s(12), s(8), s(10), s(8)),
		.padding = rpl::single(QMargins(0, 0, actionWidth + s(14), 0)),
		.st = &st::defaultToast,
		.attach = RectPart::Bottom,
		.addToAttachSide = inFullscreen ? fullscreen->likeToastOffset() : rpl::single(0),
		.acceptinput = true,
		.duration = crl::time(4000),
	});
	const auto toast = lastToast.get()->widget();
	const auto action = Ui::CreateChild<Ui::LinkButton>(toast, undo);
	action->setColorOverride(st::mediaviewTextLinkFg->c);
	action->setFocusPolicy(Qt::TabFocus);
	action->resizeToWidth(actionWidth);
	action->setClickedCallback([=] { SetLiked(track, false); });
	action->show();
	toast->sizeValue() | rpl::on_next([=](QSize size) {
		action->moveToRight(st::defaultToast.padding.right(),
			(size.height() - action->height()) / 2, size.width());
	}, toast->lifetime());
	LikesChanged() | rpl::on_next([id = track.id, weak = lastToast] {
		if (const auto active = weak.get(); active && !IsLiked(id)) {
			active->hide();
		}
	}, toast->lifetime());
	return true;
}

void ShowFullscreenPlayerAt(
		not_null<Window::SessionController*> controller,
		bool openHome) {
	BindController(controller);
	auto &instance = FullscreenInstance();
	if (instance && !instance->canReuse(controller)) {
		instance->hide();
		instance->close();
		instance.clear();
	}
	if (!openHome && !instance && PlayingTrack().id.isEmpty()) {
		return;
	}
	// A desktop island can open this screen while the app is inactive or hidden.
	const auto window = controller->widget();
	window->activate();
	if (instance) {
		if (openHome) {
			instance->showHome();
		}
		instance->show();
		instance->raise();
		instance->setFocus();
	} else {
		instance = Ui::CreateChild<FullscreenPlayer>(
			window->bodyWidget(),
			controller,
			openHome);
	}
	window->fixOrder();
}

} // namespace

void ShowFullscreenPlayer(not_null<Window::SessionController*> controller) {
	ShowFullscreenPlayerAt(controller, false);
}

void ShowFullscreenPlayerHome(not_null<Window::SessionController*> controller) {
	ShowFullscreenPlayerAt(controller, true);
}

void Presence::OpenTrack(
		not_null<Window::SessionController*> controller,
		Presence::Track track) {
	if (!track) {
		return;
	}
	if (!track.soundcloudId) {
		ShowMusicBox(controller, track.artist.isEmpty()
			? track.title
			: track.title + u' ' + track.artist);
		return;
	}
	BindController(controller);
	PlayAll({ Music::Track{
		.id = QString::number(track.soundcloudId),
		.title = std::move(track.title),
		.artist = std::move(track.artist),
	} });
	ShowPlayerBox(controller);
}

void DebugGrabFullscreen(const QString &path) {
	if (const auto widget = FullscreenInstance().data()) {
		widget->grab().save(path);
	}
}

QImage CoverForPlayback(const AudioMsgId &current) {
	const auto document = current.audio();
	if (!document) {
		return {};
	} else if (OwnsTrack(current)) {
		return CoverForDocument(document->id);
	} else if (auto hq = CoverForDocument(document->id); !hq.isNull()) {
		return hq;
	}
	if (const auto media = document->activeMediaView()) {
		if (const auto image = media->thumbnail()) {
			return image->original();
		} else if (const auto image = media->thumbnailInline()) {
			return image->original();
		}
	}
	return {};
}

QColor CurrentCoverAccent() {
	if (!Mokagram::Settings::Bool(u"player.cover_colors"_q)) {
		return QColor();
	}
	const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
	if (!current.audio()) {
		return QColor();
	}
	static auto cachedKey = qint64();
	static auto cached = QColor();
	const auto cover = CoverForPlayback(current);
	if (cover.isNull()) {
		return QColor();
	}
	if (cachedKey != cover.cacheKey() || !cached.isValid()) {
		cachedKey = cover.cacheKey();
		cached = ComputeAccent(cover);
	}
	return cached;
}

bool OwnsTrack(const AudioMsgId &id) {
	return id.audio()
		&& (id.audio()->id & 0xFFFFFF0000000000ULL) == 0x4D4F4B0000000000ULL;
}

bool UsesQueue(const AudioMsgId &id) {
	const auto &queue = QueueState();
	if (queue.currentId.isEmpty() || !id.audio()
		|| &id.audio()->session() != queue.session.get()) {
		return false;
	}
	if (OwnsTrack(id)) {
		return !queue.currentId.startsWith(u"telegram:"_q)
			&& id.audio()->id == DocumentIdForTrack(queue.currentId);
	}
	const auto i = queue.messages.find(queue.currentId);
	return i != queue.messages.end()
		&& id.contextId() == i->second;
}

bool NextAvailable() {
	const auto &queue = QueueState();
	const auto &settings = Core::App().settings();
	return !queue.tracks.empty()
		&& (queue.index + 1 < int(queue.tracks.size())
			|| (queue.radio && !queue.radio->exhausted)
			|| (!queue.radio && ((queue.stream && StreamEnabled())
				|| settings.playerRepeatMode() == Media::RepeatMode::All
				|| settings.playerOrderMode() == Media::OrderMode::Shuffle)));
}

bool PreviousAvailable() {
	const auto &queue = QueueState();
	return !queue.tracks.empty()
		&& (queue.index > 0
			|| (!queue.radio && Core::App().settings().playerRepeatMode()
				== Media::RepeatMode::All));
}

[[nodiscard]] bool FeedbackAllowed() {
	const auto &queue = QueueState();
	return !queue.current.id.isEmpty()
		&& !IsTelegramTrack(queue.current)
		&& OwnsTrack(Media::Player::instance()->current(AudioMsgId::Type::Song));
}

// «Больше такого»: усиливает вкус и пересобирает предстоящие авто-треки.
void MoreLikeThis() {
	if (!FeedbackAllowed()) {
		return;
	}
	auto &queue = QueueState();
	BoostTrack(queue.current);
	if (queue.stream && StreamEnabled()) {
		DropUpcoming([](const Track &) { return true; }, queue.index + 1, true);
		++queue.streamEpoch;
		queue.refilling = false;
		RefillStream();
	}
	if (const auto controller = queue.controller.get()) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Учту: больше такого")));
	}
}

// «Не нравится автор»: в чёрный список, из очереди убираем, переключаем дальше.
void DislikeArtist() {
	if (!FeedbackAllowed()) {
		return;
	}
	auto &queue = QueueState();
	const auto track = queue.current;
	BlockArtist(track);
	DropUpcoming([=](const Track &t) {
		return IsArtistBlocked(t);
	}, queue.index, false);
	if (const auto controller = queue.controller.get()) {
		controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Автор скрыт: ")) + track.artist);
	}
	if (NextAvailable()) {
		Step(1);
	}
}

void Step(int delta) {
	auto &queue = QueueState();
	const auto count = int(queue.tracks.size());
	if (!count) {
		return;
	}
	auto target = queue.index + delta;
	if (delta > 0 && (queue.radio || (queue.stream && StreamEnabled()))) {
		if (target >= count) {
			queue.waiting = true;
			RefillStream();
			return;
		}
	} else if (delta > 0
		&& Core::App().settings().playerOrderMode()
			== Media::OrderMode::Shuffle
		&& count > 1) {
		target = queue.index;
		while (target == queue.index) {
			target = base::RandomIndex(count);
		}
	} else if (target >= count) {
		target = 0;
	} else if (target < 0) {
		if (queue.radio) {
			return;
		}
		target = count - 1;
	}
	PlayIndex(target);
}

rpl::producer<> QueueChanges() {
	return QueueState().changes.events();
}

bool CurrentTrackInfo(QString &title, QString &artist) {
	const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
	if (!OwnsTrack(current)) {
		return false;
	}
	const auto track = PlayingTrack();
	title = track.title;
	artist = track.artist;
	return true;
}

// ---- Слушать вместе: хозяин пишет состояние плеера в комнату бота, гости его читают ----

struct Together {
	QString code;
	bool host = false;
	int lastVer = -1;
	QString lastKey;
	base::Timer timer;
	bool busy = false;
	int listeners = 0;
	int lastEvent = 0;
	bool eventsSeen = false;
	bool eventsBusy = false;
	int tickCount = 0;
	// За каким ключом трека мы идём и под каким id он играет у нас.
	QString followKey;
	QString followId;
	bool resolving = false;
	// Окно комнаты.
	std::shared_ptr<Ui::RpWindow> window;
	Ui::VerticalLayout *log = nullptr;
	Ui::ScrollArea *logScroll = nullptr;
	Ui::FlatLabel *people = nullptr;
	Ui::FlatLabel *title = nullptr;
};

[[nodiscard]] Together &TogetherState();
void TogetherLog(const QString &line);

constexpr auto kTogetherPollMs = 700;
constexpr auto kTogetherDriftMs = 700;
constexpr auto kShowTogetherInPlayer = false;

// Сообщает, когда число слушателей изменилось (первый раз молча).
void TogetherNoteListeners(int count) {
	auto &t = TogetherState();
	if (count <= 0 || count == t.listeners) {
		return;
	}
	const auto first = (t.listeners == 0);
	t.listeners = count;
	if (first) {
		return;
	}
	TogetherLog(QString::fromUtf8(Mokagram::Lang::Tr("Слушают вместе: "))
		+ QString::number(count));
}

[[nodiscard]] Together &TogetherState() {
	static auto result = Together();
	return result;
}

[[nodiscard]] bool TogetherActive() {
	return !TogetherState().code.isEmpty();
}

void TogetherStop() {
	auto &t = TogetherState();
	t.timer.cancel();
	t.code.clear();
	t.host = false;
	t.lastVer = -1;
	t.lastKey.clear();
	t.busy = false;
	t.listeners = 0;
	t.lastEvent = 0;
	t.eventsSeen = false;
	t.eventsBusy = false;
	t.tickCount = 0;
	t.followKey.clear();
	t.followId.clear();
	t.resolving = false;
	t.log = nullptr;
	t.logScroll = nullptr;
	t.people = nullptr;
	t.title = nullptr;
	if (const auto window = std::move(t.window)) {
		// Окно удаляем уже вне обработчика его кнопки.
		window->hide();
		crl::on_main([window] {});
	}
}

// Что сейчас играет у нас: ключ трека, позиция мс, играет ли. false: слать нечего.
[[nodiscard]] bool TogetherLocalState(QString &key, qint64 &position, bool &playing) {
	const auto &track = QueueState().current;
	if (track.id.isEmpty()) {
		return false;
	}
	const auto state = Media::Player::instance()->getState(AudioMsgId::Type::Song);
	if (IsTelegramTrack(track)) {
		// Файл из чата у других недоступен: передаём название, исполнителя и длительность.
		const auto meta = track.title.left(40).toUtf8() + '\x1f'
			+ track.artist.left(30).toUtf8() + '\x1f'
			+ QByteArray::number(track.durationMs);
		key = u"m:"_q + QString::fromLatin1(meta.toBase64(
			QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
	} else {
		key = track.id;
	}
	position = state.position * 1000 / std::max<int64>(state.frequency, 1);
	playing = !Media::Player::IsPausedOrPausing(state.state)
		&& !Media::Player::IsStoppedOrStopping(state.state);
	return true;
}

void TogetherHostTick() {
	auto &t = TogetherState();
	QString key;
	qint64 position = 0;
	bool playing = false;
	if (!TogetherActive() || !t.host || t.busy
		|| !TogetherLocalState(key, position, playing)) {
		return;
	}
	t.busy = true;
	const auto code = t.code;
	Mokagram::Bot::Ask(
		u"room:put:%1:%2|%3|%4"_q.arg(code).arg(key).arg(position).arg(playing ? 1 : 0),
		[=](Mokagram::Bot::Results results) {
			TogetherState().busy = false;
			if (!results.empty()) {
				TogetherNoteListeners(results.front().description.toInt());
			}
		},
		[=] { TogetherState().busy = false; });
}

void TogetherGuestApply(const QString &reply, qint64 roundTripMs) {
	auto &t = TogetherState();
	const auto parts = reply.split('|');
	if (parts.size() < 5) {
		return;
	}
	if (parts.size() > 5) {
		TogetherNoteListeners(parts[5].toInt());
	}
	const auto key = parts[1];
	auto position = parts[2].toLongLong();
	const auto playing = (parts[3] == u"1"_q);
	if (playing) {
		position += roundTripMs / 2;
	}
	auto &queue = QueueState();
	if (key.isEmpty() || key.startsWith(u"telegram:"_q)) {
		return;
	}
	const auto instance = Media::Player::instance();
	if (t.followKey != key) {
		// Другой трек: находим и запускаем у себя, подстроимся на следующем шаге.
		if (t.resolving) {
			return;
		}
		t.followKey = key;
		t.followId.clear();
		if (!key.startsWith(u"m:"_q)) {
			// Ключ комнаты превращается в id трека: берём только числа,
			// всё остальное могло прийти от чужого бота.
			const auto trackId = key.toULongLong();
			if (trackId == 0 || QString::number(trackId) != key) {
				return;
			}
			t.followId = key;
			auto track = Track();
			track.id = key;
			PlayAll({ track });
			return;
		}
		const auto meta = QByteArray::fromBase64(
			key.mid(2).toLatin1(),
			QByteArray::Base64UrlEncoding).split('\x1f');
		if (meta.size() < 3) {
			return;
		}
		const auto title = QString::fromUtf8(meta[0]);
		const auto artist = QString::fromUtf8(meta[1]);
		const auto duration = meta[2].toInt();
		t.resolving = true;
		const auto wanted = key;
		Search((artist + u" "_q + title).trimmed(), 0, [=](SearchResult result) {
			auto &t = TogetherState();
			t.resolving = false;
			if (t.followKey != wanted || !TogetherActive()) {
				return;
			}
			auto best = -1;
			auto bestDiff = 0;
			for (auto i = 0; i != int(result.tracks.size()); ++i) {
				const auto diff = std::abs(result.tracks[i].durationMs - duration);
				if (best < 0 || diff < bestDiff) {
					best = i;
					bestDiff = diff;
				}
			}
			if (best < 0) {
				TogetherLog(QString::fromUtf8(Mokagram::Lang::Tr("Не нашёл трек: "))
					+ artist + u" - "_q + title);
				return;
			}
			t.followId = result.tracks[best].id;
			PlayAll({ result.tracks[best] });
		});
		return;
	}
	if (t.followId.isEmpty() || queue.currentId != t.followId) {
		return;
	}
	const auto state = instance->getState(AudioMsgId::Type::Song);
	const auto ours = state.position * 1000 / std::max<int64>(state.frequency, 1);
	const auto weArePlaying = !Media::Player::IsPausedOrPausing(state.state)
		&& !Media::Player::IsStoppedOrStopping(state.state);
	if (playing && !weArePlaying && !Media::Player::IsStoppedOrStopping(state.state)) {
		instance->play(AudioMsgId::Type::Song);
	} else if (!playing && weArePlaying) {
		instance->pause(AudioMsgId::Type::Song);
	}
	// Расхождение больше порога: перематываем к позиции хозяина.
	if (state.length > 0 && std::abs(ours - position) > kTogetherDriftMs) {
		const auto total = state.length * 1000 / std::max<int64>(state.frequency, 1);
		if (total > 0) {
			instance->finishSeeking(
				AudioMsgId::Type::Song,
				std::clamp(position / float64(total), 0., 1.));
		}
	}
}

void TogetherGuestTick() {
	auto &t = TogetherState();
	if (!TogetherActive() || t.host || t.busy) {
		return;
	}
	t.busy = true;
	const auto code = t.code;
	const auto sentAt = crl::now();
	Mokagram::Bot::Ask(
		u"room:get:%1"_q.arg(code),
		[=](Mokagram::Bot::Results results) {
			auto &t = TogetherState();
			t.busy = false;
			if (!TogetherActive() || t.code != code) {
				return;
			}
			if (results.empty() || results.front().title == u"no"_q) {
				if (const auto controller = QueueState().controller.get()) {
					controller->showToast(QString::fromUtf8(
						Mokagram::Lang::Tr("Комната закрыта")));
				}
				TogetherStop();
				return;
			}
			TogetherGuestApply(results.front().title, crl::now() - sentAt);
		},
		[=] { TogetherState().busy = false; });
}

// События комнаты (реакции и сообщения): забираем новые раз в несколько опросов.
void TogetherPollEvents() {
	auto &t = TogetherState();
	if (!TogetherActive() || t.eventsBusy) {
		return;
	}
	t.eventsBusy = true;
	const auto code = t.code;
	Mokagram::Bot::Ask(
		u"room:ev:%1:%2"_q.arg(code).arg(t.lastEvent),
		[=](Mokagram::Bot::Results results) {
			auto &t = TogetherState();
			t.eventsBusy = false;
			if (!TogetherActive() || t.code != code) {
				return;
			}
			const auto show = t.eventsSeen;
			t.eventsSeen = true;
			for (const auto &item : results) {
				const auto parts = item.title.split('|');
				if (parts.size() < 4) {
					continue;
				}
				const auto id = parts[0].toInt();
				if (id <= t.lastEvent) {
					continue;
				}
				t.lastEvent = id;
				if (!show) {
					continue;
				}
				const auto body = parts.mid(3).join('|');
				const auto text = (parts[1] == u"r"_q)
					? parts[2] + u" "_q + body
					: parts[2] + u": "_q + body;
				TogetherLog(text);
			}
		},
		[=] { TogetherState().eventsBusy = false; });
}

void TogetherSay(
		not_null<Window::SessionController*> controller,
		const QString &kind,
		QString body) {
	body = body.simplified().left(120);
	if (!TogetherActive() || body.isEmpty()) {
		return;
	}
	Mokagram::Bot::Ask(
		u"room:say:%1:%2:%3"_q.arg(TogetherState().code).arg(kind).arg(body),
		[=](Mokagram::Bot::Results) {
			// Свои события приходят обратно через опрос, отдельно не показываем.
			TogetherPollEvents();
		},
		[=] {
			controller->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Не удалось отправить")));
		});
}

void TogetherWho(not_null<Window::SessionController*> controller) {
	if (!TogetherActive()) {
		return;
	}
	const auto weak = base::make_weak(controller);
	Mokagram::Bot::Ask(u"room:who:%1"_q.arg(TogetherState().code), [=](Mokagram::Bot::Results results) {
		const auto strong = weak.get();
		if (!strong) {
			return;
		}
		auto names = QStringList();
		for (const auto &item : results) {
			if (item.title == u"none"_q) {
				continue;
			}
			names.push_back(item.title.startsWith('*')
				? item.title.mid(1)
					+ QString::fromUtf8(Mokagram::Lang::Tr(" (хозяин)"))
				: item.title);
		}
		strong->show(Ui::MakeInformBox(
			names.isEmpty()
				? QString::fromUtf8(Mokagram::Lang::Tr("В комнате пока никого не видно"))
				: names.join('\n')));
	}, [=] {
		if (const auto strong = weak.get()) {
			strong->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Не удалось получить список")));
		}
	});
}

void TogetherChatBox(not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8(
			Mokagram::Lang::Tr("Сообщение в комнату"))));
		const auto input = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Текст")))));
		box->setFocusCallback([=] { input->setFocusFast(); });
		const auto submit = [=] {
			TogetherSay(controller, u"m"_q, input->getLastText());
			box->closeBox();
		};
		input->submits() | rpl::on_next(submit, input->lifetime());
		box->addButton(
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Отправить"))),
			submit);
		box->addButton(
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Отмена"))),
			[=] { box->closeBox(); });
	}));
}

void TogetherLog(const QString &line) {
	auto &t = TogetherState();
	if (t.log && t.logScroll) {
		t.log->add(
			object_ptr<Ui::FlatLabel>(t.log, line, st::boxLabel),
			QMargins(
				style::ConvertScale(12),
				style::ConvertScale(4),
				style::ConvertScale(12),
				style::ConvertScale(4)));
		const auto scroll = t.logScroll;
		Ui::PostponeCall(scroll, [=] {
			scroll->scrollToY(scroll->scrollTopMax());
		});
		return;
	}
	if (const auto controller = QueueState().controller.get()) {
		controller->showToast(line);
	}
}

void TogetherRefreshPeople() {
	auto &t = TogetherState();
	if (!TogetherActive() || !t.people) {
		return;
	}
	Mokagram::Bot::Ask(u"room:who:%1"_q.arg(t.code), [](Mokagram::Bot::Results results) {
		auto &t = TogetherState();
		if (!t.people) {
			return;
		}
		auto names = QStringList();
		for (const auto &item : results) {
			if (item.title == u"none"_q) {
				continue;
			}
			names.push_back(item.title.startsWith('*')
				? item.title.mid(1)
					+ QString::fromUtf8(Mokagram::Lang::Tr(" (хозяин)"))
				: item.title);
		}
		t.people->setText(
			QString::fromUtf8(Mokagram::Lang::Tr("Слушают ("))
			+ QString::number(names.size()) + u"): "_q + names.join(u", "_q));
	});
}

void TogetherOpenWindow(not_null<Window::SessionController*> controller) {
	auto &t = TogetherState();
	if (!TogetherActive()) {
		return;
	}
	if (t.window) {
		t.window->showNormal();
		t.window->raise();
		t.window->activateWindow();
		return;
	}
	const auto s = [](int v) { return style::ConvertScale(v); };
	t.window = std::make_shared<Ui::RpWindow>();
	const auto window = t.window.get();
	window->setTitle(QString::fromUtf8(Mokagram::Lang::Tr("Комната ")) + t.code);
	window->setMinimumSize(QSize(s(340), s(480)));
	window->setGeometry(QRect(120, 120, s(380), s(580)));
	const auto body = window->body();
	body->paintRequest() | rpl::on_next([=](QRect clip) {
		QPainter(body).fillRect(clip, QColor(0x1c, 0x1c, 0x1c));
	}, body->lifetime());

	const auto head = Ui::CreateChild<Ui::FlatLabel>(
		body,
		QString::fromUtf8(Mokagram::Lang::Tr("Код комнаты: ")) + t.code,
		st::boxLabel);
	t.title = head;
	t.people = Ui::CreateChild<Ui::FlatLabel>(body, QString(), st::boxLabel);
	const auto scroll = Ui::CreateChild<Ui::ScrollArea>(body, st::defaultScrollArea);
	t.logScroll = scroll;
	t.log = scroll->setOwnedWidget(object_ptr<Ui::VerticalLayout>(scroll));
	const auto input = Ui::CreateChild<Ui::InputField>(
		body,
		st::defaultInputField,
		rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Сообщение"))));
	const auto send = Ui::CreateChild<Ui::RoundButton>(
		body,
		rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Отправить"))),
		st::defaultActiveButton);
	const auto copy = Ui::CreateChild<Ui::RoundButton>(
		body,
		rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Скопировать код"))),
		st::defaultLightButton);
	const auto leave = Ui::CreateChild<Ui::RoundButton>(
		body,
		rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Выйти"))),
		st::defaultLightButton);
	auto reactions = std::vector<Ui::RoundButton*>();
	for (const auto emoji : { "🔥", "❤️", "😂", "👏" }) {
		const auto text = QString::fromUtf8(emoji);
		const auto button = Ui::CreateChild<Ui::RoundButton>(
			body,
			rpl::single(text),
			st::defaultLightButton);
		button->setClickedCallback([=] {
			TogetherSay(controller, u"r"_q, text);
		});
		reactions.push_back(button);
	}
	const auto submit = [=] {
		TogetherSay(controller, u"m"_q, input->getLastText());
		input->setText(QString());
	};
	input->submits() | rpl::on_next(submit, input->lifetime());
	send->setClickedCallback(submit);
	copy->setClickedCallback([=] {
		QGuiApplication::clipboard()->setText(TogetherState().code);
		TogetherLog(QString::fromUtf8(Mokagram::Lang::Tr("Код скопирован")));
	});
	leave->setClickedCallback([=] {
		TogetherStop();
	});

	body->sizeValue() | rpl::on_next([=](QSize size) {
		const auto pad = s(14);
		const auto width = size.width() - 2 * pad;
		auto y = pad;
		head->resizeToWidth(width);
		head->moveToLeft(pad, y);
		y += head->height() + s(6);
		TogetherState().people->resizeToWidth(width);
		TogetherState().people->moveToLeft(pad, y);
		y += TogetherState().people->height() + s(10);
		const auto bottom = s(44) * 3 + s(24);
		scroll->setGeometry(pad, y, width, std::max(size.height() - y - bottom, s(80)));
		scroll->updateBars();
		auto by = size.height() - pad - s(40);
		// Нижний ряд: копировать код и выйти.
		copy->moveToLeft(pad, by);
		leave->moveToRight(pad, by, size.width());
		by -= s(44);
		// Поле ввода и «Отправить».
		send->moveToRight(pad, by, size.width());
		input->resizeToWidth(width - send->width() - s(8));
		input->moveToLeft(pad, by + s(2));
		by -= s(44);
		// Ряд реакций.
		auto x = pad;
		for (const auto button : reactions) {
			button->moveToLeft(x, by);
			x += button->width() + s(6);
		}
		TogetherState().log->resizeToWidth(width);
	}, body->lifetime());

	window->show();
	TogetherRefreshPeople();
}

void TogetherStart(not_null<Window::SessionController*> controller, QString code, bool host) {
	BindController(controller);
	Subscribe();
	TogetherStop();
	auto &t = TogetherState();
	t.code = code;
	t.host = host;
	t.timer.setCallback([] {
		auto &t = TogetherState();
		if (++t.tickCount % 3 == 0) {
			TogetherPollEvents();
		}
		if (t.tickCount % 9 == 0) {
			TogetherRefreshPeople();
		}
		if (TogetherState().host) {
			TogetherHostTick();
		} else {
			TogetherGuestTick();
		}
	});
	t.timer.callEach(kTogetherPollMs);
	if (host) {
		TogetherHostTick();
	} else {
		TogetherGuestTick();
	}
	TogetherOpenWindow(controller);
}

void TogetherCreate(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	Mokagram::Bot::Ask(u"room:new"_q, [=](Mokagram::Bot::Results results) {
		const auto strong = weak.get();
		if (!strong || results.empty() || results.front().title.size() != 6) {
			if (strong) {
				strong->showToast(QString::fromUtf8(
					Mokagram::Lang::Tr("Не удалось создать комнату")));
			}
			return;
		}
		const auto code = results.front().title;
		TogetherStart(strong, code, true);
		QGuiApplication::clipboard()->setText(code);
		strong->showToast(QString::fromUtf8(
			Mokagram::Lang::Tr("Код комнаты скопирован: ")) + code);
	}, [=] {
		if (const auto strong = weak.get()) {
			strong->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Не удалось создать комнату")));
		}
	});
}

void TogetherJoinBox(not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8(
			Mokagram::Lang::Tr("Войти в комнату"))));
		const auto input = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Код комнаты")))));
		box->setFocusCallback([=] { input->setFocusFast(); });
		const auto submit = [=] {
			const auto code = input->getLastText().trimmed().toUpper();
			if (code.size() != 6) {
				input->showError();
				return;
			}
			TogetherStart(controller, code, false);
			box->closeBox();
		};
		input->submits() | rpl::on_next(submit, input->lifetime());
		box->addButton(
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Войти"))),
			submit);
		box->addButton(
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Отмена"))),
			[=] { box->closeBox(); });
	}));
}

void AddTogetherActions(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller) {
	if (!kShowTogetherInPlayer) {
		return;
	}
	if (!menu->empty()) {
		menu->addSeparator();
	}
	const auto add = [&](const QString &text, Fn<void()> callback, const style::icon *icon) {
		menu->addAction(text, std::move(callback), icon);
	};
	if (TogetherActive()) {
		const auto code = TogetherState().code;
		const auto listeners = TogetherState().listeners;
		add(QString::fromUtf8(Mokagram::Lang::Tr("Окно комнаты")), [=] {
			TogetherOpenWindow(controller);
		}, &st::menuIconChats);
		add(QString::fromUtf8(Mokagram::Lang::Tr("Скопировать код комнаты ")) + code, [=] {
			QGuiApplication::clipboard()->setText(code);
			controller->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Код скопирован")));
		}, &st::menuIconLink);
		for (const auto emoji : { "🔥", "❤️", "😂", "👏" }) {
			const auto text = QString::fromUtf8(emoji);
			add(QString::fromUtf8(Mokagram::Lang::Tr("Реакция ")) + text, [=] {
				TogetherSay(controller, u"r"_q, text);
			}, nullptr);
		}
		add(QString::fromUtf8(Mokagram::Lang::Tr("Написать в комнату")), [=] {
			TogetherChatBox(controller);
		}, &st::menuIconChats);
		add(QString::fromUtf8(Mokagram::Lang::Tr("Кто в комнате")), [=] {
			TogetherWho(controller);
		}, &st::menuIconProfile);
		add(QString::fromUtf8(Mokagram::Lang::Tr("Выйти из комнаты"))
			+ (listeners > 0
				? QString::fromUtf8(Mokagram::Lang::Tr(" (слушают: "))
					+ QString::number(listeners) + u")"_q
				: QString()), [=] {
			TogetherStop();
			controller->showToast(QString::fromUtf8(
				Mokagram::Lang::Tr("Вы вышли из комнаты")));
		}, &st::menuIconDelete);
		return;
	}
	add(QString::fromUtf8(Mokagram::Lang::Tr("Слушать вместе: создать комнату")), [=] {
		TogetherCreate(controller);
	}, &st::menuIconAdd);
	add(QString::fromUtf8(Mokagram::Lang::Tr("Слушать вместе: войти по коду")), [=] {
		TogetherJoinBox(controller);
	}, &st::menuIconProfile);
}

void ShowCurrentTrackMenu(
		not_null<Window::SessionController*> controller,
		QPoint globalPosition,
		QWidget *parent,
		bool islandMenu) {
	BindController(controller);
	const auto track = PlayingTrack();
	QueueState().controller = base::make_weak(controller);
	static auto menu = base::unique_qptr<Ui::PopupMenu>();
	menu = base::make_unique_q<Ui::PopupMenu>(
		parent ? parent : controller->widget().get(),
		st::popupMenuWithIcons);
	if (islandMenu) {
		if (!track.id.isEmpty() && CanForwardTrack(track)) {
			menu->addAction(QString::fromUtf8(Mokagram::Lang::Tr("Переслать")), [=] {
				ForwardTrack(controller, track);
			}, &st::menuIconForward);
		}
		menu->addAction(QString::fromUtf8(Mokagram::Lang::Tr("Закрыть")), [] {
			Media::Player::instance()->stopAndClose();
		}, &st::menuIconCancel);
		menu->popup(globalPosition);
		return;
	}
	if (track.id.isEmpty()) {
		AddTogetherActions(menu.get(), controller);
		menu->popup(globalPosition);
		return;
	}
	const auto add = [&](const QString &text, Fn<void()> callback, const style::icon *icon) {
		menu->addAction(text, std::move(callback), icon);
	};
	AddRadioAction(menu.get(), controller, track);
	AddToPlaylistMenu(menu.get(), controller, track);
	const auto instance = Media::Player::instance();
	add(QString::fromUtf8(Mokagram::Lang::Tr("Пауза / играть")), [=] {
		instance->playPause(AudioMsgId::Type::Song);
	}, &st::menuIconSoundOn);
	const auto audioId = instance->current(AudioMsgId::Type::Song);
	if (audioId.changeablePlaybackSpeed()) {
		const auto currentSpeed = Core::App().settings().audioPlaybackSpeed();
		const auto speedText = [](double rate) {
			return QLocale().toString(rate, 'g', 3) + u"×"_q;
		};
		auto submenu = std::make_unique<Ui::PopupMenu>(menu.get(), st::popupMenuWithIcons);
		for (const auto rate : { 0.5, 0.75, 1., 1.25, 1.5, 1.75, 2. }) {
			const auto action = submenu->addAction(speedText(rate), [=] {
				Core::App().settings().setAudioPlaybackSpeed(rate);
				Core::App().saveSettingsDelayed();
				instance->updatePlaybackSpeed();
			});
			action->setCheckable(true);
			action->setChecked(std::abs(rate - currentSpeed) < 0.001);
		}
		if (Media::Audio::SupportsMusicPitchWithSpeed()) {
			const auto key = u"player.pitch_with_speed"_q;
			submenu->addSeparator();
			const auto action = submenu->addAction(
				QString::fromUtf8(Mokagram::Lang::Tr("Менять высоту тона")), [=] {
					Mokagram::Settings::SetBool(key, !Mokagram::Settings::Bool(key));
					const auto type = AudioMsgId::Type::Song;
					const auto state = instance->getState(type);
					if (state.length <= 0 || Media::Player::IsStoppedOrStopping(state.state)
						|| Core::App().settings().audioPlaybackSpeed() == 1.) {
						return;
					}
					const auto progress = std::clamp(
						state.position / float64(state.length), 0., 1.);
					instance->startSeeking(type);
					if (Media::Player::IsPausedOrPausing(state.state)) {
						instance->updateSeeking(type, progress);
						instance->cancelSeeking(type);
					} else {
						instance->finishSeeking(type, progress);
					}
				});
			action->setCheckable(true);
			action->setChecked(Mokagram::Settings::Bool(key));
		}
		menu->addAction(
			QString::fromUtf8(Mokagram::Lang::Tr("Скорость воспроизведения"))
				+ u" · "_q + speedText(currentSpeed),
			std::move(submenu),
			nullptr);
	}
	if (const auto document = audioId.audio()) {
		auto info = std::make_unique<Ui::PopupMenu>(menu.get(), st::popupMenuWithIcons);
		const auto addInfo = [&](const char *label, const QString &value) {
			if (!value.isEmpty()) {
				info->addAction(
					QString::fromUtf8(Mokagram::Lang::Tr(label)) + u" · "_q + value,
					[=] { QGuiApplication::clipboard()->setText(value); });
			}
		};
		const auto format = QFileInfo(document->filename()).suffix().toUpper();
		const auto duration = track.durationMs > 0 ? track.durationMs : document->duration();
		addInfo("Формат", format.isEmpty() ? document->mimeString() : format);
		if (duration > 0) {
			addInfo("Длительность", Ui::FormatDurationText(duration / 1000));
		}
		if (document->size > 0) {
			addInfo("Размер", Ui::FormatSizeText(document->size));
			if (duration > 0) {
				const auto bitrate = qRound64(document->size * 8. / duration);
				addInfo("Средний битрейт",
					QString::fromUtf8(Mokagram::Lang::Tr("%1 кбит/с")).arg(bitrate));
			}
		}
		if (!info->empty()) {
			menu->addAction(
				QString::fromUtf8(Mokagram::Lang::Tr("Сведения о записи")),
				std::move(info),
				nullptr);
		}
	}
	if (UsesQueue(instance->current(AudioMsgId::Type::Song))
		&& PreviousAvailable()) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Предыдущий")), [] { Step(-1); }, &st::menuIconAbove);
	}
	if (UsesQueue(instance->current(AudioMsgId::Type::Song))
		&& NextAvailable()) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Следующий")), [] { Step(1); }, &st::menuIconBelow);
	}
	menu->addSeparator();
	add(IsLiked(track.id)
		? QString::fromUtf8(Mokagram::Lang::Tr("Убрать из избранного"))
		: QString::fromUtf8(Mokagram::Lang::Tr("В избранное")), [=] {
		ToggleTrackLike(track, controller, parent);
	}, &st::menuIconFave);
	if (!IsTelegramTrack(track)) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Скачать")), [=] { SaveToDisk(track); }, &st::menuIconDownload);
	}
	if (CanForwardTrack(track)) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Переслать")), [=] {
			ForwardTrack(controller, track);
		}, &st::menuIconForward);
	}
	if (TrackMessage(track)) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Показать в чате")), [=] {
			if (const auto message = TrackMessage(track)) {
				controller->showPeerHistory(message->history(), {}, message->id);
			}
		}, &st::menuIconChats);
	} else if (!IsTelegramTrack(track)) {
		add(QString::fromUtf8(Mokagram::Lang::Tr("Добавить в профиль")), [=] { AddToProfile(controller, track); }, &st::menuIconProfile);
		add(QString::fromUtf8(Mokagram::Lang::Tr("Копировать ссылку")), [=] {
			QGuiApplication::clipboard()->setText(track.permalink);
			controller->showToast(QString::fromUtf8(Mokagram::Lang::Tr("Ссылка скопирована")));
		}, &st::menuIconLink);
	}
	menu->addSeparator();
	add(QString::fromUtf8(Mokagram::Lang::Tr("Поиск музыки")), [=] { ShowMusicBox(controller); }, &st::menuIconSearch);
	add(QString::fromUtf8(Mokagram::Lang::Tr("Очередь")), [=] { ShowPlayerBox(controller); }, &st::menuIconChats);
	AddTogetherActions(menu.get(), controller);
	menu->popup(globalPosition);
}

void ShowPlayerBox(not_null<Window::SessionController*> controller) {
	BindController(controller);
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		Mokagram::Plugins::RoundedWindow(box);
		const auto content = box->verticalLayout();
		content->setFocusPolicy(Qt::TabFocus);
		const auto instance = Media::Player::instance();
		content->events(
		) | rpl::filter([](not_null<QEvent*> e) {
			if (e->type() == QEvent::KeyPress) {
				const auto key = static_cast<QKeyEvent*>(e.get());
				return key->key() == Qt::Key_Menu
					|| (key->key() == Qt::Key_F10
						&& key->modifiers() == Qt::ShiftModifier);
			}
			return e->type() == QEvent::ContextMenu;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			e->accept();
			if (e->type() == QEvent::KeyPress) {
				ShowCurrentTrackMenu(controller,
					content->mapToGlobal(content->rect().center()));
				return;
			}
			const auto event = static_cast<QContextMenuEvent*>(e.get());
			ShowCurrentTrackMenu(controller,
				event->reason() == QContextMenuEvent::Keyboard
					? content->mapToGlobal(content->rect().center())
					: event->globalPos());
		}, box->lifetime());

		// Смена трека: старая обложка с названием плавно растворяется в новой.
		struct InfoFade {
			Ui::Animations::Simple animation;
			QPixmap was;
			QPixmap last;
		};
		const auto fade = box->lifetime().make_state<InfoFade>();
		using namespace PlayerStyle;
		const auto s = [](int value) { return style::ConvertScale(value); };
		const auto pad = s(kSpace16);
		const auto inset = s(kSpace24);
		const auto actionSize = s(40);

		const auto header = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(pad, pad, pad, 0));
		header->resize(header->width(), s(40));
		const auto close = Mokagram::Plugins::MakeHeaderButton(
			header,
			true,
			[=] { box->closeBox(); });
		const auto title = Ui::CreateChild<Ui::FlatLabel>(
			header,
			QString::fromUtf8(Mokagram::Lang::Tr("Сейчас играет")),
			st::boxTitle);
		const auto searchButton = Mokagram::Plugins::MakeHeaderButton(
			header,
			false,
			[=] {
				box->closeBox();
				ShowMusicBox(controller);
			});
		header->widthValue(
		) | rpl::on_next([=](int width) {
			close->move(0, (header->height() - close->height()) / 2);
			title->move(
				(width - title->width()) / 2,
				(header->height() - title->height()) / 2);
			searchButton->move(
				width - searchButton->width(),
				(header->height() - searchButton->height()) / 2);
		}, header->lifetime());

		// Обложка, название, исполнитель.
		const auto info = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(0, s(kSpace8), 0, 0));
		const auto coverRect = [=] {
			const auto side = std::clamp(info->width() - 2 * inset, 0, s(240));
			return QRect((info->width() - side) / 2, s(kSpace8), side, side);
		};
		const auto titleTop = [=] {
			const auto cover = coverRect();
			return cover.y() + cover.height() + inset;
		};
		const auto actionTop = [=] {
			const auto textHeight = kTrackTitle.lineHeight() + s(kSpace2) + kTrackArtist.lineHeight();
			return titleTop() + (textHeight - actionSize) / 2;
		};
		const auto metadataWidth = [=](const AudioMsgId &id) {
			const auto actions = OwnsTrack(id) ? 4 : 2;
			return std::max(info->width() - 2 * inset - actions * actionSize - s(kSpace8), 0);
		};
		info->widthValue(
		) | rpl::on_next([=](int width) {
			const auto bottom = titleTop() + kTrackTitle.lineHeight()
				+ s(kSpace2) + kTrackArtist.lineHeight() + pad;
			info->resize(width, std::max(bottom, coverRect().height()
				+ st::mokagramMusicPlayerInfoTextHeight + s(kSpace12)));
		}, info->lifetime());
		const auto paintInfo = [=](QPainter &p) {
			auto hq = PainterHighQualityEnabler(p);
			const auto art = coverRect();
			const auto current = instance->current(AudioMsgId::Type::Song);
			const auto cover = CoverForPlayback(current);
			auto path = QPainterPath();
			path.addRoundedRect(art, 8, 8);
			if (!cover.isNull()) {
				p.save();
				p.setClipPath(path);
				const auto crop = std::min(cover.width(), cover.height());
				p.drawImage(art, cover, QRect(
					(cover.width() - crop) / 2,
					(cover.height() - crop) / 2,
					crop,
					crop));
				p.restore();
			} else {
				p.setPen(Qt::NoPen);
				p.setBrush(st::windowBgOver);
				p.drawPath(path);
				st::menuIconSoundOn.paintInCenter(p, art);
			}
			const auto track = PlayingTrack();
			const auto big = PlayerStyle::kTrackTitle.musicFont();
			p.setFont(big);
			p.setPen(st::windowFg);
			const auto textWidth = metadataWidth(current);
			p.drawText(
				QRect(inset, titleTop(), textWidth, kTrackTitle.lineHeight()),
				Qt::AlignLeft | Qt::AlignVCenter,
				QFontMetrics(big).elidedText(track.title, Qt::ElideRight, textWidth));
		};
		info->paintOn([=](QPainter &p) {
			if (fade->animation.animating() && !fade->was.isNull()) {
				p.drawPixmap(0, 0, fade->was);
				p.setOpacity(fade->animation.value(1.));
			}
			paintInfo(p);
		});
		const auto renderInfo = [=] {
			const auto ratio = style::DevicePixelRatio();
			auto result = QPixmap(info->size() * ratio);
			result.setDevicePixelRatio(ratio);
			result.fill(Qt::transparent);
			auto p = QPainter(&result);
			paintInfo(p);
			p.end();
			return result;
		};

		const auto artistButton = Ui::CreateChild<Ui::AbstractButton>(info);
		const auto artistHint = QString::fromUtf8(
			Mokagram::Lang::Tr("Найти исполнителя на SoundCloud"));
		artistButton->setToolTip(artistHint);
		artistButton->setAccessibleName(artistHint);
		artistButton->paintOn([=](QPainter &p) {
			const auto active = artistButton->isOver() || artistButton->isDown();
			const auto font = PlayerStyle::kTrackArtist.musicFont(active);
			p.setFont(font);
			p.setPen(active
				? st::windowFg
				: st::windowSubTextFg);
			p.drawText(
				artistButton->rect(),
				Qt::AlignLeft | Qt::AlignVCenter,
				QFontMetrics(font).elidedText(PlayingTrack().artist, Qt::ElideRight, artistButton->width()));
		});
		const auto updateArtistButton = [=] {
			const auto current = instance->current(AudioMsgId::Type::Song);
			const auto textWidth = metadataWidth(current);
			const auto font = QFontMetrics(kTrackArtist.musicFont());
			const auto artistWidth = std::min(textWidth, font.horizontalAdvance(PlayingTrack().artist));
			artistButton->setGeometry(
				inset,
				titleTop() + kTrackTitle.lineHeight() + s(kSpace2),
				artistWidth,
				kTrackArtist.lineHeight());
			const auto hasArtist = !PlayingTrack().artist.trimmed().isEmpty();
			artistButton->setDisabled(!hasArtist);
			artistButton->setPointerCursor(hasArtist);
			artistButton->update();
		};
		info->widthValue(
		) | rpl::on_next(updateArtistButton, artistButton->lifetime());
		artistButton->setClickedCallback([=] {
			const auto artist = PlayingTrack().artist.trimmed();
			if (artist.isEmpty()) {
				return;
			}
			QueueState().telegramSearch = false;
			QueueState().tab = 0;
			box->closeBox();
			ShowMusicBox(controller, artist);
		});

		const auto heart = Ui::CreateChild<RoundGlyph>(info, Glyph::Add, actionSize);
		info->widthValue(
		) | rpl::on_next([=](int width) {
			heart->move(width - inset - 2 * actionSize, actionTop());
		}, heart->lifetime());
		const auto more = Ui::CreateChild<RoundGlyph>(info, Glyph::More, actionSize);
		const auto block = Ui::CreateChild<RoundGlyph>(info, Glyph::Block, actionSize);
		more->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Больше такого")));
		block->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Не нравится автор")));
		info->widthValue(
		) | rpl::on_next([=](int width) {
			more->move(width - inset - 3 * actionSize, actionTop());
			block->move(width - inset - 4 * actionSize, actionTop());
		}, more->lifetime());
		more->setClickedCallback([=] { MoreLikeThis(); });
		block->setClickedCallback([=] { DislikeArtist(); });

		// Перемотка и время.
		const auto seek = content->add(
			object_ptr<SeekBar>(content),
			QMargins(pad, s(kSpace8), pad, 0));
		const auto times = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(inset, 0, inset, 0));
		times->resize(times->width(), PlayerStyle::kTimeLabel.lineHeight());
		const auto position = std::make_shared<std::pair<QString, QString>>();
		times->paintOn([=](QPainter &p) {
			p.setFont(PlayerStyle::kTimeLabel.musicFont());
			p.setPen(st::windowSubTextFg);
			p.drawText(times->rect(), Qt::AlignLeft | Qt::AlignVCenter, position->first);
			p.drawText(times->rect(), Qt::AlignRight | Qt::AlignVCenter, position->second);
		});

		// Кнопки.
		const auto controls = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(pad, s(kSpace12), pad, s(kSpace12)));
		controls->resize(controls->width(), 80);
		const auto previous = Ui::CreateChild<RoundGlyph>(controls, Glyph::Previous, 44);
		const auto play = Ui::CreateChild<RoundGlyph>(controls, Glyph::Play, 72);
		const auto next = Ui::CreateChild<RoundGlyph>(controls, Glyph::Next, 44);
		const auto shuffle = Ui::CreateChild<RoundGlyph>(controls, Glyph::Shuffle, 44);
		const auto repeat = Ui::CreateChild<RoundGlyph>(controls, Glyph::Repeat, 44);
		previous->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Предыдущий")));
		play->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Пауза / играть")));
		next->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Следующий")));
		shuffle->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Перемешать")));
		repeat->setToolTip(QString::fromUtf8(Mokagram::Lang::Tr("Повтор")));
		controls->widthValue(
		) | rpl::on_next([=](int width) {
			const auto center = width / 2;
			const auto step = (width - 44) / 4;
			play->move(center - 36, 4);
			previous->move(center - step - 22, 18);
			next->move(center + step - 22, 18);
			shuffle->move(0, 18);
			repeat->move(width - 44, 18);
		}, controls->lifetime());
		shuffle->setClickedCallback([=] {
			auto &settings = Core::App().settings();
			settings.setPlayerOrderMode(
				settings.playerOrderMode() == Media::OrderMode::Shuffle
					? Media::OrderMode::Default
					: Media::OrderMode::Shuffle);
			Core::App().saveSettingsDelayed();
		});
		repeat->setClickedCallback([=] {
			auto &settings = Core::App().settings();
			const auto mode = settings.playerRepeatMode();
			settings.setPlayerRepeatMode(mode == Media::RepeatMode::None
				? Media::RepeatMode::All
				: mode == Media::RepeatMode::All
				? Media::RepeatMode::One
				: Media::RepeatMode::None);
			Core::App().saveSettingsDelayed();
		});
		Core::App().settings().playerOrderModeValue(
		) | rpl::on_next([=] { shuffle->update(); }, shuffle->lifetime());
		Core::App().settings().playerRepeatModeValue(
		) | rpl::on_next([=] { repeat->update(); }, repeat->lifetime());

		const auto share = Ui::CreateChild<Ui::AbstractButton>(info);
		share->resize(actionSize, actionSize);
		share->setPointerCursor(true);
		const auto shareHint = QString::fromUtf8(Mokagram::Lang::Tr("Переслать"));
		share->setToolTip(shareHint);
		share->setAccessibleName(shareHint);
		share->paintOn([=](QPainter &p) {
			auto hq = PainterHighQualityEnabler(p);
			if (share->isOver() || share->isDown()) {
				p.setPen(Qt::NoPen);
				p.setBrush(st::windowBgOver);
				p.drawEllipse(share->rect());
			}
			st::menuIconForward.paintInCenter(p, share->rect());
		});
		info->widthValue(
		) | rpl::on_next([=](int width) {
			share->move(width - inset - share->width(), actionTop());
		}, share->lifetime());
		share->setClickedCallback([=] {
			const auto track = PlayingTrack();
			if (!track.id.isEmpty() && CanForwardTrack(track)) {
				ForwardTrack(controller, track);
			}
		});

		const auto step = [=](int delta) {
			if (OwnsTrack(instance->current(AudioMsgId::Type::Song))) {
				Step(delta);
			} else if (delta > 0) {
				instance->next(AudioMsgId::Type::Song);
			} else {
				instance->previous(AudioMsgId::Type::Song);
			}
		};
		heart->setClickedCallback([=] {
			const auto track = PlayingTrack();
			if (!track.id.isEmpty()) {
				const auto liked = !IsLiked(track.id);
				if (ToggleTrackLike(track, controller, heart)) {
					heart->animateLike(liked);
				}
			}
		});
		previous->setClickedCallback([=] { step(-1); });
		next->setClickedCallback([=] { step(1); });
		play->setClickedCallback([=] {
			instance->playPause(AudioMsgId::Type::Song);
		});
		seek->sought(
		) | rpl::on_next([=](float64 progress) {
			instance->startSeeking(AudioMsgId::Type::Song);
			instance->finishSeeking(AudioMsgId::Type::Song, progress);
		}, seek->lifetime());

		struct State {
			AudioMsgId current;
			std::shared_ptr<Data::DocumentMedia> media;
			rpl::lifetime downloadLifetime;
			qint64 coverKey = 0;
		};
		const auto playback = box->lifetime().make_state<State>();
		const auto refresh = [=] {
			const auto current = instance->current(AudioMsgId::Type::Song);
			const auto trackChanged = playback->current != current;
			if (trackChanged) {
				playback->current = current;
				playback->downloadLifetime.destroy();
				playback->media = nullptr;
				const auto document = current.audio();
				if (document && !OwnsTrack(current)) {
					playback->media = document->createMediaView();
					document->session().downloaderTaskFinished(
					) | rpl::on_next([=] {
						info->update();
					}, playback->downloadLifetime);
					playback->media->thumbnailWanted(current.contextId());
				}
			}
			const auto state = instance->getState(AudioMsgId::Type::Song);
			const auto frequency = std::max<int64>(state.frequency, 1);
			const auto length = state.length;
			seek->setProgress(length > 0 ? state.position / float64(length) : 0.);
			const auto fmt = [](int64 seconds) {
				return u"%1:%2"_q.arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
			};
			const auto text = std::pair{
				fmt(state.position / frequency),
				fmt(length / frequency),
			};
			if (*position != text) {
				*position = text;
				times->update();
			}
			const auto coverKey = CoverForPlayback(current).cacheKey();
			if (trackChanged || playback->coverKey != coverKey) {
				playback->coverKey = coverKey;
				if (!fade->last.isNull()
					&& !PowerSaving::On(PowerSaving::kAnimations)) {
					fade->was = fade->last;
					fade->animation.start(
						[=] { info->update(); },
						0.,
						1.,
						kCoverFadeDuration,
						anim::easeOutCubic);
				}
				fade->last = renderInfo();
				info->update();
			}
			const auto own = OwnsTrack(current);
			updateArtistButton();
			const auto track = PlayingTrack();
			share->setVisible(!track.id.isEmpty());
			share->setDisabled(!CanForwardTrack(track));
			heart->setVisible(!track.id.isEmpty());
			more->setVisible(own);
			block->setVisible(own);
			heart->setLiked(IsLiked(track.id));
			play->setPlaying(state.state == Media::Player::State::Playing
				|| state.state == Media::Player::State::Starting
				|| state.state == Media::Player::State::Resuming);
		};
		instance->updatedNotifier(
		) | rpl::on_next(refresh, box->lifetime());
		QueueState().changes.events(
		) | rpl::on_next(refresh, box->lifetime());
		CoversChanged() | rpl::on_next(refresh, box->lifetime());
		LikesChanged() | rpl::on_next(refresh, box->lifetime());
		refresh();

		// Очередь: живой список, меняется при добавлении, перемещении и смене трека.
		content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				QString::fromUtf8(Mokagram::Lang::Tr("Очередь")),
				st::defaultSubsectionTitle),
			QMargins(inset, inset, inset, s(kSpace8)));
		const auto queueList = content->add(object_ptr<Ui::VerticalLayout>(content));
		const auto empty = content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				QString::fromUtf8(Mokagram::Lang::Tr("Очередь пуста. Найдите музыку и выберите «Играть следующим» или «В конец очереди» в меню трека.")),
				st::boxLabel),
			QMargins(inset, s(kSpace4), inset, s(kSpace8)));
		const auto rebuild = [=] {
			queueList->clear();
			const auto &queue = QueueState();
			empty->setVisible(queue.tracks.empty());
			for (auto i = 0; i != int(queue.tracks.size()); ++i) {
				queueList->add(object_ptr<Row>(
					queueList,
					controller,
					queue.tracks[i],
					i,
					RowMode::Queue));
			}
		};
		rebuild();
		QueueState().changes.events(
		) | rpl::on_next(rebuild, box->lifetime());
		Ui::AddSkip(content, 10);
	}));
}

void DebugPlay(int index) {
	PlayFromResults(index);
}

void DebugPlayYouTube(
		not_null<Window::SessionController*> controller,
		const QString &videoId) {
	if (videoId.isEmpty()) {
		return;
	}
	BindController(controller);
	PlayAll({ Track{
		.id = u"yt:"_q + videoId,
		.title = u"YouTube Music"_q,
		.artist = u"Debug"_q,
		.videoId = videoId,
		.durationMs = 0,
		.unavailable = true,
	} });
}

void DebugLike(int index) {
	const auto &queue = QueueState();
	if (index >= 0 && index < int(queue.results.size())) {
		SetLiked(queue.results[index], !IsLiked(queue.results[index].id));
	}
}

void DebugTab(int tab) {
	QueueState().tab = tab;
	if (const auto controller = QueueState().controller.get()) {
		ShowMusicBox(controller);
	}
}

void DebugSaveTrack(int index) {
	const auto &queue = QueueState();
	if (index >= 0 && index < int(queue.tracks.size())) {
		SaveToDisk(queue.tracks[index]);
	}
}

namespace {

void ShowMusicBoxWith(
		not_null<Window::SessionController*> controller,
		const QString &initialQuery,
		bool albumsMode,
		const QString &albumsTrackId) {
	BindController(controller);
	if (albumsMode) {
		// Альбомы есть только у SoundCloud.
		QueueState().telegramSearch = false;
		QueueState().tab = 0;
	}
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		Mokagram::Plugins::RoundedWindow(box);
		const auto content = box->verticalLayout();

		// Шапка: закрыть слева, название по центру.
		const auto header = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(10, 10, 10, 0));
		header->resize(header->width(), 40);
		const auto close = Mokagram::Plugins::MakeHeaderButton(
			header,
			true,
			[=] { box->closeBox(); });
		const auto title = Ui::CreateChild<Ui::FlatLabel>(
			header,
			QString::fromUtf8(Mokagram::Lang::Tr("Музыка")),
			st::boxTitle);
		const auto queueButton = Ui::CreateChild<Ui::RoundButton>(
			header,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Очередь"))),
			st::defaultLightButton);
		queueButton->setClickedCallback([=] {
			box->closeBox();
			ShowPlayerBox(controller);
		});
		header->widthValue(
		) | rpl::on_next([=](int width) {
			close->move(0, (header->height() - close->height()) / 2);
			title->move(
				(width - title->width()) / 2,
				(header->height() - title->height()) / 2);
			queueButton->move(
				width - queueButton->width(),
				(header->height() - queueButton->height()) / 2);
		}, header->lifetime());

		// Поиск: круглая плашка с полем.
		static const auto clear = style::owned_color(QColor(0, 0, 0, 0));
		static const auto fieldStyle = [] {
			auto result = st::defaultInputField;
			result.textBg = clear.color();
			result.textBgActive = clear.color();
			const auto pad = std::max(0, (44 - st::defaultInputField.style.font->height) / 2);
			result.textMargins = style::margins(0, pad, 0, pad);
			result.placeholderFgActive = st::windowSubTextFg;
			result.placeholderScale = 0.;
			result.placeholderShift = 0;
			result.placeholderFont = st::normalFont;
			result.border = 0;
			result.borderActive = 0;
			result.heightMin = 44;
			return result;
		}();
		const auto searchWrap = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(16, 14, 16, 6));
		searchWrap->resize(searchWrap->width(), 44);
		searchWrap->paintOn([=](QPainter &p) {
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBg);
			p.drawRoundedRect(searchWrap->rect(), 22, 22);
		});
		const auto input = Ui::CreateChild<Ui::InputField>(
			searchWrap,
			fieldStyle,
			rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Поиск на SoundCloud"))));
		searchWrap->widthValue(
		) | rpl::on_next([=](int width) {
			input->setGeometry(18, 0, width - 36, 44);
		}, searchWrap->lifetime());
		const auto sources = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(16, 0, 16, 0));
		sources->resize(sources->width(), 48);
		const auto sourceSwitch = Ui::CreateChild<SourceSwitch>(
			sources,
			u"SoundCloud"_q,
			QString::fromUtf8(Mokagram::Lang::Tr("Из чатов")));
		sources->widthValue(
		) | rpl::on_next([=](int width) {
			sourceSwitch->setGeometry(0, 4, width, 40);
		}, sources->lifetime());
		const auto chooseChat = content->add(
			object_ptr<Ui::RoundButton>(content,
				rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Выбрать чат"))),
				st::defaultLightButton),
			QMargins(16, 0, 16, 4),
			style::al_justify);
		chooseChat->resize(chooseChat->width(), 36);

		// Что искать на SoundCloud: треки или альбомы.
		const auto kinds = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(16, 0, 16, 0));
		kinds->resize(kinds->width(), 44);
		const auto kindSwitch = Ui::CreateChild<SourceSwitch>(
			kinds,
			QString::fromUtf8(Mokagram::Lang::Tr("Треки")),
			QString::fromUtf8(Mokagram::Lang::Tr("Альбомы")));
		kinds->widthValue(
		) | rpl::on_next([=](int width) {
			kindSwitch->setGeometry(0, 4, width, 36);
		}, kinds->lifetime());
		struct AlbumsState {
			bool active = false;
			std::vector<Album> found;
			std::optional<Album> opened;
			std::vector<Track> openedTracks;
			QString lastQuery;
			QString trackId;
			int generation = 0;
		};
		const auto albums = box->lifetime().make_state<AlbumsState>();
		albums->active = albumsMode;
		albums->trackId = albumsTrackId;
		kindSwitch->setChats(albumsMode);
		kindSwitch->finishAnimating();

		// Вкладки: поиск, лайкнутые, подборка.
		const auto tabs = content->add(
			object_ptr<Ui::RpWidget>(content),
			QMargins(16, 4, 16, 6));
		tabs->resize(tabs->width(), 40);
		// Дорожка и плашка как у переключателей выше: плашка едет к выбранной
		// вкладке и слегка вытягивается на ходу.
		struct TabsSlide {
			Ui::Animations::Simple animation;
			int index = 0;
			int from = 0;
			int count = 3;
		};
		const auto slide = box->lifetime().make_state<TabsSlide>();
		tabs->paintRequest(
		) | rpl::on_next([=](QRect) {
			auto p = QPainter(tabs);
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgOver);
			p.drawRoundedRect(tabs->rect(), tabs->height() / 2., tabs->height() / 2.);
			const auto margin = 4;
			const auto part = (tabs->width() - 2 * margin) / double(slide->count);
			const auto at = slide->animation.value(slide->index);
			const auto distance = std::abs(slide->index - slide->from);
			const auto progress = distance
				? std::clamp(std::abs(at - slide->from) / distance, 0., 1.)
				: 1.;
			const auto moving = part * 0.18 * (1. - std::abs(2. * progress - 1.));
			const auto selected = QRectF(
				margin + part * at - moving / 2,
				margin,
				part + moving,
				tabs->height() - 2 * margin);
			p.setBrush(st::windowActiveTextFg->c);
			p.drawRoundedRect(selected, selected.height() / 2., selected.height() / 2.);
		}, tabs->lifetime());
		const auto tabSearch = Ui::CreateChild<TabPill>(tabs, QString::fromUtf8(Mokagram::Lang::Tr("Поиск")));
		const auto tabLiked = Ui::CreateChild<TabPill>(tabs, QString::fromUtf8(Mokagram::Lang::Tr("Лайкнутые")));
		const auto tabForYou = Ui::CreateChild<TabPill>(tabs, QString::fromUtf8(Mokagram::Lang::Tr("Для вас")));
		tabs->widthValue(
		) | rpl::on_next([=](int width) {
			// Кнопки лежат ровно на сегментах плашки, чтобы подпись была по её центру.
			const auto part = (width - 8) / 3.;
			const auto at = [&](int i) { return 4 + int(base::SafeRound(part * i)); };
			tabSearch->setGeometry(
				QueueState().telegramSearch ? 0 : at(0),
				0,
				QueueState().telegramSearch ? width : at(1) - at(0),
				tabs->height());
			tabLiked->setGeometry(at(1), 0, at(2) - at(1), tabs->height());
			tabForYou->setGeometry(at(2), 0, at(3) - at(2), tabs->height());
		}, tabs->lifetime());

		const auto list = content->add(object_ptr<Ui::VerticalLayout>(content));
		Ui::AddSkip(content, 10);

		// Смена раздела: старое содержимое гаснет и уезжает в сторону,
		// новое проявляется и въезжает с другой стороны.
		struct Transition {
			Ui::Animations::Simple animation;
			QPixmap was;
			QPixmap now;
			int top = 0;
			int direction = 0;
			bool changing = false;
		};
		const auto transition = box->lifetime().make_state<Transition>();
		const auto overlay = Ui::CreateChild<Ui::RpWidget>(content);
		overlay->hide();
		const auto transitionRect = [=] {
			const auto bottom = list->y() + list->height();
			return QRect(
				0,
				transition->top,
				content->width(),
				std::max(bottom - transition->top, 1));
		};
		const auto grabNow = [=] {
			overlay->hide();
			const auto rect = transitionRect();
			transition->now = Ui::GrabWidget(content, rect);
			overlay->setGeometry(rect);
			// Вкладки живут поверх перехода, чтобы плашка ехала по-настоящему.
			overlay->setMask(QRegion(overlay->rect()).subtracted(
				QRegion(tabs->geometry().translated(-rect.topLeft()))));
			overlay->show();
			overlay->raise();
		};
		overlay->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(overlay);
			const auto progress = transition->animation.value(1.);
			const auto shift = 28. * transition->direction;
			p.fillRect(overlay->rect(), st::boxBg);
			p.setOpacity(1. - progress);
			p.drawPixmap(int(base::SafeRound(-shift * progress)), 0, transition->was);
			p.setOpacity(progress);
			p.drawPixmap(int(base::SafeRound(shift * (1. - progress))), 0, transition->now);
		}, overlay->lifetime());
		const auto animateChange = [=](int top, int direction, Fn<void()> change) {
			if (transition->changing) {
				change();
				return;
			}
			overlay->hide();
			transition->top = top;
			auto was = Ui::GrabWidget(content, transitionRect());
			// Старые вкладки в снимке не нужны: они остаются живыми.
			{
				auto p = QPainter(&was);
				p.fillRect(
					tabs->geometry().translated(0, -top),
					st::boxBg);
			}
			transition->changing = true;
			change();
			transition->changing = false;
			transition->was = std::move(was);
			transition->direction = direction;
			grabNow();
			transition->animation.start([=] {
				overlay->update();
				if (!transition->animation.animating()) {
					overlay->hide();
					transition->was = transition->now = QPixmap();
				}
			}, 0., 1., kSwitchDuration, anim::easeOutCubic);
		};
		// Если список обновился прямо во время перехода, подменяем новый кадр.
		const auto refreshTransition = [=] {
			if (!transition->changing && transition->animation.animating()) {
				grabNow();
			}
		};
		const auto statusText = std::make_shared<QString>();
		const auto statusLoading = std::make_shared<bool>(false);
		const auto statusSticker = std::make_shared<bool>(false);
		const auto refreshForYou = std::make_shared<Fn<void()>>();
		struct SearchState {
			std::unique_ptr<ChatMusicSearch> chats;
			std::unique_ptr<ChatMusicSearch> extraChat;
			int generation = 0;
		};
		const auto searchState = box->lifetime().make_state<SearchState>();
		const auto runSearch = std::make_shared<Fn<void()>>();

		const auto hintStickerMenu = box->lifetime().make_state<HintStickerMenu>();
		const auto showHintMenu = [=](QPoint global) {
			const auto outer = box->getDelegate()->outerContainer();
			if (outer) {
				hintStickerMenu->show(box, outer.data(), controller, global);
			}
		};

		// Список: сверху строка действий (для подборки) и пояснение, дальше треки.
		const auto openAlbum = std::make_shared<Fn<void(Album)>>();
		const auto closeAlbum = std::make_shared<Fn<void()>>();
		const Fn<void()> fill = [=] {
			list->clear();
			const auto &queue = QueueState();
			const auto albumsShown = albums->active
				&& queue.tab == 0
				&& !queue.telegramSearch;
			if (albumsShown && albums->opened) {
				list->add(
					object_ptr<AlbumBar>(
						list,
						albums->opened->title,
						[=] { crl::on_main(box, [=] { (*closeAlbum)(); }); },
						[=] { PlayAll(albums->openedTracks, true); }),
					QMargins(10, 0, 16, 6));
			}
			if (queue.tab == 1 && !queue.telegramSearch) {
				list->add(
					object_ptr<SoundCloudAccountBar>(
						list,
						[=] {
							ShowSoundCloudLogin(nullptr);
						},
						[=] {
							Mokagram::Music::Account::Logout();
						}),
					QMargins(10, 4, 10, 10));
			}
			if (queue.tab == 2 && !queue.forYou.empty()) {
				list->add(
					object_ptr<PicksBar>(
						list,
						[=] { PlayAll(QueueState().forYou); },
						[=] {
							QueueState().forYou.clear();
							QueueState().results.clear();
							(*refreshForYou)();
						}),
					QMargins(16, 4, 16, 10));
			}
			if (*statusSticker) {
				list->add(object_ptr<StickerHint>(
					list,
					&controller->session(),
					showHintMenu));
			} else if (!statusText->isEmpty() && *statusLoading) {
				list->add(object_ptr<LoadingView>(list, *statusText));
			} else if (!statusText->isEmpty()) {
				list->add(
					object_ptr<Ui::FlatLabel>(list, *statusText, st::boxLabel),
					QMargins(24, 8, 24, 8));
			}
			if (albumsShown && !albums->opened) {
				for (const auto &album : albums->found) {
					list->add(object_ptr<AlbumRow>(list, controller, album, [=] {
						crl::on_main(box, [=] { (*openAlbum)(album); });
					}));
				}
			}
			for (auto i = 0; i != int(queue.results.size()); ++i) {
				list->add(object_ptr<Row>(list, controller, queue.results[i], i));
			}
			const auto activeChatSearch = queue.selectedChat
				? searchState->extraChat.get()
				: searchState->chats.get();
			if (queue.tab == 0 && queue.telegramSearch
				&& activeChatSearch && activeChatSearch->hasMore()) {
				const auto more = list->add(
					object_ptr<Ui::RoundButton>(
						list,
						rpl::single(QString::fromUtf8(Mokagram::Lang::Tr("Показать ещё"))),
						st::defaultLightButton),
					QMargins(24, 8, 24, 8));
				more->setClickedCallback([=] {
					const auto active = QueueState().selectedChat
						? searchState->extraChat.get()
						: searchState->chats.get();
					if (active) {
						active->searchMore();
					}
					more->setDisabled(true);
				});
			}
			refreshTransition();
		};
		const auto showStatus = [=](const QString &text, bool loading = false, bool sticker = false) {
			*statusText = text;
			*statusLoading = loading;
			*statusSticker = sticker;
			fill();
		};
		const auto albumsTop = [=] {
			return tabs->y() + tabs->height();
		};
		*openAlbum = [=](Album album) {
			animateChange(albumsTop(), 1, [=] {
				++albums->generation;
				albums->opened = album;
				albums->openedTracks.clear();
				QueueState().results.clear();
				showStatus(QString::fromUtf8(Mokagram::Lang::Tr("Загружаю альбом…")), true);
			});
			const auto generation = albums->generation;
			AlbumTracks(album.id, crl::guard(box, [=](std::vector<Track> tracks, QString error) {
				if (generation != albums->generation) {
					return;
				}
				albums->openedTracks = std::move(tracks);
				auto &queue = QueueState();
				if (!albums->active || queue.tab != 0 || queue.telegramSearch) {
					return;
				}
				queue.results = albums->openedTracks;
				showStatus(!error.isEmpty()
					? error
					: albums->openedTracks.empty()
					? QString::fromUtf8(Mokagram::Lang::Tr("В альбоме нет доступных треков"))
					: QString());
			}));
		};
		*closeAlbum = [=] {
			animateChange(albumsTop(), -1, [=] {
				++albums->generation;
				albums->opened.reset();
				albums->openedTracks.clear();
				QueueState().results.clear();
				showStatus(albums->found.empty()
					? QString::fromUtf8(Mokagram::Lang::Tr("Альбомы не найдены"))
					: QString());
			});
		};
		const auto searchAlbums = [=](const QString &query) {
			const auto generation = ++albums->generation;
			albums->lastQuery = query;
			albums->opened.reset();
			albums->openedTracks.clear();
			albums->found.clear();
			QueueState().results.clear();
			showStatus(QString::fromUtf8(Mokagram::Lang::Tr("Ищу альбомы…")), true);
			const auto finish = [=](
					const std::vector<Album> &first,
					const std::vector<Album> &rest,
					const QString &error) {
				if (generation != albums->generation) {
					return;
				}
				auto seen = base::flat_set<QString>();
				albums->found.clear();
				for (const auto list : { &first, &rest }) {
					for (const auto &album : *list) {
						if (seen.emplace(album.id).second) {
							albums->found.push_back(album);
						}
					}
				}
				if (!albums->active
					|| QueueState().tab != 0
					|| QueueState().telegramSearch) {
					return;
				}
				showStatus(!albums->found.empty()
					? QString()
					: !error.isEmpty()
					? error
					: QString::fromUtf8(Mokagram::Lang::Tr("Альбомы не найдены")));
			};
			const auto byQuery = [=](std::vector<Album> first) {
				if (query.isEmpty()) {
					finish(first, {}, QString());
					return;
				}
				SearchAlbums(query, 20, crl::guard(box, [=](std::vector<Album> rest, QString error) {
					finish(first, rest, error);
				}));
			};
			// Альбомы с играющим треком первыми; только для запроса, с которым окно открыли.
			const auto trackId = std::exchange(albums->trackId, QString());
			if (!trackId.isEmpty()) {
				TrackAlbums(trackId, crl::guard(box, byQuery));
			} else {
				byQuery({});
			}
		};
		searchState->chats = std::make_unique<ChatMusicSearch>(
			&controller->session(),
			[=](std::vector<Track> tracks, QString error) {
				auto &queue = QueueState();
				if (!queue.telegramSearch) {
					return;
				}
				queue.searchResults = std::move(tracks);
				if (queue.tab == 0) {
					queue.results = queue.searchResults;
					showStatus(!error.isEmpty()
						? error
						: queue.results.empty()
						? QString::fromUtf8(Mokagram::Lang::Tr("Ничего не найдено"))
						: QString());
				}
			});
		const auto buildForYou = [=] {
			showStatus(QString::fromUtf8(Mokagram::Lang::Tr("Подбираю музыку под ваш вкус…")), true);
			BuildForYou(30, crl::guard(box, [=](std::vector<Track> tracks, QString note) {
				auto &queue = QueueState();
				queue.forYou = std::move(tracks);
				queue.forYouNote = note;
				if (queue.tab == 2) {
					queue.results = queue.forYou;
					showStatus(note);
					fill();
				}
			}));
		};
		*refreshForYou = buildForYou;
		const auto setTab = [=](int tab) {
			auto &queue = QueueState();
			if (queue.telegramSearch) {
				tab = 0;
			}
			queue.tab = tab;
			sources->setVisible(tab == 0);
			sources->resize(sources->width(), (tab == 0) ? 48 : 0);
			const auto kindsShown = (tab == 0) && !queue.telegramSearch;
			kinds->setVisible(kindsShown);
			kinds->resize(kinds->width(), kindsShown ? 44 : 0);
			chooseChat->setVisible(tab == 0 && queue.telegramSearch);
			chooseChat->resize(chooseChat->width(), queue.telegramSearch ? 36 : 0);
			tabs->resize(tabs->width(), queue.telegramSearch ? 0 : 40);
			tabLiked->setVisible(!queue.telegramSearch);
			tabForYou->setVisible(!queue.telegramSearch);
			if (slide->index != tab) {
				slide->from = slide->index;
				slide->animation.start(
					[=] { tabs->update(); },
					slide->index,
					tab,
					kSwitchDuration,
					anim::easeOutCubic);
				slide->index = tab;
			}
			tabSearch->setActive(tab == 0);
			tabLiked->setActive(tab == 1);
			tabForYou->setActive(tab == 2);
			if (tab == 0 && kindsShown && albums->active) {
				queue.results = albums->opened
					? albums->openedTracks
					: std::vector<Track>();
				showStatus(
					QString(),
					false,
					!albums->opened && albums->found.empty());
			} else if (tab == 0) {
				queue.results = queue.searchResults;
				showStatus(QString(), false, queue.searchResults.empty());
			} else if (tab == 1) {
				queue.results = LikedTracks();
				showStatus(queue.results.empty()
					? QString::fromUtf8(Mokagram::Lang::Tr("Пока пусто. Нажмите сердечко у трека, и он появится здесь."))
					: QString());
			} else {
				queue.results = queue.forYou;
				if (queue.forYou.empty()) {
					buildForYou();
				} else {
					showStatus(QString());
				}
			}
			fill();
		};
		const auto switchTab = [=](int tab) {
			const auto was = QueueState().tab;
			if (was == tab) {
				return;
			}
			animateChange(
				searchWrap->y() + searchWrap->height(),
				(tab > was) ? 1 : -1,
				[=] { setTab(tab); });
		};
		tabSearch->setClickedCallback([=] { switchTab(0); });
		tabLiked->setClickedCallback([=] { switchTab(1); });
		tabForYou->setClickedCallback([=] { switchTab(2); });
		LikesChanged(
		) | rpl::filter([=] {
			return QueueState().tab == 1;
		}) | rpl::on_next([=] {
			auto &queue = QueueState();
			queue.results = LikedTracks();
			showStatus(queue.results.empty()
				? QString::fromUtf8(Mokagram::Lang::Tr("Пока пусто. Нажмите сердечко у трека, и он появится здесь."))
				: QString());
			fill();
		}, box->lifetime());

		const auto search = [=] {
			const auto query = input->getLastText().trimmed();
			auto &queue = QueueState();
			if (albums->active && !queue.telegramSearch) {
				if (query.isEmpty() && albums->trackId.isEmpty()) {
					return;
				}
				if (queue.tab != 0) {
					switchTab(0);
				}
				searchAlbums(query);
				return;
			}
			if (query.isEmpty() && !queue.telegramSearch) {
				return;
			}
			if (queue.tab != 0) {
				switchTab(0);
			}
			const auto generation = ++searchState->generation;
			searchState->chats->cancel();
			queue.lastQuery = query;
			queue.searchResults.clear();
			queue.results.clear();
			showStatus(QString::fromUtf8(Mokagram::Lang::Tr("Ищу…")), true);
			if (queue.telegramSearch) {
				if (queue.selectedChat) {
					searchState->chats->cancel();
					searchState->extraChat = std::make_unique<ChatMusicSearch>(
						&controller->session(),
						[=](std::vector<Track> tracks, QString error) {
							auto &queue = QueueState();
							if (!queue.telegramSearch || queue.tab != 0) {
								return;
							}
							queue.searchResults = std::move(tracks);
							queue.results = queue.searchResults;
							showStatus(!error.isEmpty()
								? error
								: queue.results.empty()
								? QString::fromUtf8(Mokagram::Lang::Tr("Ничего не найдено"))
								: QString());
						},
						queue.selectedChat);
					searchState->extraChat->search(query);
				} else {
					searchState->chats->search(query);
				}
				return;
			}
			Search(query, 0, crl::guard(box, [=](SearchResult result) {
				if (generation != searchState->generation) {
					return;
				}
				auto &queue = QueueState();
				queue.searchResults = std::move(result.tracks);
				if (queue.tab == 0) {
					queue.results = queue.searchResults;
					showStatus(!result.error.isEmpty()
						? result.error
						: queue.results.empty()
						? QString::fromUtf8(Mokagram::Lang::Tr("Ничего не найдено"))
						: QString());
				}
			}));
		};
		*runSearch = search;
		chooseChat->setClickedCallback([=] {
			ShowMusicSourceBox(
				controller,
				[=](not_null<Data::Thread*> thread) {
					QueueState().selectedChat = thread->owningHistory()->peer->id;
					const auto peer = controller->session().data().peer(QueueState().selectedChat);
					chooseChat->setText(rpl::single(
						QString::fromUtf8(Mokagram::Lang::Tr("Чат: %1"))
						.arg(peer->name())));
					if (*runSearch) {
						(*runSearch)();
					}
					return true;
				});
		});
		const auto updateSource = [=] {
			const auto chats = QueueState().telegramSearch;
			sourceSwitch->setChats(chats);
			chooseChat->setVisible(chats && QueueState().tab == 0);
			chooseChat->resize(chooseChat->width(), chats ? 36 : 0);
			tabs->resize(tabs->width(), chats ? 0 : 36);
			const auto kindsShown = !chats && QueueState().tab == 0;
			kinds->setVisible(kindsShown);
			kinds->resize(kinds->width(), kindsShown ? 44 : 0);
			if (QueueState().selectedChat) {
				if (const auto peer = controller->session().data().peer(QueueState().selectedChat)) {
					chooseChat->setText(rpl::single(
						QString::fromUtf8(Mokagram::Lang::Tr("Чат: %1"))
						.arg(peer->name())));
				}
			}
			tabLiked->setVisible(!chats);
			tabForYou->setVisible(!chats);
			tabs->update();
			slide->count = chats ? 1 : 3;
			if (chats) {
				tabSearch->setGeometry(0, 0, tabs->width(), 40);
			} else {
				const auto part = (tabs->width() - 8) / 3.;
				const auto at = [&](int i) { return 4 + int(base::SafeRound(part * i)); };
				tabSearch->setGeometry(at(0), 0, at(1) - at(0), 40);
				tabLiked->setGeometry(at(1), 0, at(2) - at(1), 40);
				tabForYou->setGeometry(at(2), 0, at(3) - at(2), 40);
			}
			input->setPlaceholder(rpl::single(input->getLastText().isEmpty()
				? QString::fromUtf8(Mokagram::Lang::Tr(chats
					? "Поиск музыки в чатах и группах"
					: albums->active
					? "Поиск альбомов на SoundCloud"
					: "Поиск на SoundCloud"))
				: QString()));
		};
		const auto setSource = [=](bool chats) {
			if (QueueState().telegramSearch == chats) {
				return;
			}
			++searchState->generation;
			searchState->chats->cancel();
			if (searchState->extraChat) {
				searchState->extraChat->cancel();
			}
			QueueState().telegramSearch = chats;
			QueueState().searchResults.clear();
			QueueState().results.clear();
			updateSource();
			setTab(0);
			search();
		};
		sourceSwitch->setChangedCallback([=](bool chats) {
			// Сам переключатель анимируется отдельно, переход идёт под ним.
			animateChange(
				sources->y() + sources->height(),
				chats ? 1 : -1,
				[=] { setSource(chats); });
		});
		kindSwitch->setChangedCallback([=](bool toAlbums) {
			animateChange(
				kinds->y() + kinds->height(),
				toAlbums ? 1 : -1,
				[=] {
					albums->active = toAlbums;
					kindSwitch->setChats(toAlbums);
					updateSource();
					const auto query = input->getLastText().trimmed();
					const auto last = toAlbums
						? albums->lastQuery
						: QueueState().lastQuery;
					if (!query.isEmpty() && query != last) {
						search();
					} else {
						setTab(0);
					}
				});
		});
		// Поиск по мере набора: запускаем после короткой паузы между буквами.
		const auto typingTimer = box->lifetime().make_state<base::Timer>();
		typingTimer->setCallback([=] {
			const auto query = input->getLastText().trimmed();
			const auto last = (albums->active && !QueueState().telegramSearch)
				? albums->lastQuery
				: QueueState().lastQuery;
			if (query.size() >= 2 && query != last) {
				search();
			}
		});
		input->changes() | rpl::on_next([=] {
			updateSource();
			typingTimer->callOnce(kTypingDelay);
		}, input->lifetime());
		input->submits() | rpl::on_next([=] {
			typingTimer->cancel();
			search();
		}, input->lifetime());
		input->setFocusFast();
		if (!initialQuery.isEmpty() || albumsMode) {
			input->setText(initialQuery);
			setTab(0);
			search();
		} else {
			if (!QueueState().lastQuery.isEmpty()) {
				// Возвращаем последний запрос и его результаты без нового запроса.
				input->setText(QueueState().lastQuery);
			}
			setTab(QueueState().tab);
			if (QueueState().tab == 0 && QueueState().telegramSearch) {
				search();
			}
		}
		updateSource();
		sourceSwitch->finishAnimating();
	}));
}

} // namespace

void ShowMusicBox(
		not_null<Window::SessionController*> controller,
		const QString &initialQuery) {
	ShowMusicBoxWith(controller, initialQuery, false, QString());
}

void ShowAlbumsBox(
		not_null<Window::SessionController*> controller,
		const QString &query,
		const QString &trackId) {
	ShowMusicBoxWith(controller, query, true, trackId);
}


// ---- Мост для панели плеера в баре Omarchy (Quickshell) ----
// Клиент держит локальный сокет $XDG_RUNTIME_DIR/mokagram/ctl.sock. Панель
// подключается, получает построчный JSON (состояние, тексты) и шлёт команды
// строками «play», «seek 12345», «volume 0.5» и т.д. Обложка лежит файлом.
namespace {

struct ShellBridge {
	QLocalServer *server = nullptr;
	std::vector<QLocalSocket*> clients;
	QTimer *timer = nullptr;
	QByteArray lastState;
	QString dir;
	QString coverPath;
	qint64 coverKey = 0;
	int coverSerial = 0;
	QString lyricsId;
	QByteArray lyricsLine;
	QByteArray lastLibrary;
	QString commentsId;
	QByteArray lastCommentsLine;
	qint64 sleepAt = 0;
	int libTick = 0;
	rpl::lifetime lifetime;
};

ShellBridge &Bridge() {
	static auto *bridge = new ShellBridge();
	return *bridge;
}

void BridgeSend(const QByteArray &line) {
	auto &b = Bridge();
	for (const auto client : b.clients) {
		client->write(line);
		client->write("\n");
	}
}

[[nodiscard]] QByteArray BridgeJson(const QJsonObject &object) {
	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

void BridgeSendLyrics(const QString &id, const Lyrics &lyrics) {
	auto lines = QJsonArray();
	for (const auto &line : lyrics.lines) {
		lines.push_back(QJsonArray{ double(line.timeMs), line.text });
	}
	auto object = QJsonObject();
	object.insert(u"t"_q, u"lyrics"_q);
	object.insert(u"id"_q, id);
	object.insert(u"found"_q, lyrics.found);
	object.insert(u"synced"_q, lyrics.synced);
	object.insert(u"lines"_q, lines);
	auto &b = Bridge();
	b.lyricsLine = BridgeJson(object);
	BridgeSend(b.lyricsLine);
}

void BridgeWriteCover(const AudioMsgId &current, QJsonObject &state) {
	auto &b = Bridge();
	const auto cover = CoverForPlayback(current);
	if (cover.isNull()) {
		state.insert(u"cover"_q, QString());
		return;
	}
	if (cover.cacheKey() != b.coverKey || b.coverPath.isEmpty()) {
		const auto old = b.coverPath;
		b.coverKey = cover.cacheKey();
		b.coverPath = u"%1/cover-%2.jpg"_q.arg(b.dir).arg(++b.coverSerial);
		cover.save(b.coverPath, "JPG", 92);
		if (!old.isEmpty()) {
			QFile::remove(old);
		}
	}
	state.insert(u"cover"_q, b.coverPath);
}

void BridgePush(bool force = false) {
	auto &b = Bridge();
	// При выходе плеер телеги уничтожается раньше, чем гаснет таймер моста.
	if (b.clients.empty() || Core::Quitting()) {
		return;
	}
	const auto instance = Media::Player::instance();
	const auto type = AudioMsgId::Type::Song;
	const auto current = instance->current(type);
	const auto st = instance->getState(type);
	auto state = QJsonObject();
	state.insert(u"t"_q, u"state"_q);
	const auto &settings = Core::App().settings();
	state.insert(u"volume"_q, settings.songVolume());
	state.insert(u"shuffle"_q, settings.playerOrderMode() == Media::OrderMode::Shuffle);
	state.insert(u"repeat"_q, settings.playerRepeatMode() == Media::RepeatMode::One
		? u"one"_q
		: settings.playerRepeatMode() == Media::RepeatMode::All
		? u"all"_q
		: u"none"_q);
	if (!current.audio()) {
		state.insert(u"has"_q, false);
	} else {
		const auto track = PlayingTrack();
		const auto frequency = std::max<int64>(st.frequency, 1);
		const auto playing = (st.state == Media::Player::State::Playing)
			|| (st.state == Media::Player::State::Starting)
			|| (st.state == Media::Player::State::Resuming);
		const auto &queue = QueueState();
		const auto owns = OwnsTrack(current);
		state.insert(u"has"_q, true);
		state.insert(u"id"_q, track.id);
		state.insert(u"title"_q, track.title);
		state.insert(u"artist"_q, track.artist);
		state.insert(u"source"_q, IsTelegramTrack(track)
			? u"telegram"_q
			: u"soundcloud"_q);
		state.insert(u"playing"_q, playing);
		state.insert(u"posMs"_q, double(st.position * 1000 / frequency));
		state.insert(u"lenMs"_q, double(st.length > 0
			? st.length * 1000 / frequency
			: track.durationMs));
		state.insert(u"liked"_q, owns && IsLiked(track.id));
		state.insert(u"canLike"_q, owns);
		state.insert(u"canNext"_q, owns ? NextAvailable() : true);
		state.insert(u"canPrev"_q, owns ? PreviousAvailable() : true);
		state.insert(u"speed"_q, settings.audioPlaybackSpeed(true));
		state.insert(u"speedOk"_q, current.changeablePlaybackSpeed());
		state.insert(u"link"_q, !track.permalink.isEmpty());
		state.insert(u"sleepLeft"_q, b.sleepAt > 0
			? double(std::max<qint64>(0, (b.sleepAt - crl::now()) / 1000))
			: 0.);
		const auto &together = TogetherState();
		state.insert(u"together"_q, !together.code.isEmpty());
		state.insert(u"togetherHost"_q, together.host);
		state.insert(u"listeners"_q, together.listeners);
		const auto accent = CurrentCoverAccent();
		state.insert(u"accent"_q, accent.isValid() ? accent.name() : QString());
		BridgeWriteCover(current, state);
		auto list = QJsonArray();
		if (owns) {
			const auto count = int(queue.tracks.size());
			const auto from = std::max(0, queue.index - 2);
			for (auto i = from; i < std::min(count, from + 40); ++i) {
				auto row = QJsonObject();
				row.insert(u"i"_q, i);
				row.insert(u"title"_q, queue.tracks[i].title);
				row.insert(u"artist"_q, queue.tracks[i].artist);
				row.insert(u"current"_q, i == queue.index);
				list.push_back(row);
			}
		}
		state.insert(u"queue"_q, list);
		if (b.lyricsId != track.id) {
			b.lyricsId = track.id;
			b.lyricsLine.clear();
			b.lastCommentsLine.clear();
			const auto id = track.id;
			if (!IsTelegramTrack(track)) {
				LoadTrackComments(id, QString(), [=](TrackCommentsPage page) {
					if (Bridge().lyricsId != id) {
						return;
					}
					auto list = QJsonArray();
					for (const auto &c : page.comments) {
						if (c.timestampMs < 0 || c.body.trimmed().isEmpty()) {
							continue;
						}
						list.push_back(QJsonArray{ double(c.timestampMs), c.author, c.body });
					}
					auto object = QJsonObject();
					object.insert(u"t"_q, u"comments"_q);
					object.insert(u"id"_q, id);
					object.insert(u"list"_q, list);
					Bridge().lastCommentsLine = BridgeJson(object);
					BridgeSend(Bridge().lastCommentsLine);
				});
			}
			FetchLyrics(
				id,
				track.title,
				track.artist,
				track.durationMs,
				[=](Lyrics lyrics) {
					if (Bridge().lyricsId == id) {
						BridgeSendLyrics(id, lyrics);
					}
				});
		}
	}
	const auto line = BridgeJson(state);
	if (force || line != b.lastState) {
		b.lastState = line;
		BridgeSend(line);
	}
}

[[nodiscard]] QJsonObject BridgeTrackJson(const Track &track) {
	auto row = QJsonObject();
	row.insert(u"title"_q, track.title);
	row.insert(u"artist"_q, track.artist);
	return row;
}

void BridgePushLibrary(bool force = false) {
	auto &b = Bridge();
	if (b.clients.empty()) {
		return;
	}
	auto liked = QJsonArray();
	auto count = 0;
	for (const auto &track : LikedTracks()) {
		if (++count > 150) {
			break;
		}
		liked.push_back(BridgeTrackJson(track));
	}
	auto lists = QJsonArray();
	for (const auto &playlist : Playlists()) {
		auto tracks = QJsonArray();
		auto n = 0;
		for (const auto &track : playlist.tracks) {
			if (++n > 150) {
				break;
			}
			tracks.push_back(BridgeTrackJson(track));
		}
		auto row = QJsonObject();
		row.insert(u"id"_q, playlist.id);
		row.insert(u"name"_q, playlist.name);
		row.insert(u"tracks"_q, tracks);
		lists.push_back(row);
	}
	auto object = QJsonObject();
	object.insert(u"t"_q, u"library"_q);
	object.insert(u"liked"_q, liked);
	object.insert(u"playlists"_q, lists);
	const auto line = BridgeJson(object);
	if (force || line != b.lastLibrary) {
		b.lastLibrary = line;
		BridgeSend(line);
	}
}

void BridgeCommand(const QString &line) {
	const auto parts = line.trimmed().split(QChar(' '), Qt::SkipEmptyParts);
	if (parts.isEmpty()) {
		return;
	}
	const auto cmd = parts[0];
	const auto arg = (parts.size() > 1) ? parts[1] : QString();
	const auto instance = Media::Player::instance();
	const auto type = AudioMsgId::Type::Song;
	const auto current = instance->current(type);
	auto &settings = Core::App().settings();
	if (cmd == u"toggle"_q) {
		instance->playPause(type);
	} else if (cmd == u"play"_q || cmd == u"pause"_q) {
		const auto st = instance->getState(type);
		const auto playing = (st.state == Media::Player::State::Playing)
			|| (st.state == Media::Player::State::Starting)
			|| (st.state == Media::Player::State::Resuming);
		if (playing == (cmd == u"pause"_q)) {
			instance->playPause(type);
		}
	} else if (cmd == u"next"_q || cmd == u"prev"_q) {
		const auto delta = (cmd == u"next"_q) ? 1 : -1;
		if (OwnsTrack(current)) {
			Step(delta);
		} else if (delta > 0) {
			instance->next(type);
		} else {
			instance->previous(type);
		}
	} else if (cmd == u"seek"_q) {
		const auto st = instance->getState(type);
		const auto lengthMs = st.length * 1000 / std::max<int64>(st.frequency, 1);
		if (lengthMs > 0) {
			instance->startSeeking(type);
			instance->finishSeeking(type, std::clamp(arg.toDouble() / double(lengthMs), 0., 1.));
		}
	} else if (cmd == u"volume"_q) {
		const auto value = std::clamp(arg.toDouble(), 0., 1.);
		settings.setSongVolume(value);
		Core::App().saveSettingsDelayed();
		Media::Player::mixer()->setSongVolume(value);
	} else if (cmd == u"shuffle"_q) {
		settings.setPlayerOrderMode(
			settings.playerOrderMode() == Media::OrderMode::Shuffle
				? Media::OrderMode::Default
				: Media::OrderMode::Shuffle);
		Core::App().saveSettingsDelayed();
	} else if (cmd == u"repeat"_q) {
		const auto mode = settings.playerRepeatMode();
		settings.setPlayerRepeatMode(mode == Media::RepeatMode::None
			? Media::RepeatMode::All
			: mode == Media::RepeatMode::All
			? Media::RepeatMode::One
			: Media::RepeatMode::None);
		Core::App().saveSettingsDelayed();
	} else if (cmd == u"like"_q) {
		const auto &queue = QueueState();
		if (OwnsTrack(current) && !queue.current.id.isEmpty()) {
			SetLiked(queue.current, !IsLiked(queue.current.id));
		}
	} else if (cmd == u"playindex"_q) {
		const auto index = arg.toInt();
		if (OwnsTrack(current) && index >= 0 && index < int(QueueState().tracks.size())) {
			PlayIndex(index);
		}
	} else if (cmd == u"speed"_q) {
		settings.setAudioPlaybackSpeed(std::clamp(arg.toDouble(), 0.5, 2.0));
		Core::App().saveSettingsDelayed();
		instance->updatePlaybackSpeed();
	} else if (cmd == u"sleep"_q) {
		const auto minutes = arg.toInt();
		Bridge().sleepAt = (minutes > 0) ? (crl::now() + minutes * 60 * 1000) : 0;
	} else if (cmd == u"playliked"_q) {
		auto &queue = QueueState();
		queue.custom = false;
		queue.results = LikedTracks();
		PlayFromResults(arg.toInt());
	} else if (cmd == u"playlist"_q && parts.size() > 2) {
		for (const auto &playlist : Playlists()) {
			if (playlist.id == arg) {
				auto &queue = QueueState();
				queue.custom = false;
				queue.results = playlist.tracks;
				PlayFromResults(parts[2].toInt());
				break;
			}
		}
	} else if (cmd == u"copylink"_q || cmd == u"openlink"_q) {
		const auto link = PlayingTrack().permalink;
		if (!link.isEmpty()) {
			if (cmd == u"copylink"_q) {
				QGuiApplication::clipboard()->setText(link);
			} else {
				QDesktopServices::openUrl(QUrl(link));
			}
		}
	} else if (cmd == u"forward"_q) {
		if (const auto window = Core::App().activePrimaryWindow()) {
			window->activate();
			if (const auto controller = window->sessionController()) {
				const auto track = PlayingTrack();
				if (!track.id.isEmpty()) {
					ForwardTrack(controller, track);
				}
			}
		}
	} else if (cmd == u"open"_q || cmd == u"fullscreen"_q) {
		if (const auto window = Core::App().activePrimaryWindow()) {
			window->activate();
			if (const auto controller = window->sessionController()) {
				if (cmd == u"fullscreen"_q) {
					ShowFullscreenPlayer(controller);
				} else {
					ShowPlayerBox(controller);
				}
			}
		}
	}
	BridgePush();
}

} // namespace

void StartShellBridge() {
	auto &b = Bridge();
	if (b.server) {
		return;
	}
	const auto runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
	if (runtime.isEmpty()) {
		return;
	}
	b.dir = runtime + u"/mokagram"_q;
	QDir().mkpath(b.dir);
	QFile::setPermissions(b.dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
	const auto path = b.dir + u"/ctl.sock"_q;
	QLocalServer::removeServer(path);
	b.server = new QLocalServer(qApp);
	b.server->setSocketOptions(QLocalServer::UserAccessOption);
	if (!b.server->listen(path)) {
		return;
	}
	QObject::connect(b.server, &QLocalServer::newConnection, [] {
		auto &b = Bridge();
		while (const auto client = b.server->nextPendingConnection()) {
			b.clients.push_back(client);
			const auto buffer = new QByteArray();
			QObject::connect(client, &QLocalSocket::readyRead, client, [=] {
				buffer->append(client->readAll());
				while (true) {
					const auto end = buffer->indexOf('\n');
					if (end < 0) {
						break;
					}
					const auto text = QString::fromUtf8(buffer->left(end));
					buffer->remove(0, end + 1);
					BridgeCommand(text);
				}
			});
			QObject::connect(client, &QLocalSocket::disconnected, client, [=] {
				auto &b = Bridge();
				b.clients.erase(std::remove(b.clients.begin(), b.clients.end(), client), b.clients.end());
				delete buffer;
				client->deleteLater();
			});
			b.lastState.clear();
			crl::on_main([] {
				BridgePush(true);
				BridgePushLibrary(true);
				const auto &b = Bridge();
				if (!b.lastCommentsLine.isEmpty()) {
					BridgeSend(b.lastCommentsLine);
				}
			});
			if (!b.lyricsLine.isEmpty()) {
				client->write(b.lyricsLine + "\n");
			}
		}
	});
	b.timer = new QTimer(qApp);
	b.timer->setInterval(250);
	// Плеер создаётся позже запуска моста: до этого его не трогаем.
	QObject::connect(b.timer, &QTimer::timeout, [] {
		if (Core::Quitting()) {
			return;
		}
		static const auto started = crl::now();
		if (crl::now() - started > 8000) {
			auto &b = Bridge();
			if (b.sleepAt > 0 && crl::now() >= b.sleepAt) {
				b.sleepAt = 0;
				const auto instance = Media::Player::instance();
				const auto st = instance->getState(AudioMsgId::Type::Song);
				if (st.state == Media::Player::State::Playing
					|| st.state == Media::Player::State::Starting
					|| st.state == Media::Player::State::Resuming) {
					instance->playPause(AudioMsgId::Type::Song);
				}
			}
			if (++b.libTick % 8 == 0) {
				BridgePushLibrary();
			}
			BridgePush();
		}
	});
	b.timer->start();

}

Track CurrentQueueTrack() {
	const auto current = Media::Player::instance()->current(AudioMsgId::Type::Song);
	const auto &queue = QueueState();
	return OwnsTrack(current) ? queue.current : Track();
}

} // namespace Mokagram::Music
