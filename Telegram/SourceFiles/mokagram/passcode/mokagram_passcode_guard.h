// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: охрана код-пароля: стирание после неверных попыток,
// автоблокировка при уходе из окна и блокировке экрана компьютера.
#pragma once

#include "base/basic_types.h"

namespace Mokagram::PasscodeGuard {

// Неверные попытки переживают перезапуск клиента.
void EnsureLoaded();

// true, если эта ошибка запустила стирание.
bool RegisterFailure();
void RegisterSuccess();

// Сколько ждать до следующей попытки.
[[nodiscard]] crl::time WaitLeft();
// -1, если стирание после попыток выключено.
[[nodiscard]] int AttemptsBeforeWipe();

// Автоблокировка: следит за окном и экраном компьютера.
void Init();
void ScreenLocked();

void RegisterSettings();

} // namespace Mokagram::PasscodeGuard
