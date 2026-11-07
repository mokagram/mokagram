// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: экран код-пароля: часы, дата и карточка с полем.
#pragma once

namespace style {
struct InputField;
struct RoundButton;
} // namespace style

namespace Ui {
class RpWidget;
class RoundButton;
} // namespace Ui

namespace Mokagram::LockScreen {

[[nodiscard]] const style::InputField &FieldStyle();
[[nodiscard]] const style::RoundButton &SubmitStyle();

// Подложка под поле, кнопку и ссылку «Выйти» (кладётся под них).
void Setup(
	not_null<Ui::RpWidget*> widget,
	not_null<QWidget*> field,
	not_null<Ui::RoundButton*> submit,
	not_null<QWidget*> logout);
void Layout(not_null<Ui::RpWidget*> widget);
void SetError(not_null<Ui::RpWidget*> widget, const QString &error);
void SetBusy(not_null<Ui::RpWidget*> widget, bool busy);

} // namespace Mokagram::LockScreen
