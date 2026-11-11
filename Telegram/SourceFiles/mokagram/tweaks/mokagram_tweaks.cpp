// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: мелкие настройки.
#include "mokagram/tweaks/mokagram_tweaks.h"

#include "mokagram/lang/mokagram_lang.h"
#include "mokagram/settings/mokagram_settings.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "main/main_session.h"

#include <QtCore/QStringList>
#include <rpl/filter.h>

namespace Mokagram::Tweaks {
namespace {

constexpr auto kPinsKey = "chats.local_pins";

constexpr uchar kMarkerSOI = 0xD8;
constexpr uchar kMarkerSOS = 0xDA;
constexpr uchar kMarkerAPP1 = 0xE1;
constexpr uchar kMarkerAPP13 = 0xED;
constexpr uchar kMarkerCOM = 0xFE;

[[nodiscard]] QString U(const char *text) {
	return QString::fromUtf8(Mokagram::Lang::Tr(text));
}

struct PinCache {
	std::vector<uint64> ids;
	bool loaded = false;
	rpl::lifetime lifetime;
};

[[nodiscard]] QStringList PinIds() {
	return Settings::String(QString::fromLatin1(kPinsKey)).split(
		',',
		Qt::SkipEmptyParts);
}

[[nodiscard]] PinCache &Pins() {
	static auto result = PinCache();
	if (!result.loaded) {
		result.loaded = true;
		const auto refresh = [] {
			auto &p = Pins();
			p.ids.clear();
			for (const auto &id : PinIds()) {
				if (const auto val = id.toULongLong()) {
					p.ids.push_back(val);
				}
			}
		};
		refresh();
		Settings::Changed() | rpl::filter([](const QString &key) {
			return key == QString::fromLatin1(kPinsKey);
		}) | rpl::on_next(refresh, result.lifetime);
	}
	return result;
}

void WritePinIds(const QStringList &ids) {
	Settings::SetString(QString::fromLatin1(kPinsKey), ids.join(','));
}

// Порядок в списке чатов пересчитывается для тех, кого задело.
void Resort(not_null<Main::Session*> session, const QStringList &ids) {
	for (const auto &id : ids) {
		if (const auto peer = session->data().peerLoaded(
				PeerId(id.toULongLong()))) {
			if (const auto history = session->data().historyLoaded(peer)) {
				history->updateChatListSortPosition();
			}
		}
	}
}

// Ориентация кадра из EXIF: 1 значит как есть. 0 если EXIF не разобрать.
[[nodiscard]] int ExifOrientation(const QByteArray &segment) {
	// "Exif\0\0" + TIFF: порядок байт, 0x002A, смещение первой таблицы.
	const auto data = reinterpret_cast<const uchar*>(segment.constData());
	const auto size = int(segment.size());
	if (size < 14 || memcmp(data, "Exif\0\0", 6) != 0) {
		return 0;
	}
	const auto tiff = data + 6;
	const auto little = (tiff[0] == 'I' && tiff[1] == 'I');
	if (!little && !(tiff[0] == 'M' && tiff[1] == 'M')) {
		return 0;
	}
	const auto read16 = [&](int at) -> int {
		return (at + 2 > size - 6)
			? -1
			: little
			? (tiff[at] | (tiff[at + 1] << 8))
			: ((tiff[at] << 8) | tiff[at + 1]);
	};
	const auto read32 = [&](int at) -> int {
		return (at + 4 > size - 6)
			? -1
			: little
			? (tiff[at] | (tiff[at + 1] << 8)
				| (tiff[at + 2] << 16) | (tiff[at + 3] << 24))
			: ((tiff[at] << 24) | (tiff[at + 1] << 16)
				| (tiff[at + 2] << 8) | tiff[at + 3]);
	};
	const auto first = read32(4);
	const auto count = (first >= 8) ? read16(first) : -1;
	if (count <= 0) {
		return 0;
	}
	for (auto i = 0; i != count; ++i) {
		const auto entry = first + 2 + i * 12;
		if (read16(entry) == 0x0112) {
			const auto value = read16(entry + 8);
			return (value >= 1 && value <= 8) ? value : 0;
		}
	}
	return 0;
}

// Минимальный EXIF только с ориентацией, чтобы кадр не повернулся.
[[nodiscard]] QByteArray OrientationOnly(int orientation) {
	auto tiff = QByteArray();
	tiff.append("MM", 2);
	tiff.append(char(0)).append(char(0x2A));
	tiff.append(char(0)).append(char(0)).append(char(0)).append(char(8));
	tiff.append(char(0)).append(char(1)); // одна запись
	tiff.append(char(0x01)).append(char(0x12)); // тег Orientation
	tiff.append(char(0)).append(char(3)); // тип SHORT
	tiff.append(char(0)).append(char(0)).append(char(0)).append(char(1));
	tiff.append(char(0)).append(char(orientation));
	tiff.append(char(0)).append(char(0));
	tiff.append(char(0)).append(char(0)).append(char(0)).append(char(0));
	auto segment = QByteArray("Exif\0\0", 6) + tiff;
	const auto length = int(segment.size()) + 2;
	auto result = QByteArray();
	result.append(char(0xFF)).append(char(kMarkerAPP1));
	result.append(char((length >> 8) & 0xFF)).append(char(length & 0xFF));
	result.append(segment);
	return result;
}

[[nodiscard]] QByteArray StripJpeg(const QByteArray &bytes) {
	const auto data = reinterpret_cast<const uchar*>(bytes.constData());
	const auto size = int(bytes.size());
	if (size < 4 || data[0] != 0xFF || data[1] != kMarkerSOI) {
		return bytes;
	}
	auto result = QByteArray();
	result.reserve(size);
	result.append(bytes.constData(), 2);
	auto orientation = 0;
	auto at = 2;
	while (at + 4 <= size) {
		if (data[at] != 0xFF) {
			return bytes;
		}
		const auto marker = data[at + 1];
		if (marker == 0xFF) {
			++at;
			continue;
		} else if (marker == kMarkerSOS) {
			break;
		}
		const auto length = (data[at + 2] << 8) | data[at + 3];
		if (length < 2 || at + 2 + length > size) {
			return bytes;
		}
		const auto segment = bytes.mid(at + 4, length - 2);
		if (marker == kMarkerAPP1) {
			// EXIF и XMP: ориентацию запоминаем, остальное не берём.
			if (const auto found = ExifOrientation(segment)) {
				orientation = found;
			}
		} else if (marker != kMarkerAPP13 && marker != kMarkerCOM) {
			// ICC-профиль и разметка кодека остаются: без них цвета уедут.
			result.append(bytes.constData() + at, 2 + length);
		}
		at += 2 + length;
	}
	if (orientation > 1) {
		result.insert(2, OrientationOnly(orientation));
	}
	result.append(bytes.constData() + at, size - at);
	return result;
}

[[nodiscard]] QByteArray StripPng(const QByteArray &bytes) {
	static const auto signature = QByteArray("\x89PNG\r\n\x1A\n", 8);
	if (!bytes.startsWith(signature)) {
		return bytes;
	}
	const auto data = reinterpret_cast<const uchar*>(bytes.constData());
	const auto size = int(bytes.size());
	auto result = signature;
	result.reserve(size);
	auto at = 8;
	while (at + 12 <= size) {
		const auto length = (data[at] << 24)
			| (data[at + 1] << 16)
			| (data[at + 2] << 8)
			| data[at + 3];
		if (length < 0 || at + 12 + length > size) {
			return bytes;
		}
		const auto type = bytes.mid(at + 4, 4);
		const auto drop = (type == "eXIf")
			|| (type == "tEXt")
			|| (type == "zTXt")
			|| (type == "iTXt")
			|| (type == "tIME");
		if (!drop) {
			result.append(bytes.constData() + at, 12 + length);
		}
		at += 12 + length;
	}
	if (at != size) {
		return bytes;
	}
	return result;
}

} // namespace

bool HideStories() {
	return Settings::Bool(u"look.hide_stories"_q);
}

bool RevealSpoilers() {
	return Settings::Bool(u"chats.reveal_spoilers"_q);
}

bool StripsMetadata() {
	return Settings::Bool(u"privacy.strip_metadata"_q);
}

int LocalPinIndex(not_null<PeerData*> peer) {
	const auto &ids = Pins().ids;
	const auto target = peer->id.value;
	for (auto i = 0, count = int(ids.size()); i != count; ++i) {
		if (ids[i] == target) {
			return 10 + i;
		}
	}
	return 0;
}

bool IsLocallyPinned(not_null<PeerData*> peer) {
	return LocalPinIndex(peer) != 0;
}

void ToggleLocalPin(not_null<PeerData*> peer) {
	auto ids = PinIds();
	const auto before = ids;
	const auto id = QString::number(peer->id.value);
	if (!ids.removeOne(id)) {
		// Новый закреп ложится наверх.
		ids.push_front(id);
	}
	WritePinIds(ids);
	auto touched = before;
	touched.append(ids);
	touched.removeDuplicates();
	Resort(&peer->session(), touched);
}

QByteArray StripMetadata(const QByteArray &bytes) {
	if (bytes.size() < 8) {
		return bytes;
	} else if (static_cast<uchar>(bytes[0]) == 0xFF) {
		return StripJpeg(bytes);
	}
	return StripPng(bytes);
}

void RegisterSettings() {
	Settings::Register({
		.key = u"look.hide_stories"_q,
		.section = U("Список чатов"),
		.title = U("Спрятать полосу историй"),
		.description = U("Над списком чатов не будет круглых аватарок с историями."),
		.kind = Settings::Kind::Bool,
		.defaultValue = false,
		.category = u"look"_q,
		.sectionIcon = u"chats"_q,
		.icon = u"hide"_q,
	});
	Settings::Register({
		.key = u"chats.reveal_spoilers"_q,
		.section = U("Сообщения"),
		.title = U("Раскрывать спойлеры сразу"),
		.description = U("Текст под спойлером и размытые фото читаются без нажатия. Подействует на сообщения, которые откроются заново."),
		.kind = Settings::Kind::Bool,
		.defaultValue = false,
		.category = u"chats"_q,
		.sectionIcon = u"chats"_q,
		.icon = u"hide"_q,
	});
	Settings::Register({
		.key = u"privacy.strip_metadata"_q,
		.section = U("Отправка"),
		.title = U("Чистить метаданные у фото"),
		.description = U("Из JPEG и PNG убираются геотег, модель камеры, автор и время съёмки. Поворот кадра остаётся."),
		.kind = Settings::Kind::Bool,
		.defaultValue = false,
		.category = u"privacy"_q,
		.sectionIcon = u"hide"_q,
		.icon = u"hide"_q,
	});
	Settings::Register({
		.key = QString::fromLatin1(kPinsKey),
		.section = U("Список чатов"),
		.title = U("Свои закрепы"),
		.kind = Settings::Kind::String,
		.defaultValue = QString(),
		.category = u"look"_q,
		.hidden = true,
	});
}

} // namespace Mokagram::Tweaks
