// This file is part of mokaGram. See LICENSE and MOKAGRAM_NOTICE.md.
#pragma once

#include "base/basic_types.h"

class QByteArray;
class QImage;
class QWidget;

namespace Mokagram {

void RecordProfileGif(
	not_null<QWidget*> guard,
	Fn<bool(QImage&)> render,
	bool animated,
	Fn<void(QByteArray)> done);
[[nodiscard]] bool CopyProfileGif(const QByteArray &gif);

} // namespace Mokagram
