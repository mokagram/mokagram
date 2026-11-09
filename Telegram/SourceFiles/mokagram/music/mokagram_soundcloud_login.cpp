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
#include <QtCore/QPointer>
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

// oauth_token SoundCloud — JWT: три части через точку, каждая из base64url.
// Иногда встречается и старый вид "2-293746-185937845-AbCd".
[[nodiscard]] QRegularExpression TokenPattern() {
	return QRegularExpression(
		u"^([A-Za-z0-9_-]{10,}\\.[A-Za-z0-9_-]{10,}\\.[A-Za-z0-9_-]{10,}"
		u"|[0-9][0-9]*-[0-9]+-[0-9]+-[A-Za-z0-9_-]{6,})$"_q);
}

// Токен приходит из консоли на странице SoundCloud: сам токен, JSON
// {"oauth_token":"..."} или строка "TOKEN: ..."; редко — в base64.
[[nodiscard]] QString TokenFromText(const QString &text) {
	const auto trimmed = text.trimmed();
	const auto bare = trimmed.startsWith(u"TOKEN:"_q)
		? trimmed.mid(6).trimmed()
		: trimmed;
	if (TokenPattern().match(bare).hasMatch() && bare.size() <= 4096) {
		return bare;
	}
	const auto pattern = QRegularExpression(
		u"\"(?:oauth_token|access_token)\"\\s*:\\s*\""
		u"([A-Za-z0-9_.-]{20,4096})\""_q);
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
				Tr("2. Скопируйте короткий скрипт и вставьте его в консоль SoundCloud. Он найдёт токен автоматически."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));

		// Код для консоли браузера: печатает токен отдельной строкой.
		// copy() в DevTools кладёт в буфер не всегда, поэтому нужен вывод.
		const auto snippet = QString::fromUtf8(
			R"JS((()=>{
// Токен лежит в заголовке авторизации и в URL запросов к api-v2.
// Ищем и там, и там; формат — JWT из трёх частей через точку.
const JWT=/([A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,})/;
const out=new Set();
const add=(v)=>{if(!v)return;const m=String(v).match(JWT);if(m)out.add(m[1])};
const done=()=>{if(!out.size)return false;const v=[...out].sort((a,b)=>b.length-a.length)[0];
  console.log('TOKEN:',v);try{copy(v)}catch(e){}return true};
const scanText=(t)=>{
  if(!t)return;
  const s=String(t);
  const u=s.match(/oauth_token=([A-Za-z0-9_.%-]+)/);
  if(u)add(decodeURIComponent(u[1]));
  const j=s.match(/"(?:oauth_token|access_token)"\s*:\s*"([A-Za-z0-9_.-]{20,4096})"/);
  if(j)add(j[1]);
};
// Заголовок Authorization у запросов.
const of=window.fetch;
window.fetch=function(...a){
  try{const h=(a[1]&&a[1].headers)||(a[0]&&a[0].headers);
    if(h&&h.get)add(h.get('Authorization')||h.get('authorization'));}catch(e){}
  return of.apply(this,a).then(r=>{
    try{r.clone().text().then(t=>{scanText(t);done()}).catch(()=>{})}catch(e){}
    return r;
  });
};
const oo=XMLHttpRequest.prototype.open;
const os=XMLHttpRequest.prototype.send;
XMLHttpRequest.prototype.open=function(m,u){this.__mu=u;return oo.apply(this,arguments)};
XMLHttpRequest.prototype.send=function(){
  this.addEventListener('load',()=>{try{scanText(this.responseText);done()}catch(e){}});
  try{scanText(String(this.__mu))}catch(e){}
  return os.apply(this,arguments);
};
try{scanText(document.cookie)}catch(e){}
done();
console.log('Перехват включён. Обновите страницу (F5) и дождитесь TOKEN: в консоли.');
setTimeout(()=>{if(!done())console.log('TOKEN пока не найден: обновите страницу (F5) и прокрутите ленту.')},2000);
})())JS");
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
				Tr("3. Скопируйте строку после TOKEN: и вставьте её здесь."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				rpl::single(Tr("Токен SoundCloud")),
				TokenFromText(QGuiApplication::clipboard()->text())),
			st::boxRowPadding);
		const auto pasteButton = box->addRow(
			object_ptr<Ui::RoundButton>(
				content,
				rpl::single(Tr("Вставить из буфера")),
				st::defaultLightButton),
			style::margins(
				st::boxRowPadding.left(),
				0,
				st::boxRowPadding.right(),
				st::boxLittleSkip));
		pasteButton->setClickedCallback([=] {
			const auto fromClipboard = TokenFromText(
				QGuiApplication::clipboard()->text());
			if (fromClipboard.isEmpty()) {
				Ui::Toast::Show(Tr("В буфере обмена нет токена."));
				return;
			}
			field->setText(fromClipboard);
			field->setFocusFast();
		});
		box->setFocusCallback([=] { field->setFocusFast(); });

		const auto submit = [=] {
			const auto token = TokenFromText(field->getLastText());
			if (token.isEmpty()) {
				field->showError();
				return;
			}
			const auto guardedBox = QPointer<Ui::GenericBox>(box);
			Account::Login(token, [=](QString error) {
				if (!guardedBox) {
					return;
				}
				if (!error.isEmpty()) {
					Ui::Toast::Show(error);
					return;
				}
				guardedBox->closeBox();
				finish(true);
			});
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
