<p align="center">
  <img src="docs/readme/logo.png" alt="mokaGram" width="120">
</p>

<h1 align="center">mokaGram</h1>

<p align="center">
  Клиент на основе Telegram Desktop с переработанным дизайном и новыми функциями.
</p>

<p align="center">
  <a href="https://github.com/mokagram/mokagram/releases"><img src="https://img.shields.io/github/v/release/mokagram/mokagram?include_prereleases&style=for-the-badge&label=релиз&color=0a84ff&labelColor=282828" alt="Релиз"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/лицензия-GPLv3-8e8e93?style=for-the-badge&labelColor=282828" alt="GPLv3"></a>
  <a href="https://t.me/ecronx"><img src="https://img.shields.io/badge/автор-@ecronx-8e8e93?style=for-the-badge&labelColor=282828&logo=telegram&logoColor=white" alt="@ecronx"></a>
</p>

<p align="center">
  <img src="docs/readme/chat-full.webp" alt="mokaGram: окно чата" width="86%">
</p>

## О проекте

**mokaGram** основан на Telegram Desktop и работает с обычным аккаунтом Telegram. Интерфейс оформлен в стиле Material Design 3. Дополнительные функции включаются по отдельности.

[Скачать для macOS, Windows или Linux](#скачать) · [Обзор функций со скриншотами](docs/FEATURES.md)

## Что умеет

- **Приватность**: режим стримера прячет номер, юзернейм и ID, режим призрака не отмечает прочитанным и скрывает, что ты в сети
- **История сообщений**: клиент может сохранять удалённые сообщения и предыдущие версии отредактированных сообщений. Срок хранения копий можно ограничить
- **Безопасность**: фильтр сообщений от незнакомых пользователей, блокировка за спам стикерами и предупреждения о мошенничестве
- **Музыка**: SoundCloud и треки из Telegram, плейлисты, синхронизация лайков, текст песни и комментарии с таймкодами
- **Плеер**: обычная панель или Dynamic Island, на macOS отдельный островок у верхнего края экрана
- **Быстрые команды**
- **Система**: анти-лаг и экономия в фоне
- **Кастомизация** интерфейса
- **Инструменты**: карточку сообщения можно сохранить в PNG, а для анимированных эмодзи и подарков выбрать GIF
- **Плагины** в формате `.plugin`, Python runtime и магазин прямо в клиенте

Для воспроизведения недоступных треков SoundCloud из YouTube Music нужен актуальный локальный `yt-dlp`. Скачать его и проверить требования к среде JavaScript можно на [странице yt-dlp](https://github.com/yt-dlp/yt-dlp#dependencies).

Чтобы включить нужные функции, откройте **Настройки → Настройки mokaGram**.

## Скачать

| Система | Версия | Файл |
|---|---|---|
| **macOS Apple Silicon (arm64)** | v0.1.12 | [DMG](https://github.com/mokagram/mokagram/releases/download/v0.1.12/mokagram-macos-arm64.dmg) |
| **Windows x64** | v0.1.14 | [ZIP](https://github.com/mokagram/mokagram/releases/download/v0.1.14/mokaGram-v0.1.14-windows-x64.zip) · [EXE](https://github.com/mokagram/mokagram/releases/download/v0.1.14/Telegram.exe) |
| **Linux x86_64** | v0.1.15 | [AppImage](https://github.com/mokagram/mokagram/releases/download/v0.1.15/mokagram-x86_64.AppImage) · [.rpm](https://github.com/mokagram/mokagram/releases/download/v0.1.15/mokagram-0.1.15-1.x86_64.rpm) · [tar.xz](https://github.com/mokagram/mokagram/releases/download/v0.1.15/mokagram-linux-x64.tar.xz) |

Для Linux также доступны пакеты из предыдущих выпусков: [.deb из v0.1.11](https://github.com/mokagram/mokagram/releases/download/v0.1.11/mokagram_0.1.11_amd64.deb) и [пакет для pacman из v0.1.10](https://github.com/mokagram/mokagram/releases/download/v0.1.10/mokagram-0.1.10-1-x86_64.pkg.tar.zst).

**macOS:** откройте DMG и перетащите **mokaGram** в **Applications**.

Сборки macOS и Windows без подписи разработчика, система может предупредить при первом запуске.

**Linux:** для Python-плагинов установите `python3`, для их изоляции также нужен `bubblewrap` (`bwrap`).

**AppImage:** разрешите выполнение файла и запустите его. **tar.xz:** распакуйте и запустите `mokaGram/Telegram`. Пакеты deb, rpm и pacman устанавливаются штатным менеджером пакетов; команда запуска: `mokagram`.

[Все релизы и списки изменений](https://github.com/mokagram/mokagram/releases).

## Плагины

Плагин: один файл `.plugin` с классом, наследующим `BasePlugin`. Метаданные, права и команды лежат в самом файле, лимит 300 действий в минуту, каждое действие пишется в журнал. Плагины выполняются на Python runtime и работают с клиентом через API mokaGram.

Полное описание: [docs-plugins-api.md](docs-plugins-api.md), пример: [examples/showcase.plugin](examples/showcase.plugin).

## Сборка

mokaGram собирается так же, как Telegram Desktop:

- [macOS](docs/building-mac.md)
- [Windows](docs/building-win.md)
- [GNU/Linux (Docker)](docs/building-linux.md)

Устройство репозитория: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Исходники Telegram Desktop и список сторонних библиотек: [telegramdesktop/tdesktop](https://github.com/telegramdesktop/tdesktop).

## Авторство и лицензия

> **Основано на mokaGram, автор [@ecronx](https://t.me/ecronx) (https://t.me/ecronx). https://github.com/mokagram/mokagram**

mokaGram основан на [Telegram Desktop](https://github.com/telegramdesktop/tdesktop) (© The Telegram Desktop Authors, GPLv3 с исключением OpenSSL). Доработки © 2026 [@ecronx](https://t.me/ecronx), лицензия та же, GPLv3, плюс условие об сохранении авторства.

Кто берёт исходники или сборки mokaGram, обязан сохранить [уведомление](MOKAGRAM_NOTICE.md) и указать авторство в README и в окне «О программе». При нарушении лицензия прекращается.

*Anyone using mokaGram source code **must credit the author** ([@ecronx](https://t.me/ecronx), https://github.com/mokagram/mokagram) in the README and the About window and keep the [notice](MOKAGRAM_NOTICE.md).*

<p align="center">
  <a href="https://t.me/ecronx">@ecronx</a> · <a href="https://github.com/mokagram/mokagram/releases">релизы</a>
</p>
