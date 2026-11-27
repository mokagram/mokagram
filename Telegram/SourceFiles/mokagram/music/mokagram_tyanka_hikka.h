// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#pragma once

#include <QtCore/QString>

class HistoryItem;

namespace Mokagram::Music {
struct Track;
} // namespace Mokagram::Music

namespace Mokagram::TyankaHikka {

enum class WaffleBark {
	Double,
	Single,
	Triple,
	Alternate,
};

void Start();
void Finish();
[[nodiscard]] bool IsArtist(const QString &name);
void Unlock();
void PlayWaffleBarks(const Music::Track &track, WaffleBark bark);
void OnOwnMessage();
void OnIncomingMessage(not_null<HistoryItem*> item);

} // namespace Mokagram::TyankaHikka
