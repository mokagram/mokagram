// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: мелкие настройки: полоса историй, спойлеры, свои закрепы,
// метаданные файлов.
#pragma once

#include "base/basic_types.h"

class PeerData;

namespace Mokagram::Tweaks {

[[nodiscard]] bool HideStories();
[[nodiscard]] bool RevealSpoilers();

// Закрепы только у себя, сервер о них не знает. 0 значит не закреплён,
// меньше число выше в списке.
[[nodiscard]] int LocalPinIndex(not_null<PeerData*> peer);
[[nodiscard]] bool IsLocallyPinned(not_null<PeerData*> peer);
void ToggleLocalPin(not_null<PeerData*> peer);

// Убирает из JPEG и PNG геотег, камеру, автора и время съёмки.
// Поворот кадра сохраняется. Другие форматы возвращаются как есть.
[[nodiscard]] QByteArray StripMetadata(const QByteArray &bytes);
[[nodiscard]] bool StripsMetadata();

void RegisterSettings();

} // namespace Mokagram::Tweaks
