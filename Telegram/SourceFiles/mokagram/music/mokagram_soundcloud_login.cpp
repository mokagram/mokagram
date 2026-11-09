// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 8b14f3d2
#include "mokagram/music/mokagram_soundcloud_login.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/music/mokagram_soundcloud.h"

#include "core/application.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/toast/toast.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "main/main_session.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>

namespace Mokagram::Music {
namespace {

constexpr auto kSoundCloudUrl = "https://soundcloud.com/";

[[nodiscard]] QString Tr(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

// Токен прилетает в clipboard из консоли на странице SoundCloud:
// JSON {"oauth_token":"..."} или сам токен, иногда в base64.
[[nodiscard]] QString TokenFromText(const QString &text) {
	const auto trimmed = text.trimmed();
	if (QRegularExpression(u"^[A-Za-z0-9_-]{16,256}$"_q).match(trimmed).hasMatch()) {
		return trimmed;
	}
	const auto pattern = QRegularExpression(
		u"\"oauth_token\"\\s*:\\s*\"([A-Za-z0-9_-]{16,256})\""_q);
	if (const auto found = pattern.match(trimmed); found.hasMatch()) {
		return found.captured(1);
	}
	const auto decoded = QString::fromUtf8(
		QByteArray::fromBase64(trimmed.toLatin1()));
	if (decoded != trimmed) {
		if (const auto found = pattern.match(decoded); found.hasMatch()) {
			return found.captured(1);
		}
	}
	return QString();
}

} // namespace

bool OpenSoundCloudInBrowser() {
	return QDesktopServices::openUrl(QUrl(QString::fromLatin1(kSoundCloudUrl)));
}

void ShowSoundCloudLogin(Fn<void()> onLoggedIn, Fn<void()> onClosed) {
	const auto window = Core::App().activeWindow();
	const auto controller = window ? window->sessionController() : nullptr;
	if (!controller) {
		Ui::Toast::Show(Tr("Не удалось открыть окно входа."));
		return;
	}
	auto done = std::make_shared<bool>(false);
	const auto finish = [=](bool success) {
		if (*done) {
			return;
		}
		*done = true;
		if (success && onLoggedIn) {
			onLoggedIn();
		} else if (!success && onClosed) {
			onClosed();
		}
	};

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(Tr("Вход в SoundCloud")));
		box->setWidth(st::boxWideWidth);

		const auto content = box->verticalLayout();
		content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				Tr("1. Откройте SoundCloud в браузере и войдите в аккаунт."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), 0, st::boxRowPadding.right(), st::boxLittleSkip));
		const auto openButton = box->addRow(
			object_ptr<Ui::RoundButton>(
				content,
				rpl::single(Tr("Открыть SoundCloud в браузере")),
				st::defaultLightButton),
			style::margins(st::boxRowPadding.left(), 0, st::boxRowPadding.right(), st::boxLittleSkip));
		openButton->setClickedCallback([] {
			if (!OpenSoundCloudInBrowser()) {
				Ui::Toast::Show(Tr("Не удалось открыть браузер."));
			}
		});

		content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				Tr("2. На странице SoundCloud нажмите F12, вставьте код ниже в консоль и скопируйте результат."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));

		// Код для консоли браузера: достаёт токен и кладёт в clipboard.
		const auto snippet = QString::fromUtf8(
			R"JS((()=>{const m=/oauth_token["'\s\\]{0,3}[:=]["'\s\\]{0,3}([A-Za-z0-9_-]{16,256})/;const v=localStorage.getItem('oauth_token')||([...Object.keys(localStorage)].map(k=>localStorage.getItem(k)).join(' ').match(m)||[])[1]||'';copy(v);console.log('Токен скопирован')})())JS");
		const auto snippetLabel = content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				snippet,
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), 0, st::boxRowPadding.right(), st::boxLittleSkip));
		snippetLabel->setSelectable(true);
		const auto copySnippet = box->addRow(
			object_ptr<Ui::RoundButton>(
				content,
				rpl::single(Tr("Скопировать код")),
				st::defaultLightButton),
			style::margins(st::boxRowPadding.left(), 0, st::boxRowPadding.right(), st::boxLittleSkip));
		copySnippet->setClickedCallback([=] {
			QGuiApplication::clipboard()->setText(snippet);
			Ui::Toast::Show(Tr("Код скопирован."));
		});

		content->add(
			object_ptr<Ui::FlatLabel>(
				content,
				Tr("3. Вставьте сюда токен из буфера обмена."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				rpl::single(Tr("Токен SoundCloud")),
				TokenFromText(QGuiApplication::clipboard()->text())),
			st::boxRowPadding);
		box->setFocusCallback([=] { field->setFocusFast(); });

		const auto submit = [=] {
			const auto token = TokenFromText(field->getLastText());
			if (token.isEmpty()) {
				field->showError();
				return;
			}
			Account::Login(token);
			box->closeBox();
			finish(true);
		};
		field->submits() | rpl::on_next(submit, field->lifetime());
		box->addButton(rpl::single(Tr("Подключить")), submit);
		box->addButton(rpl::single(Tr("Отмена")), [=] { box->closeBox(); });

		box->boxClosing() | rpl::on_next([=] {
			crl::on_main([=] { finish(false); });
		}, box->lifetime());

		// Браузер сразу: человеку не нужно искать, где открыть.
		OpenSoundCloudInBrowser();
	}));
}

} // namespace Mokagram::Music
