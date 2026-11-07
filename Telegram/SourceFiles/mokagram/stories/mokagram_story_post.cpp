// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: публикация истории с компьютера (фото или видео).
#include "mokagram/stories/mokagram_story_post.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/ui/mokagram_box_parts.h"
#include "api/api_sending.h"
#include "api/api_text_entities.h"
#include "apiwrap.h"
#include "base/random.h"
#include "chat_helpers/message_field.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "data/data_document.h"
#include "data/data_peer.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/main_app_config.h"
#include "main/main_session.h"
#include "mainwidget.h"
#include "storage/file_upload.h"
#include "storage/localimageloader.h"
#include "storage/storage_media_prepare.h"
#include "ui/chat/attach/attach_prepare.h"
#include "ui/effects/animations.h"
#include "ui/effects/animation_value.h"
#include "ui/image/image_prepare.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/painter.h"
#include "ui/text/text_entity.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <QtGui/QPainterPath>

namespace Mokagram::Stories {
namespace {

using namespace Mokagram::BoxParts;

constexpr auto kBoxWidth = 420;
constexpr auto kPad = 16;
constexpr auto kRowHeight = 52;
constexpr auto kPreviewHeight = 340;
constexpr auto kDefaultPeriod = TimeId(24 * 3600);
constexpr TimeId kPeriods[] = { 6 * 3600, 12 * 3600, 24 * 3600, 48 * 3600 };

enum class Audience {
	Everyone,
	Contacts,
	CloseFriends,
};

struct Draft {
	not_null<PeerData*> peer;
	Ui::PreparedFile file;
	TextWithTags caption;
	Audience audience = Audience::Contacts;
	TimeId period = kDefaultPeriod;
	bool pinned = false;
	bool protect = false;
};

struct Callbacks {
	Fn<void(float64)> progress;
	Fn<void()> done;
	Fn<void(QString)> failed;
};

[[nodiscard]] int S(int value) {
	return style::ConvertScale(value);
}

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

// Медиа ещё не готово к отправке: файл загрузят заново.
[[nodiscard]] bool MediaFailed(const QString &type) {
	return type.startsWith(u"FILE_"_q) || type.startsWith(u"MEDIA_"_q);
}

[[nodiscard]] QString ErrorText(const QString &type) {
	if (type == u"STORIES_TOO_MUCH"_q) {
		return U("Лимит историй на сегодня исчерпан.");
	} else if (type == u"PREMIUM_ACCOUNT_REQUIRED"_q) {
		return U("Для этого нужен Telegram Premium.");
	} else if (type == u"BOOSTS_REQUIRED"_q) {
		return U("Каналу не хватает бустов для историй.");
	} else if (type == u"STORY_PERIOD_INVALID"_q) {
		return U("Такой срок доступен только с Premium.");
	} else if (type == u"CHAT_ADMIN_REQUIRED"_q
		|| type == u"CHAT_WRITE_FORBIDDEN"_q) {
		return U("Нет прав публиковать истории здесь.");
	} else if (MediaFailed(type)) {
		return U("Telegram не принял этот файл как историю.");
	}
	return type;
}

class PrepareTask final : public Task {
public:
	PrepareTask(
		FileLoadTask::Args &&args,
		Fn<void(std::shared_ptr<FilePrepareResult>)> done)
	: _task(std::move(args))
	, _done(std::move(done)) {
	}

	void process() override {
		_task.process({ .generateGoodThumbnail = false });
	}
	void finish() override {
		_done(_task.peekResult());
	}

private:
	FileLoadTask _task;
	Fn<void(std::shared_ptr<FilePrepareResult>)> _done;

};

// Загружает одно фото или видео и публикует его историей.
class Poster final {
public:
	explicit Poster(not_null<Main::Session*> session);
	~Poster();

	void post(Draft &&draft, Callbacks &&callbacks);
	void cancel();

private:
	struct Active {
		Draft draft;
		Callbacks callbacks;
		FullMsgId uploadId;
		uint64 dataId = 0;
		bool video = false;
		QString filemime;
		QVector<MTPDocumentAttribute> attributes;
	};

	void prepared(uint64 token, std::shared_ptr<FilePrepareResult> result);
	void send(uint64 token, MTPInputMedia media);
	void fail(uint64 token, const QString &text);
	void progress(const FullMsgId &id, bool photo);

	const not_null<Main::Session*> _session;
	MTP::Sender _api;
	std::unique_ptr<TaskQueue> _prepareQueue;
	std::unique_ptr<Active> _active;
	uint64 _token = 0;
	uint64 _randomId = 0;
	rpl::lifetime _lifetime;

};

Poster::Poster(not_null<Main::Session*> session)
: _session(session)
, _api(&session->mtp())
, _prepareQueue(std::make_unique<TaskQueue>()) {
	const auto &uploader = _session->uploader();
	uploader.photoReady(
	) | rpl::on_next([=](const Storage::UploadedMedia &data) {
		if (_active && _active->uploadId == data.fullId) {
			send(_token, MTP_inputMediaUploadedPhoto(
				MTP_flags(0),
				data.info.file,
				MTP_vector<MTPInputDocument>(QVector<MTPInputDocument>()),
				MTPint(),
				MTPInputDocument()));
		}
	}, _lifetime);
	uploader.documentReady(
	) | rpl::on_next([=](const Storage::UploadedMedia &data) {
		if (!_active || _active->uploadId != data.fullId) {
			return;
		}
		using Flag = MTPDinputMediaUploadedDocument::Flag;
		send(_token, MTP_inputMediaUploadedDocument(
			MTP_flags(data.info.thumb ? Flag::f_thumb : Flag()),
			data.info.file,
			data.info.thumb.value_or(MTPInputFile()),
			MTP_string(_active->filemime),
			MTP_vector<MTPDocumentAttribute>(_active->attributes),
			MTP_vector<MTPInputDocument>(),
			MTPInputPhoto(),
			MTP_int(0),
			MTP_int(0)));
	}, _lifetime);
	uploader.photoProgress(
	) | rpl::on_next([=](const FullMsgId &id) {
		progress(id, true);
	}, _lifetime);
	uploader.documentProgress(
	) | rpl::on_next([=](const FullMsgId &id) {
		progress(id, false);
	}, _lifetime);
	const auto failed = [=](const FullMsgId &id) {
		if (_active && _active->uploadId == id) {
			fail(_token, tr::lng_attach_failed(tr::now));
		}
	};
	uploader.photoFailed() | rpl::on_next(failed, _lifetime);
	uploader.documentFailed() | rpl::on_next(failed, _lifetime);
}

Poster::~Poster() {
	cancel();
}

void Poster::progress(const FullMsgId &id, bool photo) {
	if (!_active
		|| _active->uploadId != id
		|| !_active->dataId
		|| !_active->callbacks.progress) {
		return;
	}
	_active->callbacks.progress(photo
		? _session->data().photo(_active->dataId)->progress()
		: _session->data().document(_active->dataId)->progress());
}

void Poster::post(Draft &&draft, Callbacks &&callbacks) {
	cancel();
	const auto token = ++_token;
	const auto video = (draft.file.type == Ui::PreparedFile::Type::Video);
	const auto peer = draft.peer;
	_randomId = base::RandomValue<uint64>();
	_active = std::make_unique<Active>(Active{
		.draft = std::move(draft),
		.callbacks = std::move(callbacks),
		.video = video,
	});
	auto &file = _active->draft.file;
	_prepareQueue->addTask(std::make_unique<PrepareTask>(
		FileLoadTask::Args{
			.session = _session,
			.filepath = file.path,
			.content = file.content,
			.information = std::move(file.information),
			.videoCover = nullptr,
			.type = video ? SendMediaType::File : SendMediaType::Photo,
			.to = FileLoadTo(
				peer->id,
				Api::SendOptions(),
				FullReplyTo(),
				MsgId()),
			.caption = TextWithTags(),
			.displayName = file.displayName,
		},
		[=](std::shared_ptr<FilePrepareResult> result) {
			prepared(token, std::move(result));
		}));
}

void Poster::prepared(
		uint64 token,
		std::shared_ptr<FilePrepareResult> result) {
	if (!_active || _token != token) {
		return;
	} else if (!result) {
		fail(token, tr::lng_attach_failed(tr::now));
		return;
	}
	if (_active->video) {
		// Перекодированное видео будет MP4, другой формат сервер не примет.
		if (!result->videoSource && (result->filemime != u"video/mp4"_q)) {
			fail(token, U("Видео для истории должно быть в MP4."));
			return;
		}
		result->document.match([&](const MTPDdocument &data) {
			_active->attributes = data.vattributes().v;
		}, [](const auto &) {
		});
		_active->filemime = result->filemime;
	} else if (result->type != SendMediaType::Photo) {
		fail(token, tr::lng_attach_failed(tr::now));
		return;
	}
	_active->dataId = result->id;
	_active->uploadId = FullMsgId(
		_active->draft.peer->id,
		_session->data().nextLocalMessageId());
	_session->uploader().upload(_active->uploadId, result);
}

void Poster::send(uint64 token, MTPInputMedia media) {
	if (!_active || _token != token) {
		return;
	}
	const auto &draft = _active->draft;
	using Flag = MTPstories_SendStory::Flag;
	auto flags = MTPstories_SendStory::Flags(Flag::f_period);
	if (draft.pinned) {
		flags |= Flag::f_pinned;
	}
	if (draft.protect) {
		flags |= Flag::f_noforwards;
	}
	auto entities = Api::EntitiesToMTP(
		_session,
		TextUtilities::ConvertTextTagsToEntities(draft.caption.tags),
		Api::ConvertOption::SkipLocal);
	if (!draft.caption.text.isEmpty()) {
		flags |= Flag::f_caption;
		if (!entities.v.isEmpty()) {
			flags |= Flag::f_entities;
		}
	}
	auto rules = QVector<MTPInputPrivacyRule>();
	switch (draft.audience) {
	case Audience::Everyone:
		rules.push_back(MTP_inputPrivacyValueAllowAll());
		break;
	case Audience::Contacts:
		rules.push_back(MTP_inputPrivacyValueAllowContacts());
		break;
	case Audience::CloseFriends:
		rules.push_back(MTP_inputPrivacyValueAllowCloseFriends());
		break;
	}
	_api.request(MTPstories_SendStory(
		MTP_flags(flags),
		draft.peer->input(),
		std::move(media),
		MTPVector<MTPMediaArea>(),
		MTP_string(draft.caption.text),
		entities,
		MTP_vector<MTPInputPrivacyRule>(std::move(rules)),
		MTP_long(_randomId),
		MTP_int(draft.period),
		MTPInputPeer(),
		MTPint(),
		MTPVector<MTPint>(),
		MTPInputDocument()
	)).done([=](const MTPUpdates &result) {
		if (_token != token || !_active) {
			return;
		}
		_session->api().applyUpdates(result);
		const auto done = std::move(_active->callbacks.done);
		_active = nullptr;
		if (done) {
			done();
		}
	}).fail([=](const MTP::Error &error) {
		fail(token, error.type());
	}).send();
}

void Poster::fail(uint64 token, const QString &text) {
	if (!_active || _token != token) {
		return;
	}
	const auto failed = std::move(_active->callbacks.failed);
	if (_active->uploadId) {
		_session->uploader().cancel(_active->uploadId);
	}
	_active = nullptr;
	if (failed) {
		failed(text);
	}
}

void Poster::cancel() {
	if (!_active) {
		return;
	}
	++_token;
	if (_active->uploadId) {
		_session->uploader().cancel(_active->uploadId);
	}
	_active = nullptr;
}

void CheckCanPost(
		not_null<Main::Session*> session,
		not_null<PeerData*> peer,
		Fn<void()> allowed,
		Fn<void(QString)> failed) {
	session->api().request(
		MTPstories_CanSendStory(peer->input())
	).done([=](const MTPstories_CanSendStoryCount &result) {
		if (result.c_stories_canSendStoryCount().vcount_remains().v > 0) {
			allowed();
		} else {
			failed(u"STORIES_TOO_MUCH"_q);
		}
	}).fail([=](const MTP::Error &error) {
		failed(error.type());
	}).send();
}

// Как в просмотре историй: медиа вписано в 9:16 поверх своей размытой копии.
[[nodiscard]] QImage PrepareCard(const QImage &image, QSize size) {
	const auto ratio = style::DevicePixelRatio();
	auto result = QImage(size * ratio, QImage::Format_ARGB32_Premultiplied);
	result.setDevicePixelRatio(ratio);
	result.fill(st::windowBg->c);
	if (image.isNull() || size.isEmpty()) {
		return result;
	}
	auto p = QPainter(&result);
	auto hq = PainterHighQualityEnabler(p);
	const auto small = image.scaled(
		QSize(size.width() / 4, size.height() / 4).expandedTo({ 1, 1 }),
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	const auto blurred = Images::BlurLargeImage(
		small.convertToFormat(QImage::Format_ARGB32_Premultiplied),
		8);
	const auto cover = blurred.size().scaled(
		size,
		Qt::KeepAspectRatioByExpanding);
	p.drawImage(
		QRect(
			(size.width() - cover.width()) / 2,
			(size.height() - cover.height()) / 2,
			cover.width(),
			cover.height()),
		blurred);
	p.fillRect(QRect(QPoint(), size), QColor(0, 0, 0, 90));
	const auto fitted = image.size().scaled(size, Qt::KeepAspectRatio);
	p.drawImage(
		QRect(
			(size.width() - fitted.width()) / 2,
			(size.height() - fitted.height()) / 2,
			fitted.width(),
			fitted.height()),
		image);
	return result;
}

[[nodiscard]] const style::owned_color &Clear() {
	static const auto result = style::owned_color(QColor(0, 0, 0, 0));
	return result;
}

void SetupRounded(not_null<Ui::GenericBox*> box) {
	static const auto noButtons = [] {
		auto result = st::defaultBox;
		result.buttonPadding = style::margins();
		result.buttonHeight = 0;
		result.bg = Clear().color();
		return result;
	}();
	box->setStyle(noButtons);
	box->paintOn([=](QPainter &p) {
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(st::boxBg);
		p.drawRoundedRect(box->rect(), S(kBoxRadius), S(kBoxRadius));
	});
	box->setWidth(S(kBoxWidth));
	box->setNoContentMargin(true);
}

void AddHeader(not_null<Ui::GenericBox*> box, const QString &title) {
	static const auto titleSt = [] {
		auto result = st::boxTitle;
		result.minWidth = 1;
		result.maxHeight = 0;
		result.align = style::al_top;
		return result;
	}();
	const auto content = box->verticalLayout();
	const auto header = content->add(
		object_ptr<Ui::RpWidget>(content),
		QMargins(S(10), S(10), S(10), 0));
	const auto round = S(kRoundSize);
	header->resize(header->width(), round);
	const auto close = MakeRoundButton(
		header,
		RoundIcon::Close,
		[=] { box->closeBox(); });
	const auto label = Ui::CreateChild<Ui::FlatLabel>(header, title, titleSt);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	header->widthValue() | rpl::on_next([=](int width) {
		const auto side = round + S(8);
		label->resizeToWidth(std::max(width - 2 * side, 1));
		const auto height = std::max(round, label->height());
		label->moveToLeft(side, (height - label->height()) / 2);
		close->moveToLeft(0, (height - close->height()) / 2);
		header->resize(width, height);
	}, header->lifetime());
}

[[nodiscard]] not_null<Ui::VerticalLayout*> AddCard(
		not_null<Ui::VerticalLayout*> container,
		int top) {
	const auto card = container->add(
		object_ptr<Ui::VerticalLayout>(container),
		QMargins(S(kOuterSide), S(top), S(kOuterSide), 0));
	card->paintOn([=](QPainter &p) {
		PaintCard(p, card->rect());
	});
	return card;
}

void PaintDivider(QPainter &p, not_null<QWidget*> row) {
	auto line = st::windowSubTextFg->c;
	line.setAlpha(40);
	p.fillRect(S(kPad), 0, row->width() - S(kPad), st::lineWidth, line);
}

[[nodiscard]] const style::FlatLabel &RowLabel() {
	static const auto result = [] {
		auto result = st::boxLabel;
		result.minWidth = 1;
		return result;
	}();
	return result;
}

void AddToggle(
		not_null<Ui::VerticalLayout*> card,
		const QString &title,
		std::shared_ptr<rpl::variable<bool>> value) {
	const auto first = (card->count() == 0);
	const auto row = card->add(object_ptr<Ui::RpWidget>(card));
	row->resize(row->width(), S(kRowHeight));
	const auto label = Ui::CreateChild<Ui::FlatLabel>(row, title, RowLabel());
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	const auto toggle = MakeSwitch(
		row,
		value->current(),
		[=](bool checked) { *value = checked; });
	row->widthValue() | rpl::on_next([=](int width) {
		label->resizeToWidth(std::max(
			width - 3 * S(kPad) - toggle->width(),
			1));
		label->moveToLeft(S(kPad), (row->height() - label->height()) / 2);
		toggle->moveToRight(S(kPad), (row->height() - toggle->height()) / 2);
	}, row->lifetime());
	if (!first) {
		row->paintOn([=](QPainter &p) {
			PaintDivider(p, row);
		});
	}
}

// Строка с выбором одного варианта: подпись сверху, пилюли в ряд.
void AddChoice(
		not_null<Ui::VerticalLayout*> card,
		const QString &title,
		const QStringList &options,
		std::shared_ptr<rpl::variable<int>> value) {
	struct State {
		Ui::Animations::Simple animation;
		int from = 0;
		QPoint pressed;
		std::vector<QRect> pills;
	};
	const auto first = (card->count() == 0);
	const auto row = card->add(object_ptr<Ui::AbstractButton>(card));
	const auto state = row->lifetime().make_state<State>();
	state->from = value->current();
	const auto label = Ui::CreateChild<Ui::FlatLabel>(row, title, RowLabel());
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	const auto font = st::normalFont;
	const auto pillHeight = S(34);
	const auto layout = [=](int width) {
		state->pills.clear();
		const auto top = S(12) + label->height() + S(10);
		const auto available = width - 2 * S(kPad);
		const auto gap = S(6);
		const auto each = (available - gap * (int(options.size()) - 1))
			/ int(options.size());
		for (auto i = 0; i != int(options.size()); ++i) {
			state->pills.push_back(QRect(
				S(kPad) + i * (each + gap),
				top,
				each,
				pillHeight));
		}
		row->resize(width, top + pillHeight + S(14));
	};
	row->widthValue() | rpl::on_next([=](int width) {
		label->resizeToWidth(std::max(width - 2 * S(kPad), 1));
		label->moveToLeft(S(kPad), S(12));
		layout(width);
	}, row->lifetime());
	row->events() | rpl::filter([](not_null<QEvent*> e) {
		return e->type() == QEvent::MouseButtonPress;
	}) | rpl::on_next([=](not_null<QEvent*> e) {
		state->pressed = static_cast<QMouseEvent*>(e.get())->pos();
	}, row->lifetime());
	row->setClickedCallback([=] {
		const auto point = state->pressed;
		for (auto i = 0; i != int(state->pills.size()); ++i) {
			if (state->pills[i].contains(point) && value->current() != i) {
				state->from = value->current();
				*value = i;
				state->animation.start(
					[=] { row->update(); },
					0.,
					1.,
					220,
					anim::easeOutCubic);
			}
		}
	});
	row->setPointerCursor(true);
	row->paintRequest() | rpl::on_next([=] {
		auto p = QPainter(row);
		auto hq = PainterHighQualityEnabler(p);
		if (!first) {
			PaintDivider(p, row);
		}
		const auto t = state->animation.value(1.);
		const auto now = value->current();
		p.setFont(font);
		for (auto i = 0; i != int(state->pills.size()); ++i) {
			const auto pill = state->pills[i];
			const auto selected = (i == now)
				? t
				: (i == state->from)
				? (1. - t)
				: 0.;
			p.setPen(Qt::NoPen);
			p.setBrush(anim::color(st::windowBg, st::activeButtonBg, selected));
			p.drawRoundedRect(pill, pill.height() / 2., pill.height() / 2.);
			p.setPen(anim::color(
				st::windowFg,
				st::activeButtonFg,
				selected));
			p.drawText(pill, Qt::AlignCenter, options[i]);
		}
	}, row->lifetime());
}

void ComposeBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		std::shared_ptr<Ui::PreparedFile> file) {
	struct State {
		explicit State(not_null<Main::Session*> session) : poster(session) {
		}
		Poster poster;
		std::optional<float64> progress;
		QImage card;
		bool busy = false;
	};
	const auto session = &peer->session();
	const auto premium = session->premium();
	const auto state = box->lifetime().make_state<State>(session);
	SetupRounded(box);
	AddHeader(box, peer->isSelf()
		? U("Новая история")
		: (U("История в ") + peer->name()));
	const auto content = box->verticalLayout();

	// Превью 9:16 с прогрессом загрузки поверх.
	const auto previewHeight = S(kPreviewHeight);
	const auto previewWidth = previewHeight * 9 / 16;
	const auto preview = content->add(
		object_ptr<Ui::FixedHeightWidget>(content, previewHeight),
		QMargins(S(kOuterSide), S(12), S(kOuterSide), 0));
	preview->paintRequest() | rpl::on_next([=] {
		auto p = QPainter(preview);
		auto hq = PainterHighQualityEnabler(p);
		const auto card = QRect(
			(preview->width() - previewWidth) / 2,
			0,
			previewWidth,
			previewHeight);
		if (state->card.isNull()) {
			state->card = PrepareCard(file->preview, card.size());
		}
		auto path = QPainterPath();
		path.addRoundedRect(card, S(kCardRadius), S(kCardRadius));
		p.setClipPath(path);
		p.drawImage(card.topLeft(), state->card);
		if (!state->progress) {
			return;
		}
		p.fillRect(card, QColor(0, 0, 0, 110));
		const auto size = S(52);
		const auto circle = QRect(
			card.x() + (card.width() - size) / 2,
			card.y() + (card.height() - size) / 2,
			size,
			size);
		auto pen = QPen(QColor(255, 255, 255, 70), S(4));
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(circle);
		pen.setColor(QColor(255, 255, 255));
		pen.setCapStyle(Qt::RoundCap);
		p.setPen(pen);
		const auto value = std::clamp(*state->progress, 0.02, 1.);
		p.drawArc(circle, 90 * 16, -int(360 * 16 * value));
	}, preview->lifetime());

	// Подпись.
	const auto fields = AddCard(content, 14);
	const auto caption = fields->add(
		object_ptr<Ui::InputField>(
			fields,
			CardField(st::defaultInputField),
			Ui::InputField::Mode::MultiLine,
			rpl::single(U("Подпись к истории"))),
		QMargins(S(kPad), S(4), S(kPad), S(4)));
	caption->setMaxLength(session->appConfig().get<int>(
		premium
			? u"story_caption_length_limit_premium"_q
			: u"story_caption_length_limit_default"_q,
		premium ? 2048 : 200));
	InitMessageFieldHandlers(
		controller,
		caption,
		ChatHelpers::PauseReason::Layer);

	// Кому показать и сколько держать.
	const auto audience = std::make_shared<rpl::variable<int>>(
		int(Audience::Contacts));
	const auto period = std::make_shared<rpl::variable<int>>(2);
	const auto pinned = std::make_shared<rpl::variable<bool>>(false);
	const auto protect = std::make_shared<rpl::variable<bool>>(false);
	const auto options = AddCard(content, 12);
	if (peer->isSelf()) {
		AddChoice(
			options,
			U("Кто увидит"),
			{ U("Все"), U("Контакты"), U("Близкие") },
			audience);
	} else {
		*audience = int(Audience::Everyone);
	}
	if (premium) {
		AddChoice(
			options,
			U("Сколько держать"),
			{ U("6 ч"), U("12 ч"), U("24 ч"), U("48 ч") },
			period);
	}
	AddToggle(options, U("Оставить в профиле"), pinned);
	AddToggle(options, U("Запретить пересылку и скриншоты"), protect);

	const auto error = content->add(
		object_ptr<Ui::FlatLabel>(content, QString(), RowLabel()),
		QMargins(S(kOuterSide + kPad), S(8), S(kOuterSide + kPad), 0));
	error->setTextColorOverride(st::boxTextFgError->c);
	error->hide();

	const auto row = content->add(
		object_ptr<Ui::RpWidget>(content),
		QMargins(S(kOuterSide), S(12), S(kOuterSide), 0));
	row->resize(row->width(), PillActive().height);
	const auto button = Ui::CreateChild<Ui::RoundButton>(
		row,
		rpl::single(U("Опубликовать")),
		PillActive());
	row->widthValue() | rpl::on_next([=](int width) {
		button->setFullWidth(width);
		button->moveToLeft(0, 0);
	}, row->lifetime());
	Ui::AddSkip(content, S(kOuterSide));

	const auto post = [=] {
		if (state->busy) {
			return;
		}
		state->busy = true;
		error->hide();
		button->setText(rpl::single(U("Загружаю…")));
		state->progress = 0.;
		preview->update();
		const auto index = std::clamp(period->current(), 0, 3);
		auto copy = Ui::PreparedFile(file->path);
		copy.displayName = file->displayName;
		copy.content = file->content;
		copy.size = file->size;
		if (file->information) {
			copy.information = std::make_unique<Ui::PreparedFileInformation>(
				*file->information);
		}
		copy.preview = file->preview;
		copy.type = file->type;
		state->poster.post(Draft{
			.peer = peer,
			.file = std::move(copy),
			.caption = caption->getTextWithTags(),
			.audience = Audience(audience->current()),
			.period = premium ? kPeriods[index] : kDefaultPeriod,
			.pinned = pinned->current(),
			.protect = protect->current(),
		}, {
			.progress = crl::guard(box, [=](float64 value) {
				state->progress = value;
				preview->update();
			}),
			.done = crl::guard(box, [=] {
				controller->showToast(U("История опубликована"));
				box->closeBox();
			}),
			.failed = crl::guard(box, [=](QString type) {
				state->busy = false;
				state->progress = std::nullopt;
				preview->update();
				button->setText(rpl::single(U("Опубликовать")));
				error->setText(ErrorText(type));
				error->show();
			}),
		});
	};
	button->setClickedCallback(post);
	box->setFocusCallback([=] {
		caption->setFocusFast();
	});
}

[[nodiscard]] bool OneMedia(const Ui::PreparedList &list) {
	if (list.files.size() != 1) {
		return false;
	}
	const auto type = list.files.front().type;
	return (type == Ui::PreparedFile::Type::Photo)
		|| (type == Ui::PreparedFile::Type::Video);
}

void ChooseFile(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	const auto callback = crl::guard(controller, [=](
			FileDialog::OpenResult &&result) {
		auto list = Storage::PreparedFileFromFilesDialog(
			std::move(result),
			OneMedia,
			[=](tr::phrase<> text) {
				controller->showToast(text(tr::now));
			},
			st::sendMediaPreviewSize,
			controller->session().premium());
		if (!list) {
			return;
		} else if (!OneMedia(*list)) {
			controller->showToast(U("Выберите одно фото или видео."));
			return;
		}
		const auto file = std::make_shared<Ui::PreparedFile>(
			std::move(list->files.front()));
		controller->show(Box(ComposeBox, controller, peer, file));
	});
	FileDialog::GetOpenPath(
		controller->content().get(),
		U("Фото или видео для истории"),
		FileDialog::PhotoVideoFilesFilter(),
		callback);
}

} // namespace

void DebugShowCompose(
		not_null<Window::SessionController*> controller,
		const QString &path) {
	auto list = Storage::PrepareMediaList(
		QList<QUrl>{ QUrl::fromLocalFile(path) },
		st::sendMediaPreviewSize,
		controller->session().premium());
	if (list.error != Ui::PreparedList::Error::None || !OneMedia(list)) {
		return;
	}
	const auto file = std::make_shared<Ui::PreparedFile>(
		std::move(list.files.front()));
	controller->show(Box(
		ComposeBox,
		controller,
		not_null<PeerData*>(controller->session().user()),
		file));
}

bool CanPost(not_null<PeerData*> peer) {
	return peer->canPostStories();
}

void StartPosting(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	CheckCanPost(
		&controller->session(),
		peer,
		crl::guard(controller, [=] {
			ChooseFile(controller, peer);
		}),
		crl::guard(controller, [=](QString type) {
			controller->showToast(ErrorText(type));
		}));
}

} // namespace Mokagram::Stories
