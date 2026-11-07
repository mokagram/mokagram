// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: окно «Ложные пароли» и строка в настройках.
#include "mokagram/passcode/mokagram_fake_passcode.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/settings/mokagram_settings.h"
#include "mokagram/ui/mokagram_box_parts.h"
#include "core/application.h"
#include "data/data_user.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "storage/storage_domain.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <QtGui/QPainterPath>

namespace Mokagram::FakePasscode {
namespace {

using namespace Mokagram::BoxParts;

constexpr auto kBoxWidth = 400;
constexpr auto kPad = 16;
constexpr auto kRowHeight = 52;
constexpr auto kListRowHeight = 60;

[[nodiscard]] int S(int value) {
	return style::ConvertScale(value);
}

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

[[nodiscard]] rpl::event_stream<> &Changes() {
	static auto result = rpl::event_stream<>();
	return result;
}

// Число паролей в moka_settings.json не пишем: файл не зашифрован.
void Notify() {
	Changes().fire({});
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

void AddHeader(
		not_null<Ui::GenericBox*> box,
		const QString &title,
		bool nested) {
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
	const auto left = nested
		? MakeRoundButton(header, RoundIcon::Back, [=] { box->closeBox(); })
		: MakeRoundButton(header, RoundIcon::Close, [=] { box->closeBox(); });
	const auto label = Ui::CreateChild<Ui::FlatLabel>(header, title, titleSt);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	header->widthValue() | rpl::on_next([=](int width) {
		const auto side = round + S(8);
		label->resizeToWidth(std::max(width - 2 * side, 1));
		const auto height = std::max(round, label->height());
		label->moveToLeft(side, (height - label->height()) / 2);
		left->moveToLeft(0, (height - left->height()) / 2);
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

void AddNote(
		not_null<Ui::VerticalLayout*> container,
		const QString &text,
		int top = 8) {
	static const auto noteSt = [] {
		auto result = st::boxDividerLabel;
		result.minWidth = 1;
		return result;
	}();
	container->add(
		object_ptr<Ui::FlatLabel>(container, text, noteSt),
		QMargins(
			S(kOuterSide + kPad),
			S(top),
			S(kOuterSide + kPad),
			0));
}

void PaintDivider(QPainter &p, not_null<QWidget*> row) {
	auto line = st::windowSubTextFg->c;
	line.setAlpha(40);
	p.fillRect(S(kPad), 0, row->width() - S(kPad), st::lineWidth, line);
}

// Строка «подпись + тумблер». Тумблер с анимацией из BoxParts.
not_null<Ui::AbstractButton*> AddToggle(
		not_null<Ui::VerticalLayout*> card,
		const QString &title,
		std::shared_ptr<rpl::variable<bool>> value,
		rpl::producer<bool> active = rpl::single(true)) {
	static const auto labelSt = [] {
		auto result = st::boxLabel;
		result.minWidth = 1;
		return result;
	}();
	const auto first = (card->count() == 0);
	const auto row = card->add(object_ptr<Ui::RpWidget>(card));
	row->resize(row->width(), S(kRowHeight));
	const auto label = Ui::CreateChild<Ui::FlatLabel>(row, title, labelSt);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	const auto toggle = MakeSwitch(
		row,
		value->current(),
		[=](bool checked) { *value = checked; },
		std::move(active));
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
	return toggle;
}

[[nodiscard]] Ui::InputField *AddField(
		not_null<Ui::VerticalLayout*> card,
		const QString &placeholder,
		const QString &value) {
	return card->add(
		object_ptr<Ui::InputField>(
			card,
			CardField(st::defaultInputField),
			rpl::single(placeholder),
			value),
		QMargins(S(kPad), S(4), S(kPad), S(4)));
}

[[nodiscard]] Ui::PasswordInput *AddPassword(
		not_null<Ui::VerticalLayout*> card,
		const QString &placeholder) {
	const auto &fieldSt = CardField(st::defaultInputField);
	// PasswordInput не RpWidget: кладём его в контейнер фиксированной высоты.
	const auto holder = card->add(
		object_ptr<Ui::FixedHeightWidget>(card, fieldSt.heightMin),
		QMargins(S(kPad), S(4), S(kPad), S(4)));
	const auto input = Ui::CreateChild<Ui::PasswordInput>(
		holder,
		fieldSt,
		rpl::single(placeholder));
	holder->widthValue() | rpl::on_next([=](int width) {
		input->setGeometry(0, 0, width, holder->height());
	}, holder->lifetime());
	return input;
}

not_null<Ui::RoundButton*> AddMainButton(
		not_null<Ui::VerticalLayout*> container,
		const QString &text,
		const style::RoundButton &st,
		Fn<void()> callback) {
	const auto row = container->add(
		object_ptr<Ui::RpWidget>(container),
		QMargins(S(kOuterSide), S(12), S(kOuterSide), 0));
	row->resize(row->width(), st.height);
	const auto button = Ui::CreateChild<Ui::RoundButton>(
		row,
		rpl::single(text),
		st);
	button->setClickedCallback(std::move(callback));
	row->widthValue() | rpl::on_next([=](int width) {
		button->setFullWidth(width);
		button->moveToLeft(0, 0);
	}, row->lifetime());
	return button;
}

[[nodiscard]] QString AccountName(int index) {
	for (const auto &entry : Core::App().domain().accounts()) {
		if (entry.index == index) {
			if (const auto session = entry.account->maybeSession()) {
				return session->user()->name();
			}
		}
	}
	return QString();
}

[[nodiscard]] QString Summary(const Entry &entry) {
	auto parts = QStringList();
	if (entry.actions.deleteAll) {
		parts.push_back(U("стирает всё"));
	} else {
		if (!entry.actions.logoutAccounts.empty()) {
			parts.push_back(U("выход из аккаунтов: ")
				+ QString::number(entry.actions.logoutAccounts.size()));
		}
		if (entry.actions.clearCache) {
			parts.push_back(U("чистит кэш"));
		}
		if (entry.actions.clearProxies) {
			parts.push_back(U("удаляет прокси"));
		}
	}
	const auto what = parts.isEmpty()
		? U("ничего не делает")
		: parts.join(u", "_q);
	return (entry.unlocks ? U("Открывает клиент") : U("Не открывает"))
		+ u" · "_q
		+ what;
}

void EditBox(not_null<Ui::GenericBox*> box, std::optional<Entry> existing) {
	SetupRounded(box);
	const auto adding = !existing.has_value();
	AddHeader(
		box,
		adding ? U("Новый ложный пароль") : existing->name,
		true);
	const auto content = box->verticalLayout();
	const auto initial = existing.value_or(Entry{ .unlocks = true });

	const auto fields = AddCard(content, 14);
	const auto name = AddField(fields, U("Название"), initial.name);
	name->setMaxLength(32);
	const auto passcode = adding
		? AddPassword(fields, U("Ложный пароль"))
		: nullptr;
	const auto real = adding
		? AddPassword(fields, U("Ваш настоящий код-пароль"))
		: nullptr;
	if (adding) {
		AddNote(content, U("Поставьте ложный пароль, который можно назвать вместо настоящего. Кто его введёт, ничего не заметит."));
	}

	const auto unlocks = std::make_shared<rpl::variable<bool>>(
		initial.unlocks);
	const auto deleteAll = std::make_shared<rpl::variable<bool>>(
		initial.actions.deleteAll);
	const auto clearCache = std::make_shared<rpl::variable<bool>>(
		initial.actions.clearCache);
	const auto clearProxies = std::make_shared<rpl::variable<bool>>(
		initial.actions.clearProxies);

	const auto mode = AddCard(content, 14);
	if (adding) {
		AddToggle(
			mode,
			U("Открывать mokaGram"),
			unlocks,
			deleteAll->value() | rpl::map([](bool v) { return !v; }));
		AddNote(content, U("Включено: клиент откроется как обычно и тихо выполнит действия. Выключено: экран останется заблокированным, будто пароль неверный."));
	} else {
		mode->hide();
	}

	AddNote(content, U("ЧТО СДЕЛАТЬ"), 14);
	const auto what = AddCard(content, 6);
	auto accounts = std::vector<std::pair<int, std::shared_ptr<rpl::variable<bool>>>>();
	for (const auto &entry : Core::App().domain().accounts()) {
		if (!entry.account->maybeSession()) {
			continue;
		}
		const auto value = std::make_shared<rpl::variable<bool>>(
			ranges::contains(initial.actions.logoutAccounts, entry.index));
		accounts.emplace_back(entry.index, value);
		AddToggle(
			what,
			U("Выйти из ") + AccountName(entry.index),
			value,
			deleteAll->value() | rpl::map([](bool v) { return !v; }));
	}
	AddToggle(
		what,
		U("Очистить кэш"),
		clearCache,
		deleteAll->value() | rpl::map([](bool v) { return !v; }));
	AddToggle(what, U("Удалить все прокси"), clearProxies);
	AddToggle(what, U("Стереть всё"), deleteAll);
	AddNote(content, U("«Стереть всё» выходит из всех аккаунтов и удаляет кэш. Клиент при этом не открывается."));

	const auto error = content->add(
		object_ptr<Ui::FlatLabel>(content, QString(), st::boxLabel),
		QMargins(S(kOuterSide + kPad), S(8), S(kOuterSide + kPad), 0));
	error->setTextColorOverride(st::attentionButtonFg->c);
	error->hide();
	const auto showError = [=](const QString &text) {
		error->setText(text);
		error->setVisible(!text.isEmpty());
	};

	const auto collect = [=] {
		auto result = Actions{
			.clearCache = clearCache->current(),
			.clearProxies = clearProxies->current(),
			.deleteAll = deleteAll->current(),
		};
		if (!result.deleteAll) {
			for (const auto &[index, value] : accounts) {
				if (value->current()) {
					result.logoutAccounts.push_back(index);
				}
			}
		}
		return result;
	};
	const auto saving = std::make_shared<bool>(false);
	const auto weak = base::make_weak(box.get());
	const auto save = std::make_shared<Fn<void()>>();
	// Enter переводит к следующему полю, в последнем сохраняет.
	name->submits() | rpl::on_next([=] {
		if (passcode) {
			passcode->setFocus();
		} else {
			(*save)();
		}
	}, name->lifetime());
	if (adding) {
		QObject::connect(passcode, &Ui::MaskedInputField::submitted, [=] {
			real->setFocus();
		});
		QObject::connect(real, &Ui::MaskedInputField::submitted, [=] {
			(*save)();
		});
	}
	*save = [=] {
		if (*saving) {
			return;
		}
		const auto title = name->getLastText().trimmed().isEmpty()
			? U("Ложный пароль")
			: name->getLastText().trimmed();
		if (!adding) {
			Update(existing->id, title, collect());
			Notify();
			box->closeBox();
			return;
		}
		if (passcode->getLastText().isEmpty()) {
			passcode->showError();
			return;
		} else if (real->getLastText().isEmpty()) {
			real->showError();
			return;
		}
		*saving = true;
		showError(U("Проверяю пароль…"));
		Add(Entry{
			.name = title,
			.passcode = passcode->getLastText(),
			.unlocks = unlocks->current() && !deleteAll->current(),
			.actions = collect(),
		}, real->getLastText(), [=](AddResult result) {
			if (!weak) {
				Notify();
				return;
			}
			*saving = false;
			switch (result) {
			case AddResult::Ok:
				Notify();
				box->closeBox();
				return;
			case AddResult::WrongPasscode:
				real->showError();
				showError(U("Неверный код-пароль"));
				return;
			case AddResult::SameAsReal:
				passcode->showError();
				showError(U("Совпадает с настоящим паролем"));
				return;
			case AddResult::Duplicate:
				passcode->showError();
				showError(U("Такой ложный пароль уже есть"));
				return;
			case AddResult::TooMany:
				showError(U("Больше восьми ложных паролей нельзя"));
				return;
			case AddResult::Busy:
				showError(U("Подождите секунду и повторите"));
				return;
			default:
				showError(U("Не удалось сохранить"));
				return;
			}
		});
	};
	AddMainButton(
		content,
		adding ? U("Добавить") : U("Сохранить"),
		PillActive(),
		[=] { (*save)(); });
	if (!adding) {
		const auto id = existing->id;
		AddMainButton(content, U("Удалить пароль"), PillAttention(), [=] {
			Remove(id);
			Notify();
			box->closeBox();
		});
	}
	Ui::AddSkip(content, S(kOuterSide));
	box->setFocusCallback([=] {
		if (passcode) {
			passcode->setFocusFast();
		} else {
			name->setFocusFast();
		}
	});
}

void FillList(
		not_null<Ui::GenericBox*> box,
		not_null<Ui::VerticalLayout*> list) {
	static const auto nameSt = [] {
		auto result = st::boxLabel;
		result.minWidth = 1;
		result.maxHeight = st::boxLabel.style.font->height;
		return result;
	}();
	static const auto aboutSt = [] {
		auto result = st::boxDividerLabel;
		result.minWidth = 1;
		result.maxHeight = st::boxDividerLabel.style.font->height;
		result.margin = style::margins();
		return result;
	}();
	list->clear();
	const auto entries = List();
	if (entries.empty()) {
		list->hide();
		return;
	}
	list->show();
	for (const auto &entry : entries) {
		const auto first = (list->count() == 0);
		const auto row = list->add(object_ptr<Ui::RippleButton>(
			list,
			st::defaultRippleAnimation));
		row->resize(row->width(), S(kListRowHeight));
		row->setPointerCursor(true);
		row->setClickedCallback([=] {
			box->uiShow()->showBox(Box(EditBox, std::make_optional(entry)));
		});
		const auto title = Ui::CreateChild<Ui::FlatLabel>(
			row,
			entry.name,
			nameSt);
		const auto about = Ui::CreateChild<Ui::FlatLabel>(
			row,
			Summary(entry),
			aboutSt);
		title->setAttribute(Qt::WA_TransparentForMouseEvents);
		about->setAttribute(Qt::WA_TransparentForMouseEvents);
		row->widthValue() | rpl::on_next([=](int width) {
			const auto available = std::max(width - 3 * S(kPad), 1);
			title->resizeToWidth(available);
			about->resizeToWidth(available);
			const auto total = title->height() + S(2) + about->height();
			const auto top = (row->height() - total) / 2;
			title->moveToLeft(S(kPad), top);
			about->moveToLeft(S(kPad), top + title->height() + S(2));
		}, row->lifetime());
		row->paintRequest() | rpl::on_next([=] {
			auto p = QPainter(row);
			auto path = QPainterPath();
			path.addRoundedRect(
				QRectF(-row->x(), -row->y(), list->width(), list->height()),
				S(kCardRadius),
				S(kCardRadius));
			p.save();
			p.setClipPath(path);
			row->paintRipple(p, 0, 0);
			p.restore();
			if (!first) {
				PaintDivider(p, row);
			}
			auto hq = PainterHighQualityEnabler(p);
			auto pen = QPen(st::windowSubTextFg->c, S(16) / 10.);
			pen.setCapStyle(Qt::RoundCap);
			p.setPen(pen);
			const auto x = row->width() - S(kPad) - S(4) + 0.;
			const auto y = row->height() / 2.;
			p.drawLine(QPointF(x - S(3), y - S(4)), QPointF(x, y));
			p.drawLine(QPointF(x, y), QPointF(x - S(3), y + S(4)));
		}, row->lifetime());
	}
	list->resizeToWidth(list->width());
}

void ListBox(not_null<Ui::GenericBox*> box) {
	SetupRounded(box);
	AddHeader(box, U("Ложные пароли"), false);
	const auto content = box->verticalLayout();
	AddNote(content, U("Пароль для чужих глаз. Его вводят вместо настоящего, а клиент тихо выходит из аккаунтов, чистит кэш или стирает всё."), 10);
	if (!Core::App().domain().local().hasLocalPasscode()) {
		AddNote(content, U("Сначала поставьте код-пароль: Настройки, Конфиденциальность, Код-пароль."), 14);
		Ui::AddSkip(content, S(kOuterSide));
		return;
	}
	const auto list = AddCard(content, 14);
	FillList(box, list);
	Changes().events() | rpl::on_next([=] {
		FillList(box, list);
	}, list->lifetime());
	const auto add = AddMainButton(
		content,
		U("Добавить ложный пароль"),
		PillActive(),
		[=] {
			box->uiShow()->showBox(Box(EditBox, std::optional<Entry>()));
		});
	Changes().events_starting_with({}) | rpl::on_next([=] {
		add->setVisible(List().size() < size_t(kMaxEntries));
	}, add->lifetime());
	Ui::AddSkip(content, S(kOuterSide));
}

} // namespace

void RegisterSettings() {
	Settings::Register({
		.key = u"fakepass.label"_q,
		.section = U("Код-пароль"),
		.title = U("Ложные пароли"),
		.description = U("Пароль для чужих глаз: тихо выходит из аккаунтов, чистит кэш или стирает всё."),
		.kind = Settings::Kind::String,
		.defaultValue = U("Настроить"),
		.category = u"privacy"_q,
		.sectionIcon = u"lock"_q,
		.icon = u"lock"_q,
		.action = [] {
			if (const auto window = Core::App().activePrimaryWindow()) {
				if (const auto controller = window->sessionController()) {
					controller->show(Box(ListBox));
				}
			}
		},
	});
}

} // namespace Mokagram::FakePasscode
