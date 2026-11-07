// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: защищённые чаты без запретов. Скриншоты, копирование текста,
// сохранение медиа и пересылка; пересылка идёт копией (файлы заливаются
// заново), потому что сервер не пересылает из таких чатов.
#pragma once

#include "api/api_common.h"
#include "base/basic_types.h"

class HistoryItem;
class PeerData;

namespace Main {
class Session;
} // namespace Main

namespace Mokagram::Forward {

// Включён ли обход (настройка). Внутри RawScope всегда false.
[[nodiscard]] bool Bypass();

// Пока жив, проверки видят настоящие запреты сервера.
class RawScope final {
public:
	RawScope();
	~RawScope();
};

[[nodiscard]] bool NeedsCopy(not_null<HistoryItem*> item);

// Отправляет копии сообщений по одному, альбомы альбомами.
void CopyForward(
	not_null<Main::Session*> session,
	std::vector<not_null<HistoryItem*>> items,
	Api::SendAction action,
	bool dropCaptions);

void RegisterSettings();

} // namespace Mokagram::Forward
