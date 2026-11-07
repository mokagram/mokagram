// This file is part of mokaGram, https://github.com/ecronxy/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
// Mokagram: ложные пароли экрана блокировки.
#pragma once

#include "base/basic_types.h"

#include <QtCore/QString>

#include <optional>
#include <vector>

namespace Mokagram::FakePasscode {

constexpr auto kMaxEntries = 8;

struct Actions {
	bool clearCache = false;
	bool clearProxies = false;
	bool deleteAll = false;
	std::vector<int> logoutAccounts; // индексы аккаунтов в Main::Domain
};

struct Entry {
	int id = 0;
	QString name;
	QString passcode;
	bool unlocks = true; // открывает клиент и тихо делает действия
	Actions actions;
};

struct Match {
	int id = 0;
	QByteArray realPasscode; // пусто, если пароль не открывает клиент
	Actions actions;

	[[nodiscard]] bool unlocks() const {
		return !realPasscode.isEmpty();
	}
};

enum class AddResult : uchar {
	Ok,
	Empty,
	TooMany,
	WrongPasscode,
	SameAsReal,
	Duplicate,
	Busy,
	Failed,
};

// Проверка с той же ценой, что и настоящий пароль. Ответ на главном потоке.
void MatchAsync(
	const QByteArray &passcode,
	Fn<void(std::optional<Match>)> done);

// Ложный пароль, который открыл клиент: действия после разблокировки.
void SetPending(int id);
void RunPending();
// Ложный пароль, который не открывает: действия сразу, клиент остаётся
// заблокированным.
void RunSilent(const Actions &actions);

[[nodiscard]] std::vector<Entry> List();
void Add(
	Entry entry,
	const QString &realPasscode,
	Fn<void(AddResult)> done);
void Update(int id, QString name, Actions actions);
void Remove(int id);

// Настоящий пароль сменили или сняли (пустой).
void RealPasscodeChanged(const QByteArray &passcode);

void RegisterSettings();

} // namespace Mokagram::FakePasscode
