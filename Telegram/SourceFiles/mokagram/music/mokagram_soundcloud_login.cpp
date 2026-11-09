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

// Настоящий oauth_token SoundCloud: "2-293746-185937845-AbCd".
// Заголовок JWT и прочие строки не подходят: у них нет ведущей группы цифр.
[[nodiscard]] QRegularExpression TokenPattern() {
	return QRegularExpression(
		u"^[0-9][0-9]*-[0-9]+-[0-9]+-[A-Za-z0-9_-]{6,}$"_q);
}

// Токен приходит из консоли на странице SoundCloud: сам токен, JSON
// {"oauth_token":"..."} или строка "TOKEN: ..."; редко — в base64.
[[nodiscard]] QString TokenFromText(const QString &text) {
	const auto trimmed = text.trimmed();
	const auto bare = trimmed.startsWith(u"TOKEN:"_q)
		? trimmed.mid(6).trimmed()
		: trimmed;
	if (TokenPattern().match(bare).hasMatch() && bare.size() <= 256) {
		return bare;
	}
	const auto pattern = QRegularExpression(
		u"\"(?:oauth_token|access_token)\"\\s*:\\s*\"([0-9][A-Za-z0-9_-]{14,80})\""_q);
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
				Tr("2. На странице SoundCloud нажмите F12, вставьте код ниже в консоль. Он напечатает токен под ним."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));

		// Код для консоли браузера: печатает токен отдельной строкой.
		// copy() в DevTools кладёт в буфер не всегда, поэтому нужен вывод.
		const auto snippet = QString::fromUtf8(
			R"JS((()=>{
const M=/"(?:oauth_token|access_token)"\s*:\s*"([0-9][A-Za-z0-9_-]{14,80})"/;
const out=new Set();
const add=(t)=>{if(!t)return;const m=String(t).match(M);if(m)out.add(m[1]);};
try{for(let i=0;i<localStorage.length;i++){const k=localStorage.key(i);add(k);add(localStorage.getItem(k))}}catch(e){}
try{for(let i=0;i<sessionStorage.length;i++){const k=sessionStorage.key(i);add(k);add(sessionStorage.getItem(k))}}catch(e){}
add(document.cookie);
const dbs=indexedDB.databases?indexedDB.databases():Promise.resolve([]);
dbs.then(list=>Promise.all(list.map(db=>new Promise(r=>{
  const q=indexedDB.open(db.name);
  q.onerror=()=>r();
  q.onsuccess=()=>{
    const d=q.result;const names=[...d.objectStoreNames];
    if(!names.length)return r();
    const tx=d.transaction(names,'readonly');
    Promise.all(names.map(n=>new Promise(r2=>{
      try{const all=tx.objectStore(n).getAll();all.onerror=()=>r2();
        all.onsuccess=()=>{try{add(JSON.stringify(all.result))}catch(e){}r2()}}catch(e){r2()}
    }))).then(()=>{d.close();r()});
  };
})))).then(()=>{
  const v=[...out].sort((a,b)=>b.length-a.length)[0];
  if(v){console.log('TOKEN:',v);try{copy(v)}catch(e){}}
  else{console.log('TOKEN NOT FOUND: откройте soundcloud.com, войдите и повторите на этой вкладке')}
});
})())JS");
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
				Tr("3. Скопируйте напечатанную строку и вставьте её здесь."),
				st::boxLabel),
			style::margins(st::boxRowPadding.left(), st::boxLittleSkip, st::boxRowPadding.right(), st::boxLittleSkip));
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				rpl::single(Tr("Токен SoundCloud")),
				TokenFromText(QGuiApplication::clipboard()->text())),
			st::boxRowPadding);
		const auto pasteButton = Ui::CreateChild<Ui::RoundButton>(
			box,
			rpl::single(Tr("Вставить из буфера")),
			st::defaultLightButton);
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
