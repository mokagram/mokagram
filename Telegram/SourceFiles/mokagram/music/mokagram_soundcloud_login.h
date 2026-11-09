// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// mokagram-id: 3e91a4c7
// Mokagram: вход в SoundCloud через окно браузера (OAuth сайта soundcloud.com).
// После входа из сохранённой сессии достаём oauth-токен, он и уходит в аккаунт.
#pragma once

namespace Mokagram::Music {

// Окно входа. onLoggedIn зовётся в главном потоке после успешного входа,
// onClosed — если окно закрыли, не войдя.
void ShowSoundCloudLogin(Fn<void()> onLoggedIn, Fn<void()> onClosed = nullptr);

} // namespace Mokagram::Music
