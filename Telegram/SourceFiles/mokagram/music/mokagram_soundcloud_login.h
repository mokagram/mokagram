// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 3e91a4c7
// Mokagram: вход в SoundCloud через обычный браузер.
// Встроенный вебвью в клиенте не работает. На Linux токен забирается из куков
// Chromium-браузера через headless-копию профиля, вручную его вводят
// только если автоматика не сработала.
#pragma once

class QPainter;
class QRectF;

namespace Mokagram::Music {

// Показывает окно входа: автовход через браузер и ручной ввод токена.
// onLoggedIn зовётся в главном потоке после успешного входа.
void ShowSoundCloudLogin(Fn<void()> onLoggedIn, Fn<void()> onClosed = nullptr);

// Тихо берёт свежий токен из куков браузера (Linux) и входит с ним.
// Токен SoundCloud живёт сутки, потом лайки отвечают 401.
void RefreshSoundCloudToken(Fn<void(bool)> done);

// Круглый оранжевый значок SoundCloud со знаком: облако и столбики звука.
void PaintSoundCloudBadge(QPainter &p, const QRectF &r);

// Открыть страницу SoundCloud в системном браузере.
[[nodiscard]] bool OpenSoundCloudInBrowser();

} // namespace Mokagram::Music
