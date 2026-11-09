// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 8b14f3d2
#include "mokagram/music/mokagram_soundcloud_login.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/music/mokagram_soundcloud.h"

#include "base/platform/base_platform_info.h"
#include "base/random.h"
#include "styles/style_window.h"
#include "ui/toast/toast.h"
#include "ui/widgets/rp_window.h"
#include "webview/webview_embed.h"
#include "webview/webview_interface.h"

#include <QtCore/QEvent>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>

#include <memory>

namespace Mokagram::Music {
namespace {

constexpr auto kLoginPage = "https://soundcloud.com/signin";

// После загрузки страницы забираем токен из localStorage плеера: он появляется
// там после входа и остаётся, пока вход действует. Значение уходит в C++
// через тот же мост, что и у остальных вебвью (window.external.invoke).
constexpr auto kTokenScript = R"JS((() => {
	// Значение после "oauth_token":"/" или oauth_token=<token>.
	const pattern = /oauth_token["'\\\s]{0,3}[:=]["'\\\s]{0,3}([A-Za-z0-9_-]{16,256})/;
	const send = (value) => {
		try {
			window.TelegramWebviewProxy.postEvent('soundcloud', value);
		} catch (e) {
		}
	};
	const scan = () => {
		try {
			const oauth = localStorage.getItem('oauth_token');
			if (oauth && /^[A-Za-z0-9_-]{16,256}$/.test(oauth)) {
				send(oauth);
				return true;
			}
			for (let i = 0; i < localStorage.length; ++i) {
				const value = localStorage.getItem(localStorage.key(i) || '') || '';
				const found = value.match(pattern);
				if (found) {
					send(found[1]);
					return true;
				}
			}
			const cookie = document.cookie.match(/(?:^|;\s*)oauth_token=([A-Za-z0-9_-]{16,256})/);
			if (cookie) {
				send(cookie[1]);
				return true;
			}
			// Сайт кладёт сессию ещё и в IndexedDB плеера.
			const request = indexedDB.open('soundcloud-web');
			request.onerror = () => {};
			request.onsuccess = () => {
				const db = request.result;
				try {
					const names = Array.from(db.objectStoreNames || []);
					for (const name of names) {
						const store = db.transaction(name, 'readonly').objectStore(name);
						const all = store.getAll();
						all.onerror = () => {};
						all.onsuccess = () => {
							const found = JSON.stringify(all.result || '').match(pattern);
							if (found) {
								send(found[1]);
							}
						};
					}
				} catch (e) {
				}
			};
		} catch (e) {
		}
		return false;
	};
	scan();
	// После входа страница может не перезагрузиться: проверяем раз в полсекунды.
	if (!window.mokaGramScanTimer) {
		window.mokaGramScanTimer = setInterval(scan, 500);
	}
})();)JS";

struct LoginState {
	std::shared_ptr<Ui::RpWindow> window;
	std::unique_ptr<Webview::Window> webview;
	Fn<void()> onLoggedIn;
	Fn<void()> onClosed;
	bool done = false;
};

// Окно входа одно на всё приложение: строка «Лайкнутых» пересоздаётся
// на каждое обновление списка, и без этого открывалось бы новое окно.
std::weak_ptr<LoginState> &ActiveLogin() {
	static auto result = std::weak_ptr<LoginState>();
	return result;
}

void Finish(LoginState *state, bool success) {
	if (state->done) {
		return;
	}
	state->done = true;
	const auto callback = success ? state->onLoggedIn : state->onClosed;
	if (const auto window = state->window) {
		window->close();
	}
	if (callback) {
		callback();
	}
}

[[nodiscard]] QString TokenFromBlob(const QByteArray &blob) {
	// Токен лежит в JSON или в base64 от JSON: ищем его в обоих видах.
	const auto raw = QString::fromUtf8(blob);
	const auto decoded = QByteArray::fromBase64(blob) != blob
		? QString::fromUtf8(QByteArray::fromBase64(blob))
		: QString();
	const auto pattern = QRegularExpression(
		u"\"oauth_token\"\\s*:\\s*\"([A-Za-z0-9_-]{16,256})\""_q);
	for (const auto &text : { raw, decoded }) {
		const auto found = pattern.match(text);
		if (found.hasMatch()) {
			return found.captured(1);
		}
	}
	return QString();
}

} // namespace

void ShowSoundCloudLogin(Fn<void()> onLoggedIn, Fn<void()> onClosed) {
	// Уже открыто: поднимаем то же окно, а не плодим новые.
	if (const auto existing = ActiveLogin().lock()) {
		if (const auto window = existing->window) {
			if (window->isMinimized()) {
				window->showNormal();
			} else if (window->isHidden()) {
				window->show();
			}
			window->raise();
			window->activateWindow();
			return;
		}
	}
	auto state = std::make_shared<LoginState>();
	ActiveLogin() = state;
	state->onLoggedIn = std::move(onLoggedIn);
	state->onClosed = std::move(onClosed);

	const auto s = [](int v) { return style::ConvertScale(v); };
	state->window = std::make_shared<Ui::RpWindow>();
	const auto window = state->window.get();
	window->setTitle(QString::fromUtf8(Mokagram::Lang::Tr("Вход в SoundCloud")));
	window->setMinimumSize(QSize(s(420), s(560)));
	window->setGeometry(QRect(160, 120, s(520), s(720)));
	// Настоящее отдельное окно поверх мессенджера, а не вкладка внутри него.
	window->setAttribute(Qt::WA_QuitOnClose, false);
	window->setWindowFlag(Qt::Window, true);

	const auto body = window->body();
	auto token = std::string(32, ' ');
	base::RandomFill(token.data(), token.size());
	state->webview = std::make_unique<Webview::Window>(
		body,
		Webview::WindowConfig{
			.opaqueBg = QColor(0x11, 0x11, 0x11),
			.storageId = Webview::StorageId{
				.path = Music::CacheDir() + u"/webview"_q,
				.token = QByteArray::fromStdString(token),
			},
			// Сторонние куки нужны кнопкам входа через Google и Apple.
			.allowThirdPartyCookies = true,
		});
	const auto raw = state->webview.get();
	if (!raw->valid() || !raw->widget()) {
		ActiveLogin().reset();
		state->webview.reset();
		state->window.reset();
		Ui::Toast::Show(QString::fromUtf8(Mokagram::Lang::Tr(
			"Не удалось открыть окно браузера.")));
		return;
	}

	const auto weak = std::weak_ptr<LoginState>(state);
	const auto getToken = [raw] {
		raw->eval(QByteArray(kTokenScript));
	};
	const auto layoutWebview = [=] {
		if (const auto widget = raw->widget()) {
			widget->setGeometry(body->rect());
			widget->show();
		}
	};
	body->sizeValue() | rpl::on_next([=](QSize) {
		layoutWebview();
	}, body->lifetime());

	raw->setMessageHandler([=](Webview::Message message) {
		auto text = QString::fromStdString(message.text);
		if (text.startsWith(u"soundcloud:"_q)) {
			text = text.mid(11);
		}
		// Сообщение либо сам токен, либо значение из хранилища целиком.
		auto found = QRegularExpression(
			u"^[A-Za-z0-9_-]{16,256}$"_q).match(text).hasMatch()
			? text
			: TokenFromBlob(text.toUtf8());
		if (found.isEmpty() || !QRegularExpression(
				u"^[A-Za-z0-9_-]{16,256}$"_q).match(found).hasMatch()) {
			return;
		}
		const auto strong = weak.lock();
		if (!strong || strong->done) {
			return;
		}
		Account::Login(found);
		Finish(strong.get(), true);
	});

	raw->setNavigationDoneHandler([=](bool success) {
		if (success) {
			getToken();
		}
	});

	window->events() | rpl::filter([=](not_null<QEvent*> e) {
		return (e->type() == QEvent::Close);
	}) | rpl::on_next([=] {
		if (const auto strong = weak.lock()) {
			if (auto webview = std::move(strong->webview)) {
				webview->navigate(QString());
			}
			Finish(strong.get(), false);
		}
	}, window->lifetime());

	raw->init(
		"window.TelegramWebviewProxy = {"
		"postEvent: function(eventType, eventData) {"
		"if (window.external && window.external.invoke) {"
		"window.external.invoke(JSON.stringify([eventType, eventData]));"
		"}"
		"}"
		"};");
	raw->navigate(QString::fromLatin1(kLoginPage));
	window->show();
	layoutWebview();
	window->raise();
	window->activateWindow();
}

} // namespace Mokagram::Music
