# Project build

For building or running this project's current Release version, follow `.agents/skills/mokagram-release-build/SKILL.md`. Use the existing Release build rather than a prebuilt AppImage when testing current changes.

# Commits

Commit messages follow the repo history: `Область: что сделано` (e.g. `Музыка: панель очереди`, `CI Windows: сборка`), no trailing period, subject ≤ ~72 chars, plus a short body explaining *why* when the change isn't obvious. Hooks live in `.githooks/` (enabled with `git config core.hooksPath .githooks`); `commit-msg` prints bilingual (ru/en) hints and never blocks.

# Commit cadence (for agents)

Commit each individual change per iteration — one logical change per commit, one file at a time when the changes are independent — instead of batching everything into one big commit at the end. After finishing an edit to a file (or a coherent slice of it), stage just that change (`git add <path>`, `git add -p` for partial hunks) and commit it immediately. Never stage unrelated work that is already in the tree (`git add -A` / `git commit -a`). Follow the message format above for every commit.

Русский: каждый отдельный правка-итерация коммитится сразу — один логический шаг на коммит, при независимых правках — по файлу; не сваливать всё в один коммит в конце и не захватывать чужое из рабочего дерева.

# Notes
