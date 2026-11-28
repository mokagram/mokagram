// This file is part of mokaGram. See LEGAL for license information.
#pragma once

namespace Ui {
class IconButton;
} // namespace Ui

namespace Window {
class SectionMemento;
} // namespace Window

namespace Mokagram::TikTok {

[[nodiscard]] Ui::IconButton *MakeButton(QWidget *parent);
[[nodiscard]] std::shared_ptr<Window::SectionMemento> MakeSection();
[[nodiscard]] bool IsSection(not_null<Window::SectionMemento*> memento);

} // namespace Mokagram::TikTok
