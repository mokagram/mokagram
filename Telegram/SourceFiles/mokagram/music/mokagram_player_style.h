// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#pragma once

#include "styles/style_basic.h"

namespace Mokagram::Music::PlayerStyle {

struct TextStyle {
	int size;
	int leading;
	bool emphasized = false;

	[[nodiscard]] style::font font() const {
		const auto base = emphasized ? st::semiboldFont : st::normalFont;
		return style::font(style::ConvertScale(size), base->flags(), base->family());
	}

	[[nodiscard]] int lineHeight() const {
		return style::ConvertScale(leading);
	}
};

inline constexpr auto kTrackTitle = TextStyle{ 22, 30, true };
inline constexpr auto kCompactTitle = TextStyle{ 16, 24, true };
inline constexpr auto kTrackArtist = TextStyle{ 15, 22 };
inline constexpr auto kCompactArtist = TextStyle{ 14, 20 };
inline constexpr auto kControlLabel = TextStyle{ 15, 20, true };
inline constexpr auto kTimeLabel = TextStyle{ 12, 16 };
inline constexpr auto kLyrics = TextStyle{ 36, 44, true };
inline constexpr auto kCompactLyrics = TextStyle{ 24, 32, true };
inline constexpr auto kLyricsSubtitle = TextStyle{ 18, 26 };

inline constexpr auto kSpace2 = 2;
inline constexpr auto kSpace4 = 4;
inline constexpr auto kSpace8 = 8;
inline constexpr auto kSpace12 = 12;
inline constexpr auto kSpace16 = 16;
inline constexpr auto kSpace24 = 24;
inline constexpr auto kSpace32 = 32;

} // namespace Mokagram::Music::PlayerStyle
