/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/profile/info_profile_pills.h"

#include "data/data_saved_music.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history_item.h"
#include "info/profile/info_profile_balance_pill.h"
#include "info/profile/info_profile_music_button.h"
#include "info/profile/info_profile_top_bar.h"
#include "ui/rect.h"
#include "wallet/wallet_card_view.h"

#include "styles/style_info.h"
#include "styles/style_info_profile_top_bar.h"

namespace Info::Profile {
namespace {

constexpr auto kFadeStart = 0.6;

// Mokagram: keep the saved music message context for native playback.
[[nodiscard]] rpl::producer<AudioMsgId> ProfileMusicValue(
		not_null<UserData*> user) {
	const auto savedMusic = &user->owner().savedMusic();
	const auto peerId = user->id;
	return rpl::single(
		peerId
	) | rpl::then(
		savedMusic->changed() | rpl::filter(rpl::mappers::_1 == peerId)
	) | rpl::map([=] {
		const auto &list = savedMusic->list(peerId);
		if (!list.empty()) {
			if (const auto media = list.front()->media()) {
				if (const auto document = media->document()) {
					return AudioMsgId(document, list.front()->fullId());
				}
			}
		}
		return AudioMsgId();
	}) | rpl::distinct_until_changed();
}

} // namespace

PillsRow::PillsRow(
	not_null<Ui::RpWidget*> parent,
	not_null<Window::SessionController*> controller,
	not_null<UserData*> user,
	rpl::producer<std::optional<QColor>> edgeColor,
	Fn<void()> showSavedMusic,
	bool underButtons)
: _parent(parent)
, _balance(std::make_unique<BalancePill>(
	parent,
	user,
	rpl::duplicate(edgeColor)))
, _music(std::make_unique<MusicButton>(parent)) {
	const auto balance = _balance.get();
	balance->setClickedCallback([=] {
		if (balance->actionable()) {
			Wallet::ShowCardView(controller, user, balance);
		}
	});
	const auto music = _music.get();
	music->setClickedCallback([=] {
		if (music->actionable() && showSavedMusic) {
			showSavedMusic();
		}
	});

	std::move(
		edgeColor
	) | rpl::on_next([=](std::optional<QColor> color) {
		music->setOverrideBg(color);
	}, _lifetime);

	ProfileMusicValue(
		user
	) | rpl::on_next([=](const AudioMsgId &track) {
		music->setTrack(track);
	}, _lifetime);

	_reserve = rpl::combine(
		balance->shownValue(),
		music->shownValue()
	) | rpl::map([=](bool hasBalance, bool hasMusic) {
		if (!hasBalance && !hasMusic) {
			return 0;
		}
		const auto balanceHeight = st::infoProfileBalancePillHeight;
		const auto rowsHeight = (hasBalance ? balanceHeight : 0)
			+ (hasMusic ? MusicButton::RowHeight() : 0)
			+ ((hasBalance && hasMusic) ? st::infoProfileTopBarPillsGap : 0);
		// Without action buttons one balance-sized row already fits.
		return rowsHeight - balanceHeight
			+ (underButtons ? st::infoProfileTopBarPillsReserve : 0);
	});

	rpl::merge(
		balance->naturalWidthValue() | rpl::to_empty,
		balance->shownValue() | rpl::to_empty,
		music->shownValue() | rpl::to_empty
	) | rpl::on_next([=] {
		layout();
	}, _lifetime);
}

PillsRow::~PillsRow() = default;

rpl::producer<int> PillsRow::reserveValue() const {
	return _reserve.value();
}

void PillsRow::place(int top, float64 progress) {
	const auto opacity = std::clamp(
		(progress - kFadeStart) / (1. - kFadeStart),
		0.,
		1.);
	_balance->setOpacity(opacity);
	_music->setOpacity(opacity);
	_top = top;
	if (_outerWidth != _parent->width()) {
		layout();
	} else {
		moveRows();
	}
}

rpl::lifetime &PillsRow::lifetime() {
	return _lifetime;
}

void PillsRow::layout() {
	_outerWidth = _parent->width();
	const auto &padding = st::infoProfileTopBarActionButtonsPadding;
	const auto available = _outerWidth - rect::m::sum::h(padding);
	if (available <= 0) {
		return;
	}
	if (!_balance->isHidden()) {
		_balance->resizeToNaturalWidth(available);
		const auto fitted = _balance->fittedWidth();
		if (fitted < _balance->width()) {
			_balance->resizeToWidth(fitted);
		}
	}
	if (!_music->isHidden()) {
		_music->resizeToWidth(available);
	}
	moveRows();
}

void PillsRow::moveRows() {
	const auto &padding = st::infoProfileTopBarActionButtonsPadding;
	const auto hasMusic = !_music->isHidden();
	if (hasMusic) {
		_music->moveToLeft(padding.left(), _top);
	}
	if (!_balance->isHidden()) {
		const auto left = hasMusic
			? _outerWidth - padding.right() - _balance->width()
			: (_outerWidth - _balance->width()) / 2;
		const auto top = _top + (hasMusic
			? MusicButton::RowHeight() + st::infoProfileTopBarPillsGap
			: 0);
		_balance->moveToLeft(left, top);
	}
}

void TopBar::setupPills(
		not_null<Window::SessionController*> controller,
		Fn<void()> showSavedMusic) {
	const auto user = _peer->asUser();
	if (!user
		|| _savedMessages
		|| _topic
		|| !(_source == Source::Profile
			|| (_source == Source::Stories && user->isSelf()))) {
		return;
	}
	_pills = std::make_unique<PillsRow>(
		this,
		controller,
		user,
		_edgeColor.value(),
		std::move(showSavedMusic),
		_hasActions);
	_pills->reserveValue(
	) | rpl::on_next([=](int reserve) {
		applyPillsReserve(reserve);
	}, _pills->lifetime());
}

void TopBar::applyPillsReserve(int reserve) {
	const auto applied = _pillsReserve.current();
	if (applied == reserve) {
		return;
	}
	const auto base = QWidget::maximumHeight() - applied;
	QWidget::setMaximumHeight(base + reserve);
	_cachedGradient = QImage();
	_cachedClipPath = QPainterPath();
	_pillsReserve = reserve;
	updateLabelsPosition();
	update();
}

rpl::producer<int> TopBar::desiredHeightValue() const {
	return _pillsReserve.value() | rpl::map([=] {
		return QWidget::maximumHeight();
	});
}

} // namespace Info::Profile
