// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 3e91a4c7
// Mokagram: вход в SoundCloud через обычный браузер.
// Встроенный вебвью в клиенте не работает, поэтому открываем soundcloud.com
// во внешнем браузере, а токен человек приносит оттуда одной кнопкой.
#pragma once

namespace Mokagram::Music {

// Открывает браузер на SoundCloud и показывает окно вставки токена.
// onLoggedIn зовётся в главном потоке после успешного входа.
void ShowSoundCloudLogin(Fn<void()> onLoggedIn, Fn<void()> onClosed = nullptr);

// Открыть страницу SoundCloud в системном браузере.
[[nodiscard]] bool OpenSoundCloudInBrowser();

} // namespace Mokagram::Music
