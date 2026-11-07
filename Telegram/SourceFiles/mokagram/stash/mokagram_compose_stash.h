// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: отложенный черновик (Ctrl+S) переживает перезапуск клиента.
// Хранится текст, ответ и пересылка; файлы только до закрытия клиента.
#pragma once

#include "data/data_drafts.h"

class History;

namespace Data {
struct ComposeStash;
} // namespace Data

namespace Mokagram::Stash {

void Persist(
	not_null<History*> history,
	Data::DraftKey key,
	const Data::ComposeStash *stash);

// Один раз на чат поднимает черновик с диска в History.
void RestoreIfNeeded(not_null<History*> history, Data::DraftKey key);

} // namespace Mokagram::Stash
