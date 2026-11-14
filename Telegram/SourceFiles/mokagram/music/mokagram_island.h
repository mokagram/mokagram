/*
This file is part of mokaGram, a Telegram Desktop fork.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Window {
class SessionController;
} // namespace Window

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Mokagram::Music {

// Островок вместо полоски плеера, create = false только двигает созданный.
void UpdateIsland(
	not_null<Ui::RpWidget*> parent,
	not_null<Window::SessionController*> controller,
	not_null<Ui::RpWidget*> anchor,
	bool create);

} // namespace Mokagram::Music
