/*
This file is part of mokaGram, a Telegram Desktop fork.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class PeerData;

namespace Mokagram::BlackHole {

// Один кусок ленты: снимок и его место внутри области эффекта.
struct Item {
	QImage image;
	QRectF rect;
};

struct Args {
	not_null<QWidget*> parent;
	// Область эффекта в координатах parent.
	QRect area;
	// Обои ровно под область, без сообщений.
	QImage wallpaper;
	// Шапка чата: гаснет, пока дыра открыта.
	QImage chrome;
	QRect chromeRect;
	// Островок, из которого растёт дыра (координаты области).
	QRectF island;
	QImage islandImage;
	QRect islandImageRect;
	// Островок остаётся после эффекта (играет музыка).
	bool islandStays = false;
	std::vector<Item> items;
};

// Режет снимок ленты на куски по пустым строкам, origin: куда в
// области попадает левый верхний угол снимка.
[[nodiscard]] std::vector<Item> Split(const QImage &shot, QPointF origin);

[[nodiscard]] bool Supported();
[[nodiscard]] bool Running();
void Start(Args &&args);

// Очистка истории с чёрной дырой, если чат открыт в окне.
// Сама вызывает clear(); false, если эффект не запустился.
bool ClearHistory(not_null<PeerData*> peer, Fn<void()> clear);

} // namespace Mokagram::BlackHole
