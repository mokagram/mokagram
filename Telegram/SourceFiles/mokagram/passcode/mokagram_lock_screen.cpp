// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: экран код-пароля: часы, дата и карточка с полем.
#include "mokagram/passcode/mokagram_lock_screen.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/passcode/mokagram_passcode_guard.h"
#include "base/timer.h"
#include "lang/lang_keys.h"
#include "ui/effects/animation_value.h"
#include "ui/effects/animations.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "styles/style_window_lock_widgets.h"
#include "settings.h"

#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtWidgets/QApplication>

namespace Mokagram::LockScreen {
namespace {

constexpr auto kName = "mokaLockBackdrop";
constexpr auto kAppearDuration = crl::time(380);
constexpr auto kShakeDuration = crl::time(420);

[[nodiscard]] int S(int value) {
	return style::ConvertScale(value);
}

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

[[nodiscard]] const style::owned_color &Clear() {
	static const auto result = style::owned_color(QColor(0, 0, 0, 0));
	return result;
}

struct Geometry {
	QRect clock;
	QRect date;
	QRect card;
	QRect title;
	QRect field;
	QRect status;
	QRect submit;
	QRect logout;
};

class Backdrop final : public Ui::RpWidget {
public:
	Backdrop(
		not_null<Ui::RpWidget*> parent,
		not_null<QWidget*> field,
		not_null<Ui::RoundButton*> submit,
		not_null<QWidget*> logout);

	void relayout();
	void setError(const QString &error);
	void setBusy(bool busy);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	[[nodiscard]] Geometry geometry() const;
	[[nodiscard]] QString statusText() const;
	[[nodiscard]] bool statusIsError() const;
	void tick();

	const not_null<Ui::RpWidget*> _parent;
	const not_null<QWidget*> _field;
	const not_null<Ui::RoundButton*> _submit;
	const not_null<QWidget*> _logout;
	style::font _clockFont;
	style::font _dateFont;
	style::font _titleFont;
	style::font _statusFont;
	Ui::Animations::Simple _appear;
	Ui::Animations::Simple _shake;
	base::Timer _timer;
	QString _error;
	bool _busy = false;

};

Backdrop::Backdrop(
	not_null<Ui::RpWidget*> parent,
	not_null<QWidget*> field,
	not_null<Ui::RoundButton*> submit,
	not_null<QWidget*> logout)
: RpWidget(parent)
, _parent(parent)
, _field(field)
, _submit(submit)
, _logout(logout)
, _clockFont(S(84), 0, st::semiboldFont->family())
, _dateFont(S(17), 0, 0)
, _titleFont(S(20), 0, st::semiboldFont->family())
, _statusFont(S(14), 0, 0)
, _timer([=] { tick(); }) {
	setObjectName(QString::fromLatin1(kName));
	_appear.start(
		[=] { relayout(); },
		0.,
		1.,
		kAppearDuration,
		anim::easeOutCubic);
	_timer.callEach(250);
}

Geometry Backdrop::geometry() const {
	const auto w = _parent->width();
	const auto h = _parent->height();
	const auto cardWidth = std::min(S(380), w - 2 * S(16));
	const auto inner = cardWidth - 2 * S(28);
	const auto titleHeight = _titleFont->height;
	const auto fieldHeight = S(52);
	const auto statusHeight = _statusFont->height * 2 + S(8);
	const auto submitHeight = S(52);
	const auto logoutHeight = _logout->height();
	const auto cardHeight = S(30)
		+ titleHeight
		+ S(20)
		+ fieldHeight
		+ statusHeight
		+ submitHeight
		+ S(18)
		+ logoutHeight
		+ S(22);
	const auto clockHeight = _clockFont->height;
	const auto dateHeight = _dateFont->height;
	const auto total = clockHeight + dateHeight + S(36) + cardHeight;
	const auto appear = _appear.value(1.);
	const auto rise = anim::interpolate(S(28), 0, appear);
	const auto shift = [&] {
		if (!_shake.animating()) {
			return 0;
		}
		const auto t = _shake.value(1.);
		return int(std::sin(t * M_PI * 6.) * (1. - t) * S(12));
	}();
	auto top = std::max(S(20), (h - total) / 2 - S(16));
	auto result = Geometry();
	result.clock = QRect(0, top, w, clockHeight);
	top += clockHeight;
	result.date = QRect(0, top, w, dateHeight);
	top += dateHeight + S(36) + rise;
	result.card = QRect(
		(w - cardWidth) / 2 + shift,
		top,
		cardWidth,
		cardHeight);
	const auto left = result.card.x() + S(28);
	top += S(30);
	result.title = QRect(left, top, inner, titleHeight);
	top += titleHeight + S(20);
	result.field = QRect(left, top, inner, fieldHeight);
	top += fieldHeight;
	result.status = QRect(left, top, inner, statusHeight);
	top += statusHeight;
	result.submit = QRect(left, top, inner, submitHeight);
	top += submitHeight + S(18);
	result.logout = QRect(
		result.card.x() + (cardWidth - _logout->width()) / 2,
		top,
		_logout->width(),
		logoutHeight);
	return result;
}

void Backdrop::relayout() {
	const auto g = geometry();
	setGeometry(_parent->rect());
	_field->setGeometry(g.field);
	_submit->setFullWidth(g.submit.width());
	_submit->moveToLeft(g.submit.x(), g.submit.y());
	_logout->move(g.logout.x(), g.logout.y());
	update();
}

void Backdrop::setError(const QString &error) {
	if (_error == error) {
		return;
	}
	_error = error;
	if (!error.isEmpty()) {
		_shake.start(
			[=] { relayout(); },
			0.,
			1.,
			kShakeDuration);
	}
	update();
}

void Backdrop::setBusy(bool busy) {
	_busy = busy;
	update();
}

void Backdrop::tick() {
	update();
}

QString Backdrop::statusText() const {
	auto lines = QStringList();
	const auto wait = PasscodeGuard::WaitLeft();
	if (_busy) {
		lines.push_back(U("Проверяю…"));
	} else if (wait > 0) {
		lines.push_back(U("Повторите через %1 с").arg((wait + 999) / 1000));
	} else if (!_error.isEmpty()) {
		lines.push_back(_error);
	}
	const auto left = PasscodeGuard::AttemptsBeforeWipe();
	if (!_busy && left >= 0 && cPasscodeBadTries() > 0) {
		lines.push_back(U("Попыток до стирания: %1").arg(left));
	}
	return lines.join('\n');
}

bool Backdrop::statusIsError() const {
	return !_busy
		&& (!_error.isEmpty() || PasscodeGuard::WaitLeft() > 0);
}

void Backdrop::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto g = geometry();
	const auto appear = _appear.value(1.);

	p.fillRect(rect(), st::windowBg);
	auto tint = st::activeButtonBg->c;
	tint.setAlpha(34);
	auto clear = tint;
	clear.setAlpha(0);
	auto gradient = QRadialGradient(
		QPointF(width() / 2., g.card.y()),
		std::max(width(), height()) * 0.7);
	gradient.setColorAt(0., tint);
	gradient.setColorAt(1., clear);
	p.fillRect(rect(), gradient);

	p.setOpacity(appear);
	const auto now = QDateTime::currentDateTime();
	p.setFont(_clockFont);
	p.setPen(st::windowFg);
	p.drawText(g.clock, Qt::AlignCenter, now.toString(u"HH:mm"_q));
	const auto russian = (U("Код-пароль") == QString::fromUtf8("Код-пароль"));
	const auto locale = russian
		? QLocale(QLocale::Russian)
		: QLocale(QLocale::English);
	auto date = locale.toString(now.date(), u"dddd, d MMMM"_q);
	if (!date.isEmpty()) {
		date[0] = date[0].toUpper();
	}
	p.setFont(_dateFont);
	p.setPen(st::windowSubTextFg);
	p.drawText(g.date, Qt::AlignCenter, date);
	p.setOpacity(1.);

	p.setPen(Qt::NoPen);
	p.setBrush(anim::color(st::windowBg, st::windowFg, 0.05));
	p.drawRoundedRect(g.card, S(28), S(28));

	p.setFont(_titleFont);
	p.setPen(st::windowFg);
	p.drawText(g.title, Qt::AlignCenter, tr::lng_passcode_enter(tr::now));

	const auto focused = _field->hasFocus();
	p.setBrush(st::windowBg);
	p.setPen(focused
		? QPen(st::activeButtonBg->c, S(2) / 1.5)
		: Qt::NoPen);
	p.drawRoundedRect(
		QRectF(g.field).adjusted(0.5, 0.5, -0.5, -0.5),
		g.field.height() / 2.,
		g.field.height() / 2.);

	const auto status = statusText();
	if (!status.isEmpty()) {
		p.setFont(_statusFont);
		p.setPen(statusIsError()
			? st::boxTextFgError->c
			: st::windowSubTextFg->c);
		p.drawText(g.status, Qt::AlignCenter, status);
	}
}

[[nodiscard]] Backdrop *Find(not_null<Ui::RpWidget*> widget) {
	for (const auto child : widget->children()) {
		if (child->objectName() == QLatin1String(kName)) {
			return static_cast<Backdrop*>(child);
		}
	}
	return nullptr;
}

} // namespace

const style::InputField &FieldStyle() {
	static const auto result = [] {
		auto result = st::passcodeInput;
		result.textBg = Clear().color();
		result.textBgActive = Clear().color();
		result.border = 0;
		result.borderActive = 0;
		result.heightMin = S(52);
		result.textMargins = style::margins(S(24), S(16), S(24), S(16));
		result.placeholderMargins = style::margins();
		result.placeholderScale = 0.;
		result.placeholderShift = 0;
		result.placeholderFont = st::normalFont;
		result.placeholderFgActive = st::windowSubTextFg;
		return result;
	}();
	return result;
}

const style::RoundButton &SubmitStyle() {
	static const auto result = [] {
		auto result = st::passcodeSubmit;
		result.height = S(52);
		result.radius = S(26);
		result.fullRadius = true;
		result.textTop = (S(52) - result.style.font->height) / 2;
		return result;
	}();
	return result;
}

void Setup(
		not_null<Ui::RpWidget*> widget,
		not_null<QWidget*> field,
		not_null<Ui::RoundButton*> submit,
		not_null<QWidget*> logout) {
	PasscodeGuard::EnsureLoaded();
	const auto backdrop = Ui::CreateChild<Backdrop>(
		widget.get(),
		field,
		submit,
		logout);
	backdrop->lower();
	backdrop->show();
	QObject::connect(qApp, &QApplication::focusChanged, backdrop, [=] {
		backdrop->update();
	});
	backdrop->relayout();
}

void Layout(not_null<Ui::RpWidget*> widget) {
	if (const auto backdrop = Find(widget)) {
		backdrop->relayout();
	}
}

void SetError(not_null<Ui::RpWidget*> widget, const QString &error) {
	if (const auto backdrop = Find(widget)) {
		backdrop->setError(error);
	}
}

void SetBusy(not_null<Ui::RpWidget*> widget, bool busy) {
	if (const auto backdrop = Find(widget)) {
		backdrop->setBusy(busy);
	}
}

} // namespace Mokagram::LockScreen
