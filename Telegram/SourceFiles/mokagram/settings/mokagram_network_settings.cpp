// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#include "mokagram/settings/mokagram_network_settings.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/settings/mokagram_settings.h"
#include "mtproto/connection_tcp.h"

namespace Mokagram::Network {

void RegisterSettings() {
	const auto kWebSocketSetting = u"net.telegram_websocket"_q;
	Settings::Register({
		.key = kWebSocketSetting,
		.section = QString::fromUtf8(Mokagram::Lang::Tr("Подключение")),
		.title = QString::fromUtf8(Mokagram::Lang::Tr(
			"Пробовать WebSocket для Telegram")),
		.description = QString::fromUtf8(Mokagram::Lang::Tr(
			"При прямом подключении сначала используется WebSocket, при сбое клиент подключится по TCP.")),
		.kind = Settings::Kind::Bool,
		.defaultValue = false,
		.requiresRestart = true,
		.category = u"other"_q,
		.sectionIcon = u"link"_q,
	});
	// Mokagram: apply once at startup; this option requires a client restart.
	MTP::SetTelegramWebSocketTransportEnabled(
		Settings::Bool(kWebSocketSetting));
}

} // namespace Mokagram::Network
