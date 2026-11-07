// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: точное «был в сети» без Premium. На пару секунд открываем
// свой «последний визит» одному человеку, читаем его время и возвращаем
// правило обратно.
#pragma once

#include "base/basic_types.h"

class PeerData;
class UserData;

namespace Main {
class Session;
} // namespace Main

namespace Mokagram::Peek {

[[nodiscard]] bool Available(not_null<UserData*> user);
void Start(not_null<UserData*> user);

// Правило, которое не успели вернуть (клиент закрыли посреди проверки).
void RestoreIfNeeded(not_null<Main::Session*> session);

// Telegram пишет «был недавно»: дописываем подсмотренное время.
[[nodiscard]] QString Augment(
	not_null<UserData*> user,
	const QString &telegramText,
	TimeId now);

} // namespace Mokagram::Peek
