// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: публикация истории с компьютера (фото или видео).
#pragma once

#include "base/basic_types.h"

class PeerData;

namespace Window {
class SessionController;
} // namespace Window

namespace Mokagram::Stories {

// Проверяет лимит, просит выбрать файл и открывает окно публикации.
void StartPosting(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

[[nodiscard]] bool CanPost(not_null<PeerData*> peer);

// Отладка: окно публикации с файлом, без выбора и без отправки.
void DebugShowCompose(
	not_null<Window::SessionController*> controller,
	const QString &path);

} // namespace Mokagram::Stories
