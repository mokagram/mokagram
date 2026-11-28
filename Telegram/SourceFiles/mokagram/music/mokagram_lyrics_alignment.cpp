// This file is part of mokaGram, https://github.com/mokagram/mokagram
// (c) 2026 @ecronx (https://t.me/ecronx). GPLv3 with the attribution term
// of MOKAGRAM_NOTICE.md (section 7(b)): keep this notice when copying.
#include "mokagram/music/mokagram_lyrics_alignment.h"

#include "mokagram/asr/mokagram_asr.h"

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace Mokagram::Music {
namespace {

constexpr auto kMaxTokens = 4096;
constexpr auto kMaxGapLines = 2;
constexpr auto kMaxGapMs = 12000;

struct Token {
	int id;
	int from;
	int to;
	int weight;
};

struct HeardToken {
	int id;
	int word;
};

struct Row {
	int line;
	int begin;
	int end;
	qint64 startMs = -1;
	qint64 endMs = 0;
};

[[nodiscard]] const QRegularExpression &WordPattern() {
	static const auto result = QRegularExpression(
		uR"([\p{L}\p{N}][\p{L}\p{N}\p{M}]*)"_q);
	return result;
}

[[nodiscard]] QString Normalize(QString text) {
	return text.normalized(QString::NormalizationForm_KC)
		.toCaseFolded()
		.replace(u'ё', u'е');
}

// Weighted longest common subsequence. Among equally good paths, prefer
// similar relative positions so a repeated chorus doesn't all match one verse.
[[nodiscard]] std::vector<int> FindMatches(
		std::span<const Token> source,
		std::span<const HeardToken> heard) {
	enum class Step : uint8 { Up, Left, Match };
	const auto n = int(source.size());
	const auto m = int(heard.size());
	constexpr auto kWeightUnit = int64(kMaxTokens) * kMaxTokens * kMaxTokens + 1;
	auto path = std::make_unique_for_overwrite<Step[]>(size_t(n) * m);
	auto scores = std::vector<int64>(m + 1);
	for (auto i = 0; i != n; ++i) {
		auto diagonal = int64(0);
		auto steps = path.get() + size_t(i) * m;
		for (auto j = 0; j != m; ++j) {
			const auto above = scores[j + 1];
			const auto left = scores[j];
			auto best = std::max(above, left);
			auto step = (above >= left) ? Step::Up : Step::Left;
			if (source[i].id == heard[j].id) {
				const auto distance = std::abs(i * m - j * n);
				const auto matched = diagonal + source[i].weight * kWeightUnit - distance;
				if (matched >= best) {
					best = matched;
					step = Step::Match;
				}
			}
			scores[j + 1] = best;
			steps[j] = step;
			diagonal = above;
		}
	}
	auto matches = std::vector<int>(n, -1);
	for (auto i = n, j = m; i && j;) {
		switch (path[size_t(i - 1) * m + j - 1]) {
		case Step::Up: --i; break;
		case Step::Left: --j; break;
		case Step::Match: matches[--i] = heard[--j].word; break;
		}
	}
	return matches;
}

[[nodiscard]] bool FillLineGaps(std::vector<Row> &rows) {
	if (rows.front().startMs < 0 || rows.back().startMs < 0) {
		return false;
	}
	for (auto left = 0, right = 1; right < int(rows.size()); ++right) {
		if (rows[right].startMs < 0) {
			continue;
		}
		if (rows[right].startMs <= rows[left].startMs) {
			return false;
		}
		const auto missing = right - left - 1;
		if (missing) {
			const auto from = rows[left].endMs;
			const auto gap = rows[right].startMs - from;
			if (missing > kMaxGapLines || gap < 450 * missing || gap > kMaxGapMs) {
				return false;
			}
			for (auto i = 1; i <= missing; ++i) {
				rows[left + i].startMs = from + gap * i / (missing + 1);
			}
		}
		left = right;
	}
	return true;
}

} // namespace

Lyrics AlignLyrics(Lyrics source, std::span<const Asr::TimedWord> heard) {
	if (!source.found || source.synced || heard.empty() || heard.size() > kMaxTokens) {
		return source;
	}
	auto vocabulary = QHash<QString, int>();
	auto tokens = std::vector<Token>();
	auto rows = std::vector<Row>();
	for (auto i = 0; i < int(source.lines.size()); ++i) {
		const auto &text = source.lines[i].text;
		const auto label = QStringView(text).trimmed();
		if (label.startsWith(u'[') && label.endsWith(u']')) {
			continue;
		}
		const auto begin = int(tokens.size());
		auto words = WordPattern().globalMatch(text);
		while (words.hasNext()) {
			if (tokens.size() == kMaxTokens) {
				return source;
			}
			const auto word = words.next();
			auto &id = vocabulary[Normalize(word.captured())];
			if (!id) {
				id = int(vocabulary.size());
			}
			const auto weight = word.capturedLength() >= 4 ? 3 : 1;
			tokens.push_back({ id, int(word.capturedStart()), int(word.capturedEnd()), weight });
		}
		if (begin != int(tokens.size())) {
			rows.push_back({ i, begin, int(tokens.size()) });
		}
	}
	if (rows.empty()) {
		return source;
	}
	auto heardTokens = std::vector<HeardToken>();
	heardTokens.reserve(heard.size());
	for (auto i = 0; i < int(heard.size()); ++i) {
		auto words = WordPattern().globalMatch(heard[i].text);
		while (words.hasNext()) {
			if (heardTokens.size() == kMaxTokens) {
				return source;
			}
			heardTokens.push_back({ vocabulary.value(Normalize(words.next().captured())), i });
		}
	}
	if (heardTokens.empty()) {
		return source;
	}
	const auto matches = FindMatches(tokens, heardTokens);
	const auto matched = std::count_if(matches.begin(), matches.end(), [](int i) { return i >= 0; });
	if ((matched < 4 && matched != int(tokens.size()))
		|| matched * 2 < int(tokens.size()) || matched * 3 < int(heardTokens.size())) {
		return source;
	}
	for (auto &row : rows) {
		auto first = -1;
		auto last = -1;
		auto weight = 0;
		for (auto i = row.begin; i != row.end; ++i) {
			if (matches[i] >= 0) {
				first = (first < 0) ? i : first;
				last = i;
				weight += tokens[i].weight;
			}
		}
		if (weight < 2 && (first < 0 || row.end - row.begin != 1)) {
			continue;
		}
		const auto lead = std::min(600, (first - row.begin) * 200);
		row.startMs = std::max<qint64>(0, heard[matches[first]].startMs - lead);
		row.endMs = heard[matches[last]].endMs;
	}
	if (!FillLineGaps(rows)) {
		return source;
	}
	// Keep exact word timing only on fully matched lines. Interpolated or
	// partially matched lines use the player's existing line highlight.
	auto next = int(source.lines.size());
	auto timestamp = std::max(rows.back().startMs + 1, rows.back().endMs);
	for (auto row = rows.rbegin(); row != rows.rend(); ++row) {
		while (--next > row->line) {
			source.lines[next].timeMs = timestamp;
		}
		auto &line = source.lines[row->line];
		line.timeMs = timestamp = row->startMs;
		if (std::find(matches.begin() + row->begin, matches.begin() + row->end, -1)
				!= matches.begin() + row->end) {
			continue;
		}
		line.words.reserve(row->end - row->begin);
		for (auto i = row->begin; i != row->end; ++i) {
			const auto &word = heard[matches[i]];
			if (i != row->begin && matches[i] == matches[i - 1]) {
				line.words.back().to = tokens[i].to;
			} else {
				line.words.push_back({ word.startMs, word.endMs, tokens[i].from, tokens[i].to });
			}
		}
	}
	while (--next >= 0) {
		source.lines[next].timeMs = timestamp;
	}
	source.synced = true;
	return source;
}

} // namespace Mokagram::Music
