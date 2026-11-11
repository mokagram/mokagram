#include "mokagram/plugins/mokagram_plugins.h"

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Mokagram::Plugins {
namespace {

struct PluginEntry {
	std::string id;
	std::string file;
	std::string state;
	std::string error;
	std::string name;
	std::string description;
	std::string details;
	std::string author;
	std::string version;
	std::string icon;
	std::vector<std::string> permissions;
	bool hasSettings = false;
};

struct HookTask {
	int kind = 0;
	uint32_t id = 0;
	std::string bytes;
};

constexpr auto kQueueLimit = size_t(64);
constexpr auto kMaxHookBytes = size_t(256 * 1024);

std::atomic<bool> _started = false;
std::atomic<bool> _engineEnabled = true;
std::atomic<bool> _stopping = false;
std::atomic<int> _fromPlugin = 0;

std::mutex _dataMutex;
std::unordered_map<std::string, PluginEntry> _plugins;
std::unordered_set<std::string> _pinned;
std::unordered_set<std::string> _disabled;
std::unordered_map<std::string, std::vector<std::string>> _grants;
std::unordered_map<std::string, std::string> _hashes;

BridgeFn _bridge;
std::mutex _bridgeMutex;

std::mutex _queueMutex;
std::condition_variable _queueCv;
std::deque<HookTask> _queue;
std::thread _worker;

std::string DataRoot() {
	auto path = std::filesystem::path(PluginsDir());
	if (!path.has_filename()) {
		path = path.parent_path();
	}
	return path.parent_path().string();
}

void SaveStateLocked() {
	const auto rootPath = DataRoot();
	if (rootPath.empty()) {
		return;
	}
	QJsonObject root;
	root.insert("engine", _engineEnabled.load());
	QJsonArray pinnedArray;
	for (const auto &item : _pinned) {
		pinnedArray.append(QString::fromStdString(item));
	}
	root.insert("pinned", pinnedArray);
	QJsonArray disabledArray;
	for (const auto &item : _disabled) {
		disabledArray.append(QString::fromStdString(item));
	}
	root.insert("disabled", disabledArray);

	std::error_code ec;
	std::filesystem::create_directories(rootPath, ec);
	const auto stateFile = (std::filesystem::path(rootPath) / "plugins_state.json").string();
	QFile f(QString::fromStdString(stateFile));
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

void LoadStateLocked() {
	const auto rootPath = DataRoot();
	if (rootPath.empty()) {
		return;
	}
	const auto stateFile = (std::filesystem::path(rootPath) / "plugins_state.json").string();
	QFile f(QString::fromStdString(stateFile));
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	if (obj.contains("engine")) {
		_engineEnabled = obj.value("engine").toBool(true);
	}
	_pinned.clear();
	for (const auto &val : obj.value("pinned").toArray()) {
		_pinned.insert(val.toString().toStdString());
	}
	_disabled.clear();
	for (const auto &val : obj.value("disabled").toArray()) {
		_disabled.insert(val.toString().toStdString());
	}
}

void SaveGrantsLocked() {
	const auto rootPath = DataRoot();
	if (rootPath.empty()) {
		return;
	}
	QJsonObject root;
	for (const auto &[pid, grants] : _grants) {
		QJsonObject entry;
		QJsonArray grantsArr;
		for (const auto &g : grants) {
			grantsArr.append(QString::fromStdString(g));
		}
		entry.insert("grants", grantsArr);
		const auto itHash = _hashes.find(pid);
		if (itHash != _hashes.end()) {
			entry.insert("hash", QString::fromStdString(itHash->second));
		}
		root.insert(QString::fromStdString(pid), entry);
	}
	std::error_code ec;
	std::filesystem::create_directories(rootPath, ec);
	const auto grantsFile = (std::filesystem::path(rootPath) / "plugins_grants.json").string();
	QFile f(QString::fromStdString(grantsFile));
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
}

void LoadGrantsLocked() {
	const auto rootPath = DataRoot();
	if (rootPath.empty()) {
		return;
	}
	const auto grantsFile = (std::filesystem::path(rootPath) / "plugins_grants.json").string();
	QFile f(QString::fromStdString(grantsFile));
	if (!f.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return;
	}
	_grants.clear();
	_hashes.clear();
	const auto obj = doc.object();
	for (auto it = obj.begin(); it != obj.end(); ++it) {
		const auto pid = it.key().toStdString();
		if (it.value().isObject()) {
			const auto entry = it.value().toObject();
			std::vector<std::string> perms;
			for (const auto &v : entry.value("grants").toArray()) {
				perms.push_back(v.toString().toStdString());
			}
			_grants[pid] = std::move(perms);
			if (entry.contains("hash")) {
				_hashes[pid] = entry.value("hash").toString().toStdString();
			}
		}
	}
}

void WorkerLoop() {
	while (!_stopping.load()) {
		HookTask task;
		{
			std::unique_lock<std::mutex> lock(_queueMutex);
			_queueCv.wait(lock, [] {
				return _stopping.load() || !_queue.empty();
			});
			if (_stopping.load() && _queue.empty()) {
				break;
			}
			task = std::move(_queue.front());
			_queue.pop_front();
		}
	}
}

}

std::string PluginsDir() {
	if (const auto env = std::getenv("MOKAGRAM_PLUGINS_DIR")) {
		if (*env) {
			return env;
		}
	}
#ifdef _WIN32
	if (const auto appdata = std::getenv("APPDATA")) {
		return std::string(appdata) + "/Telegram Desktop/tdata/plugins";
	}
#endif
	const auto home = std::getenv("HOME");
	return std::string(home ? home : ".") + "/.local/share/mokagram/plugins";
}

std::string ParseMetadataJson(const std::string &source) {
	const auto rawBytes = QByteArray::fromRawData(source.data(), qsizetype(source.size()));
	const auto sha256 = QString::fromLatin1(
		QCryptographicHash::hash(rawBytes, QCryptographicHash::Sha256).toHex());

	QJsonObject meta;
	QJsonArray detected;
	QJsonArray evidence;

	const auto doc = QJsonDocument::fromJson(rawBytes);
	if (doc.isObject()) {
		const auto obj = doc.object();
		const auto readStr = [&](const char *k1, const char *k2) -> QString {
			if (obj.contains(k1)) return obj.value(k1).toString();
			if (obj.contains(k2)) return obj.value(k2).toString();
			return {};
		};
		meta.insert("__id__", readStr("__id__", "id"));
		meta.insert("__name__", readStr("__name__", "name"));
		meta.insert("__description__", readStr("__description__", "description"));
		meta.insert("__author__", readStr("__author__", "author"));
		meta.insert("__version__", readStr("__version__", "version"));
		meta.insert("__icon__", readStr("__icon__", "icon"));
		meta.insert("__details__", readStr("__details__", "details"));

		QJsonArray perms;
		if (obj.contains("__permissions__")) {
			perms = obj.value("__permissions__").toArray();
		} else if (obj.contains("permissions")) {
			perms = obj.value("permissions").toArray();
		}
		meta.insert("__permissions__", perms);
	} else {
		const auto srcStr = QString::fromUtf8(rawBytes);
		const auto extractString = [&](const QString &key) -> QString {
			QRegularExpression re(
				key + QStringLiteral(R"(\s*=\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"([^"\\]*(?:\\.[^"\\]*)*)"|'([^'\\]*(?:\\.[^'\\]*)*)'))"));
			auto match = re.match(srcStr);
			if (match.hasMatch()) {
				for (int i = 1; i <= 4; ++i) {
					if (!match.captured(i).isEmpty() || match.capturedLength(i) == 0) {
						if (!match.captured(i).isNull()) {
							return match.captured(i);
						}
					}
				}
			}
			return {};
		};

		meta.insert("__id__", extractString(QStringLiteral("__id__")));
		meta.insert("__name__", extractString(QStringLiteral("__name__")));
		meta.insert("__description__", extractString(QStringLiteral("__description__")));
		meta.insert("__author__", extractString(QStringLiteral("__author__")));
		meta.insert("__version__", extractString(QStringLiteral("__version__")));
		meta.insert("__icon__", extractString(QStringLiteral("__icon__")));
		meta.insert("__details__", extractString(QStringLiteral("__details__")));

		QJsonArray perms;
		QRegularExpression permRe(QStringLiteral(R"(__permissions__\s*=\s*\[([^\]]*)\])"));
		auto permMatch = permRe.match(srcStr);
		if (permMatch.hasMatch()) {
			const auto body = permMatch.captured(1);
			QRegularExpression itemRe(QStringLiteral(R"(["']([^"']+)["'])"));
			auto it = itemRe.globalMatch(body);
			while (it.hasNext()) {
				perms.append(it.next().captured(1));
			}
		}
		meta.insert("__permissions__", perms);

		const auto checkCapability = [&](const QString &cap, const QStringList &tokens) {
			for (const auto &token : tokens) {
				if (srcStr.contains(token)) {
					detected.append(cap);
					break;
				}
			}
		};
		checkCapability(QStringLiteral("network"), { QStringLiteral("http"), QStringLiteral("socket"), QStringLiteral("urllib"), QStringLiteral("requests") });
		checkCapability(QStringLiteral("files"), { QStringLiteral("open("), QStringLiteral("os.path"), QStringLiteral("pathlib") });
		checkCapability(QStringLiteral("ui"), { QStringLiteral("toast"), QStringLiteral("box"), QStringLiteral("menu") });
		checkCapability(QStringLiteral("hooks"), { QStringLiteral("hook"), QStringLiteral("filter") });
	}

	QJsonObject root;
	root.insert("meta", meta);
	root.insert("detected", detected);
	root.insert("evidence", evidence);
	root.insert("error", QJsonValue());
	root.insert("sha256", sha256);

	return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

std::string RequestInfoJson(const std::string &encoded) {
	return R"({"ok":true,"allowed":true,"method":"unknown"})";
}

std::string LoadFile(const std::string &fileName, LogFn log) {
	const auto dirPath = QString::fromStdString(PluginsDir());
	const auto filePath = QDir(dirPath).filePath(QString::fromStdString(fileName));
	QFile f(filePath);
	if (!f.open(QIODevice::ReadOnly)) {
		return {};
	}
	const auto bytes = f.readAll();
	const auto metaJson = ParseMetadataJson(std::string(bytes.constData(), size_t(bytes.size())));
	const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(metaJson));
	if (!doc.isObject()) {
		return {};
	}
	const auto root = doc.object();
	const auto meta = root.value("meta").toObject();
	auto pid = meta.value("__id__").toString().toStdString();
	if (pid.empty()) {
		pid = QFileInfo(filePath).completeBaseName().toStdString();
	}

	PluginEntry entry;
	entry.id = pid;
	entry.file = fileName;
	entry.name = meta.value("__name__").toString().toStdString();
	if (entry.name.empty()) {
		entry.name = pid;
	}
	entry.description = meta.value("__description__").toString().toStdString();
	entry.details = meta.value("__details__").toString().toStdString();
	entry.author = meta.value("__author__").toString().toStdString();
	entry.version = meta.value("__version__").toString().toStdString();
	entry.icon = meta.value("__icon__").toString().toStdString();
	for (const auto &p : meta.value("__permissions__").toArray()) {
		entry.permissions.push_back(p.toString().toStdString());
	}

	{
		std::lock_guard<std::mutex> lock(_dataMutex);
		const auto disabled = _disabled.contains(pid);
		entry.state = disabled ? "disabled" : "loaded";
		_plugins[pid] = std::move(entry);
		_hashes[pid] = root.value("sha256").toString().toStdString();
	}

	return pid;
}

std::string ListJson() {
	std::lock_guard<std::mutex> lock(_dataMutex);
	QJsonArray list;
	for (const auto &[pid, entry] : _plugins) {
		QJsonObject obj;
		obj.insert("id", QString::fromStdString(entry.id));
		obj.insert("file", QString::fromStdString(entry.file));
		const auto isDisabled = _disabled.contains(entry.id);
		obj.insert("state", QString::fromLatin1(isDisabled ? "disabled" : "loaded"));
		obj.insert("error", QJsonValue());
		obj.insert("enabled", !isDisabled);
		obj.insert("pinned", _pinned.contains(entry.id));
		obj.insert("name", QString::fromStdString(entry.name));
		obj.insert("description", QString::fromStdString(entry.description));
		obj.insert("details", QString::fromStdString(entry.details));
		obj.insert("has_settings", entry.hasSettings);
		obj.insert("author", QString::fromStdString(entry.author));
		obj.insert("version", QString::fromStdString(entry.version));
		obj.insert("icon", QString::fromStdString(entry.icon));
		QJsonArray perms;
		for (const auto &p : entry.permissions) {
			perms.append(QString::fromStdString(p));
		}
		obj.insert("permissions", perms);
		list.append(obj);
	}

	std::vector<QJsonObject> sorted;
	sorted.reserve(list.size());
	for (const auto &v : list) {
		sorted.push_back(v.toObject());
	}
	std::sort(sorted.begin(), sorted.end(), [](const QJsonObject &a, const QJsonObject &b) {
		const auto aPinned = a.value("pinned").toBool();
		const auto bPinned = b.value("pinned").toBool();
		if (aPinned != bPinned) {
			return aPinned;
		}
		return a.value("name").toString() < b.value("name").toString();
	});

	QJsonArray result;
	for (const auto &item : sorted) {
		result.append(item);
	}
	return QJsonDocument(result).toJson(QJsonDocument::Compact).toStdString();
}

std::string EffectiveGrantsJson(const std::string &pid) {
	std::lock_guard<std::mutex> lock(_dataMutex);
	const auto it = _grants.find(pid);
	if (it == _grants.end()) {
		return "[]";
	}
	QJsonArray arr;
	for (const auto &g : it->second) {
		arr.append(QString::fromStdString(g));
	}
	return QJsonDocument(arr).toJson(QJsonDocument::Compact).toStdString();
}

bool EngineEnabled() {
	return _engineEnabled.load();
}

bool SetEngineEnabled(bool enabled, LogFn log) {
	_engineEnabled = enabled;
	std::lock_guard<std::mutex> lock(_dataMutex);
	SaveStateLocked();
	return true;
}

bool SetPinned(const std::string &id, bool pinned) {
	std::lock_guard<std::mutex> lock(_dataMutex);
	if (!_plugins.contains(id)) {
		return false;
	}
	if (pinned) {
		_pinned.insert(id);
	} else {
		_pinned.erase(id);
	}
	SaveStateLocked();
	return true;
}

bool SetEnabled(const std::string &id, bool enabled, LogFn log) {
	std::lock_guard<std::mutex> lock(_dataMutex);
	const auto it = _plugins.find(id);
	if (it == _plugins.end()) {
		return false;
	}
	if (enabled) {
		_disabled.erase(id);
		it->second.state = "loaded";
	} else {
		_disabled.insert(id);
		it->second.state = "disabled";
	}
	SaveStateLocked();
	return true;
}

bool Remove(const std::string &id, LogFn log) {
	std::string fileName;
	{
		std::lock_guard<std::mutex> lock(_dataMutex);
		const auto it = _plugins.find(id);
		if (it == _plugins.end()) {
			return false;
		}
		fileName = it->second.file;
		_plugins.erase(it);
		_pinned.erase(id);
		_disabled.erase(id);
		_grants.erase(id);
		_hashes.erase(id);
		SaveStateLocked();
		SaveGrantsLocked();
	}
	if (!fileName.empty()) {
		const auto path = QDir(QString::fromStdString(PluginsDir())).filePath(QString::fromStdString(fileName));
		QFile::remove(path);
	}
	return true;
}

std::string CallHook(
		const std::string &name,
		const std::string &paramsJson,
		LogFn log) {
	return {};
}

std::string MenuItemsJson(
		const std::string &location,
		const std::string &ctxJson) {
	return "[]";
}

bool MenuItemClick(
		const std::string &pid,
		int index,
		const std::string &ctxJson,
		LogFn log) {
	return false;
}

bool CallMessageFilter(const std::string &messageJson, LogFn log) {
	return false;
}

std::string CallTextHook(const std::string &messageJson, LogFn log) {
	return {};
}

void OpenPluginBuilder(LogFn log) {
}

void RestoreBackupFile(const std::string &path, LogFn log) {
}

std::string CallBadgeHook(const std::string &messageJson, LogFn log) {
	return {};
}

bool Invoke(
		const std::string &pid,
		const std::string &token,
		const std::string &payloadJson,
		LogFn log) {
	return false;
}

std::string SharedTempDir() {
	return {};
}

void SetSandboxExcluded(std::vector<std::string> paths) {
}

bool HostAlive() {
	return true;
}

bool RestartHost(LogFn log) {
	return true;
}

void Shutdown(LogFn log) {
	if (!_started) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		_stopping = true;
		_queue.clear();
	}
	_queueCv.notify_all();
	if (_worker.joinable()) {
		_worker.join();
	}
}

void SetBridge(BridgeFn bridge) {
	std::lock_guard<std::mutex> lock(_bridgeMutex);
	_bridge = std::move(bridge);
}

bool Stopping() {
	return _stopping.load();
}

bool HookActive(HookKind kind) {
	return false;
}

bool HookWanted(HookKind kind, uint32_t constructorId) {
	return false;
}

void QueueHook(
		HookKind kind,
		uint32_t constructorId,
		const void *data,
		size_t sizeBytes) {
	if (!_started || _stopping.load() || !data || sizeBytes > kMaxHookBytes) {
		return;
	}
	HookTask task{
		.kind = int(kind),
		.id = constructorId,
		.bytes = std::string(static_cast<const char*>(data), sizeBytes),
	};
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		if (_queue.size() >= kQueueLimit) {
			_queue.pop_front();
		}
		_queue.push_back(std::move(task));
	}
	_queueCv.notify_one();
}

void QueueEvent(const std::string &name, const std::string &payloadJson) {
	if (!_started || _stopping.load() || payloadJson.size() > kMaxHookBytes) {
		return;
	}
	HookTask task{
		.kind = 100,
		.id = 0,
		.bytes = name + "\n" + payloadJson,
	};
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		if (_queue.size() >= kQueueLimit) {
			_queue.pop_front();
		}
		_queue.push_back(std::move(task));
	}
	_queueCv.notify_one();
}

void MarkFromPlugin(bool on) {
	if (on) {
		++_fromPlugin;
	} else if (_fromPlugin.load() > 0) {
		--_fromPlugin;
	}
}

bool IsFromPlugin() {
	return _fromPlugin.load() > 0;
}

std::string RecordInstall(const std::string &fileName, LogFn log) {
	const auto dirPath = QString::fromStdString(PluginsDir());
	const auto filePath = QDir(dirPath).filePath(QString::fromStdString(fileName));
	QFile f(filePath);
	if (!f.open(QIODevice::ReadOnly)) {
		return {};
	}
	const auto bytes = f.readAll();
	const auto metaJson = ParseMetadataJson(std::string(bytes.constData(), size_t(bytes.size())));
	const auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(metaJson));
	if (!doc.isObject()) {
		return {};
	}
	const auto root = doc.object();
	const auto meta = root.value("meta").toObject();
	auto pid = meta.value("__id__").toString().toStdString();
	if (pid.empty()) {
		pid = QFileInfo(filePath).completeBaseName().toStdString();
	}
	std::vector<std::string> perms;
	for (const auto &p : meta.value("__permissions__").toArray()) {
		perms.push_back(p.toString().toStdString());
	}
	{
		std::lock_guard<std::mutex> lock(_dataMutex);
		_grants[pid] = std::move(perms);
		_hashes[pid] = root.value("sha256").toString().toStdString();
		SaveGrantsLocked();
	}
	return pid;
}

bool Approve(const std::string &id, LogFn log) {
	std::string fileName;
	{
		std::lock_guard<std::mutex> lock(_dataMutex);
		const auto it = _plugins.find(id);
		if (it != _plugins.end()) {
			fileName = it->second.file;
		}
	}
	if (!fileName.empty()) {
		RecordInstall(fileName, log);
		LoadFile(fileName, log);
		return true;
	}
	return false;
}

void NotifySettingChanged(
		const std::string &key,
		const std::string &valueJson,
		LogFn log) {
}

int Start(LogFn log) {
	if (_started.exchange(true)) {
		return int(_plugins.size());
	}
	{
		std::lock_guard<std::mutex> lock(_dataMutex);
		LoadStateLocked();
		LoadGrantsLocked();
	}
	InstallBridge();

	const auto dir = QDir(QString::fromStdString(PluginsDir()));
	if (dir.exists()) {
		const auto list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
		for (const auto &info : list) {
			if (info.suffix() == QStringLiteral("plugin")) {
				LoadFile(info.fileName().toStdString(), log);
			}
		}
	}

	_worker = std::thread(WorkerLoop);

	std::lock_guard<std::mutex> lock(_dataMutex);
	return int(_plugins.size());
}

} Mokagram::Plugins