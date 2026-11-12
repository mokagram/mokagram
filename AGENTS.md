# mokaGram

Десктопный клиент Telegram: форк tdesktop (C++/Qt) с системой Python-плагинов. Лицензия GPL-3 с условием атрибуции из `MOKAGRAM_NOTICE.md`. Наш код лежит в `Telegram/SourceFiles/mokagram/`, правки в коде tdesktop держать минимальными и помечать комментарием `// Mokagram:`.

Внешний вид и сборка экранов: [`design.md`](design.md). Его читать до любой визуальной правки.

# Project build

- macOS: `.agents/skills/mokagram-build/SKILL.md` (только Release, установка в /Applications).
- Linux: существующая Release-сборка через ninja в отдельной папке сборки, пересобирать только изменённые цели. Готовый AppImage для проверки текущих правок не годится.
- Поменял заголовок с классом или структурой: пересобрать все файлы, которые его подключают (`ninja -t deps`), иначе размер класса разойдётся и клиент падает.
- Правка style-констант (`*.style`) пересобирает почти всё: все такие правки делать одним заходом.
- `Telegram/lib_ui` это сабмодуль на форк `mokagram/lib_ui`: правки коммитить в форк, пушить и поднимать указатель сабмодуля здесь. `patches/lib_ui.patch` в релизе больше не накладывается.
- Новый `.cpp`/`.h` добавлять в `Telegram/CMakeLists.txt`.

# Запуск и проверка

- Закрывать клиент штатно (`Telegram -quit`) и ждать выхода. `pkill` теряет настройки.
- Отладочные команды включаются переменной `MOKAGRAM_DEBUG_CMD=1`, команда пишется в файл `mokagram_cmd` в `$XDG_RUNTIME_DIR` (`settings`, `close`, `customize`, `pluginsbox`, `sclogin`, `click:x,y` и др., полный список в `settings/mokagram_settings_box.cpp`). Клики попадают в живой клиент: по одному, после каждого скриншот.
- Тестовые сообщения только в тестовой группе mokaChat. В чужих чатах и чатах владельца ничего не писать и не удалять.
- Интерфейс пользователь проверяет вручную. Агент не запускает визуальные проверки, не меняет масштаб и не делает скриншоты без отдельного запроса пользователя.

# Код

- Любой пиксель через `style::ConvertScale(n)`.
- Строки интерфейса через `Mokagram::Lang::Tr("...")` и английский перевод в `mokagram/lang/mokagram_lang_table.inc`.
- Значки только из пака iOSTelegram, логотипы сервисов настоящие (см. `design.md`). Свои не рисовать.
- Личные абсолютные пути (`/home/...`, `/Users/...`) в код не писать.
- Длинных тире нет ни в коде, ни в комментариях, ни в коммитах, ни в текстах.
- Бот и бэкенд клиента живут отдельно; новая серверная функция клиента = новая ветка inline-запроса бота, клиент ходит через `messages.getInlineBotResults`.

# Безопасность и данные

- Ключи, `api_hash`, токены и содержимое сессий не копировать в код, коммиты и логи. Токены пользователя (например, SoundCloud) хранятся только локально и не печатаются.
- Файлы сессий не открывать параллельно с запущенным клиентом и не использовать с другого IP (AUTH_KEY_DUPLICATED, FLOOD_WAIT). Вход в аккаунты (номер, код, пароль) делает только человек.
- Сеть, порты, прокси и системные настройки не трогать.

# Ветки и пуш

- Рабочая ветка `mokagram`, remote `origin` = github.com/mokagram/mokagram.
- После каждого коммита сразу `git push origin mokagram:mokagram`. Без `--force`.
- Перед работой `git fetch` и фаст-форвард: второй разработчик пушит в ту же ветку.

# Commits

Commit messages follow the repo history: `Область: что сделано` (e.g. `Музыка: панель очереди`, `CI Windows: сборка`), no trailing period, subject ≤ ~72 chars, plus a short body explaining *why* when the change isn't obvious. Hooks live in `.githooks/` (enabled with `git config core.hooksPath .githooks`); `commit-msg` prints bilingual (ru/en) hints and never blocks.

# Commit cadence (for agents)

Commit each individual change per iteration: one logical change per commit, one file at a time when the changes are independent, instead of batching everything into one big commit at the end. After finishing an edit to a file (or a coherent slice of it), stage just that change (`git add <path>`, `git add -p` for partial hunks) and commit it immediately. Never stage unrelated work that is already in the tree (`git add -A` / `git commit -a`). Follow the message format above for every commit.

Русский: каждая отдельная правка-итерация коммитится сразу, один логический шаг на коммит, при независимых правках по файлу; не сваливать всё в один коммит в конце и не захватывать чужое из рабочего дерева.
