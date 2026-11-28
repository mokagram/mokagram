// This file is part of mokaGram, https://github.com/mokagram/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#pragma once

#include "mokagram/music/mokagram_lyrics.h"

#include <span>

namespace Mokagram::Asr {
struct TimedWord;
} // namespace Mokagram::Asr

namespace Mokagram::Music {

// Добавляет таймкоды к оригиналу. При слабом совпадении возвращает текст как есть.
[[nodiscard]] Lyrics AlignLyrics(Lyrics source, std::span<const Asr::TimedWord> heard);

} // namespace Mokagram::Music
