// This file is part of mokaGram. See LEGAL for license information.
#include "mokagram/tiktok/mokagram_tiktok.h"

#include "lang/lang_keys.h"
#include "styles/style_info.h"
#include "styles/style_widgets.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "webview/webview_embed.h"
#include "webview/webview_interface.h"
#include "window/section_memento.h"
#include "window/section_widget.h"

#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QUrl>

namespace Mokagram::TikTok {
namespace {

constexpr auto kTitle = "TikTok";
constexpr auto kUrl = "https://www.tiktok.com/";
constexpr auto kWidthProperty = "mokaTikTokWidth";

// The desktop feed has a minimum width for navigation, video and actions.
constexpr auto kFitPage = R"JS(
(() => {
	if (window.top !== window) return;
	let frame = 0;
	let applied = 0;
	const fit = () => {
		frame = 0;
		const zoom = Math.min(1, window.innerWidth / 640);
		if (!zoom || zoom === applied) return;
		applied = zoom;
		const root = document.documentElement;
		root.style.setProperty('zoom', String(zoom), 'important');
		root.style.setProperty('overflow-x', 'hidden', 'important');
	};
	const schedule = () => {
		if (!frame) frame = requestAnimationFrame(fit);
	};
	window.addEventListener('resize', schedule, { passive: true });
	document.addEventListener('DOMContentLoaded', schedule, { once: true });
	schedule();
})();
)JS";

// TikTok brand mark from Simple Icons (CC0).
constexpr auto kLogoPath =
	"M12.525.02c1.31-.02 2.61-.01 3.91-.02.08 1.53.63 3.09 1.75 4.17"
	" 1.12 1.11 2.7 1.62 4.24 1.79v4.03c-1.44-.05-2.89-.35-4.2-.97"
	"-.57-.26-1.1-.59-1.62-.93-.01 2.92.01 5.84-.02 8.75-.08 1.4-.54"
	" 2.79-1.35 3.94-1.31 1.92-3.58 3.17-5.91 3.21-1.43.08-2.86-.31"
	"-4.08-1.03-2.02-1.19-3.44-3.37-3.65-5.71-.02-.5-.03-1-.01-1.49"
	".18-1.9 1.12-3.72 2.58-4.96 1.66-1.44 3.98-2.13 6.15-1.72"
	".02 1.48-.04 2.96-.04 4.44-.99-.32-2.15-.23-3.02.37-.63.41"
	"-1.11 1.04-1.36 1.75-.21.51-.15 1.07-.14 1.61.24 1.64 1.82"
	" 3.02 3.5 2.87 1.12-.01 2.19-.66 2.77-1.61.19-.33.4-.67"
	".41-1.06.1-1.79.06-3.57.07-5.36.01-4.03-.01-8.05.02-12.07z";

class Button final : public Ui::IconButton {
public:
	explicit Button(QWidget *parent)
	: IconButton(parent, st::topBarSearch) {
		setAccessibleName(QString::fromLatin1(kTitle));
	}

protected:
	void paintEvent(QPaintEvent *) override {
		auto p = QPainter(this);
		paintRipple(p, st().rippleAreaPosition);
		const auto color = isOver() ? st::menuIconFgOver : st::menuIconFg;
		const auto size = style::ConvertScale(18);
		const auto svg = QByteArray("<svg xmlns='http://www.w3.org/2000/svg'"
			" viewBox='0 0 24 24'><path fill='")
			+ color->c.name().toLatin1()
			+ "' d='" + kLogoPath + "'/></svg>";
		auto renderer = QSvgRenderer(svg);
		renderer.render(&p, QRectF(
			(width() - size) / 2.,
			(height() - size) / 2.,
			size,
			size));
	}
};

class Section final : public Window::SectionWidget {
public:
	Section(
		QWidget *parent,
		not_null<Window::SessionController*> controller);
	~Section() override;

	bool showInternal(
			not_null<Window::SectionMemento*>,
			const Window::SectionShow &) override {
		return false;
	}

	QRect floatPlayerAvailableRect() override {
		return QRect(mapToGlobal(QPoint()), size());
	}

	bool floatPlayerHandleWheelEvent(QEvent *) override {
		return false;
	}

	rpl::producer<> removeRequests() const override {
		return _close->clicks() | rpl::to_empty;
	}

protected:
	void resizeEvent(QResizeEvent *) override;
	void showFinishedHook() override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void paintEvent(QPaintEvent *event) override {
		QPainter(this).fillRect(event->rect(), st::windowBg);
	}

private:
	Ui::FlatLabel *_title = nullptr;
	Ui::IconButton *_close = nullptr;
	Ui::FlatLabel *_error = nullptr;
	bool _failed = true;
	bool _loaded = false;
	QTemporaryDir _storage;
	std::string _storageToken = Webview::GenerateStorageToken();
	std::unique_ptr<Webview::Window> _webview;
};

Section::Section(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: SectionWidget(parent, controller)
, _title(Ui::CreateChild<Ui::FlatLabel>(
	this, QString::fromLatin1(kTitle), st::infoTopBarTitle))
, _close(Ui::CreateChild<Ui::IconButton>(this, st::infoTopBarClose))
, _error(Ui::CreateChild<Ui::FlatLabel>(
	this, tr::lng_bot_webview_failed(tr::now), st::defaultFlatLabel)) {
	_close->setAccessibleName(tr::lng_close(tr::now));
	if (!_storage.isValid()) {
		return;
	}
	_webview = std::make_unique<Webview::Window>(this, Webview::WindowConfig{
		.opaqueBg = st::windowBg->c,
		.storageId = { _storage.path(), QByteArray::fromStdString(_storageToken) },
		.safe = true,
	});
	if (_webview->widget()) {
		_failed = false;
		_error->hide();
		_webview->setNavigationPolicyHandler([](QString address, bool popup) {
			const auto url = QUrl(address);
			const auto host = url.host();
			return !popup
				&& url.scheme() == u"https"_q
				&& (host == u"tiktok.com"_q || host.endsWith(u".tiktok.com"_q));
		});
		_webview->setNavigationDoneHandler([=](bool success) {
			if (!_loaded) {
				_loaded = success;
				_failed = !success;
				showFinishedHook();
			}
		});
		_webview->init(QByteArray(kFitPage));
		_webview->navigate(QString::fromLatin1(kUrl));
	}
}

Section::~Section() {
	_webview.reset();
	Webview::ClearStorageDataByToken(_storageToken);
	if (_storage.isValid()
		&& Webview::DeferTemporaryStorageRemoval(_storage.path())) {
		_storage.setAutoRemove(false);
	}
}

void Section::showFinishedHook() {
	_error->setVisible(_failed);
	if (const auto view = _webview ? _webview->widget() : nullptr) {
		const auto visible = isVisible() && !_failed;
		view->setVisible(visible);
		_webview->setVisible(visible);
	}
}

void Section::showEvent(QShowEvent *event) {
	SectionWidget::showEvent(event);
	showFinishedHook();
}

void Section::hideEvent(QHideEvent *event) {
	if (_webview && _webview->valid()) {
		_webview->setVisible(false);
	}
	SectionWidget::hideEvent(event);
}

void Section::resizeEvent(QResizeEvent *) {
	const auto header = _close->height();
	_close->moveToRight(0, 0);
	_title->moveToLeft(
		style::ConvertScale(16), (header - _title->height()) / 2);
	_error->resizeToWidth(std::max(1, width() - style::ConvertScale(32)));
	_error->moveToLeft(
		style::ConvertScale(16), header + style::ConvertScale(12));
	if (const auto view = _webview ? _webview->widget() : nullptr) {
		const auto gutter = style::ConvertScale(10);
		view->setGeometry(
			style::RightToLeft() ? 0 : gutter,
			header,
			std::max(0, width() - gutter),
			std::max(0, height() - header));
	}
}

class Memento final : public Window::SectionMemento {
public:
	bool instant() const override {
		return true;
	}

	object_ptr<Window::SectionWidget> createWidget(
			QWidget *parent,
			not_null<Window::SessionController*> controller,
			Window::Column column,
			const QRect &geometry) override {
		if (column != Window::Column::Third) {
			return nullptr;
		}
		auto result = object_ptr<Section>(parent, controller);
		result->setGeometry(geometry);
		return result;
	}
};

} // namespace

int ColumnWidth(const QWidget &window) {
	const auto stored = window.property(kWidthProperty);
	return stored.isValid() ? stored.toInt() : style::ConvertScale(480);
}

void SetColumnWidth(QWidget &window, int width) {
	window.setProperty(kWidthProperty, std::clamp(
		width, style::ConvertScale(320), style::ConvertScale(760)));
}

Ui::IconButton *MakeButton(QWidget *parent) {
	return Ui::CreateChild<Button>(parent);
}

std::shared_ptr<Window::SectionMemento> MakeSection() {
	return std::make_shared<Memento>();
}

bool IsSection(not_null<Window::SectionMemento*> memento) {
	return dynamic_cast<Memento*>(memento.get()) != nullptr;
}

} // namespace Mokagram::TikTok
