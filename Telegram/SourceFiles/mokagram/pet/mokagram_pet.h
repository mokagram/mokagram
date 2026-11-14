// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: питомец Clawd на верхнем крае баббла поля ввода.
// Кадры только из набора Clawd_by_PackCreatorBot, клиент грузит его по API.
#pragma once

#include "base/basic_types.h"

#include <QtCore/QRect>
#include <QtCore/QString>

#include <rpl/producer.h>

#include <optional>

class History;
class QWidget;

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Mokagram::Pet {

void RegisterSettings();

struct Host {
	not_null<Ui::RpWidget*> parent;
	not_null<Main::Session*> session;
	// Баббл поля ввода в координатах parent, пустой = питомца не видно.
	Fn<QRect()> bubble;
	Fn<History*()> history;
	// Курсор набора текста в координатах parent.
	Fn<std::optional<QPoint>()> caret;
	// Видимая часть ленты (без шапки и закрепа) в координатах parent.
	Fn<QRect()> area;
	// Кнопка поля, под которой лежит питомец (панели и кнопки выше него).
	Fn<QWidget*()> under;
	rpl::producer<> typing;
};

// Питомец живёт, пока жив parent. Тумблер pet.clawd включает и выключает его.
void Attach(Host host);

// Отладка (MOKAGRAM_DEBUG_CMD): clawd, clawdsheet:<png>, clawd:<действие>.
bool DebugCommand(const QString &cmd, not_null<Main::Session*> session);

} // namespace Mokagram::Pet
