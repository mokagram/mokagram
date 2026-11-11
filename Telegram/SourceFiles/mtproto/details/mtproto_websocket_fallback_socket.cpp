/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "mtproto/details/mtproto_websocket_fallback_socket.h"

#include "core/websocket_client.h"
#include "mtproto/details/mtproto_tcp_socket.h"

#include <crl/crl_time.h>
#include <rpl/on_next.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QtEndian>

// Mokagram: bridge MTProto packets over Telegram's binary WebSocket endpoint.
namespace MTP::details {
namespace {

constexpr auto kWebSocketPort = 443;
constexpr auto kTestModeDcIdShift = 10000;
constexpr auto kFailureBackoff = crl::time(60 * 1000);
constexpr auto kMaxReplayBytes = 4 * 1024 * 1024;
QMutex &FailureMutex() {
	static auto result = QMutex();
	return result;
}

QHash<QString, crl::time> &FailureBackoffs() {
	static auto result = QHash<QString, crl::time>();
	return result;
}

[[nodiscard]] bool ShouldTryTarget(const QString &host) {
	auto lock = QMutexLocker(&FailureMutex());
	auto &backoffs = FailureBackoffs();
	const auto i = backoffs.find(host);
	if (i == backoffs.end()) {
		return true;
	} else if (i.value() <= crl::now()) {
		backoffs.erase(i);
		return true;
	}
	return false;
}

void MarkTargetFailed(const QString &host) {
	if (host.isEmpty()) {
		return;
	}
	auto lock = QMutexLocker(&FailureMutex());
	FailureBackoffs().insert(host, crl::now() + kFailureBackoff);
}

void MarkTargetSucceeded(const QString &host) {
	if (host.isEmpty()) {
		return;
	}
	auto lock = QMutexLocker(&FailureMutex());
	FailureBackoffs().remove(host);
}

} // namespace

WebSocketFallbackSocket::WebSocketFallbackSocket(
	not_null<QThread*> thread,
	QString fallbackAddress,
	int fallbackPort,
	int16 protocolDcId,
	bool protocolForFiles,
	const QNetworkProxy &fallbackProxy)
: AbstractSocket(thread)
, _fallbackAddress(std::move(fallbackAddress))
, _fallbackPort(fallbackPort)
, _protocolForFiles(protocolForFiles)
, _fallbackProxy(fallbackProxy) {
	prepareTargets(protocolDcId, protocolForFiles);
}

WebSocketFallbackSocket::~WebSocketFallbackSocket() {
	_tcpFallbackLifetime.destroy();
	closeWebSocket();
}

void WebSocketFallbackSocket::prepareTargets(
		int16 protocolDcId,
		bool protocolForFiles) {
	auto dc = std::abs(int(protocolDcId));
	const auto test = (dc > kTestModeDcIdShift);
	if (test) {
		dc -= kTestModeDcIdShift;
	}
	if (dc == 203) {
		dc = 2;
	}
	if (dc < 1 || dc > 5) {
		return;
	}
	const auto suffix = QString::number(dc);
	if (protocolForFiles) {
		_targets.push_back(u"kws"_q + suffix + u"-1.web.telegram.org"_q);
	}
	_targets.push_back(u"kws"_q + suffix + u".web.telegram.org"_q);
	_requestTarget = test ? u"/apiws_test"_q : u"/apiws"_q;
}

void WebSocketFallbackSocket::connectToHost(const QString &, int) {
	if (_state != State::NotConnected) {
		return;
	}
	_state = State::ConnectingWebSocket;
	connectNextWebSocket();
}

void WebSocketFallbackSocket::connectNextWebSocket() {
	if (_state != State::ConnectingWebSocket) {
		return;
	}
	while (_targetIndex < int(_targets.size())) {
		const auto target = _targets[_targetIndex++];
		if (!ShouldTryTarget(target)) {
			continue;
		}
		_activeTarget = target;
		const auto socket = new Core::WebSocketClient(this);
		_webSocket = socket;
		socket->onConnected = [=] {
			if (_webSocket == socket) {
				webSocketConnected();
			}
		};
		socket->onBinary = [=](QByteArray data) {
			if (_webSocket == socket) {
				webSocketData(std::move(data));
			}
		};
		socket->onClosed = [=] {
			if (_webSocket == socket) {
				webSocketClosed();
			}
		};
		socket->connectTo(target, kWebSocketPort, _requestTarget, u"binary"_q);
		return;
	}
	startTcpFallback();
}

void WebSocketFallbackSocket::webSocketConnected() {
	if (_state != State::ConnectingWebSocket) {
		return;
	}
	_state = State::ConnectedWebSocket;
	if (_logicalConnected) {
		flushReplayWrites();
	} else {
		_logicalConnected = true;
		_connected.fire({});
	}
}

void WebSocketFallbackSocket::webSocketData(QByteArray data) {
	if (_state != State::ConnectedWebSocket || data.isEmpty()) {
		return;
	}
	if (!_receivedData) {
		_receivedData = true;
		MarkTargetSucceeded(_activeTarget);
		_replayWrites.clear();
		_replayBytes = 0;
	}
	appendIncoming(std::move(data));
}

void WebSocketFallbackSocket::webSocketClosed() {
	if (_state != State::ConnectingWebSocket
		&& _state != State::ConnectedWebSocket) {
		return;
	}
	MarkTargetFailed(_activeTarget);
	closeWebSocket();
	if (_receivedData) {
		_state = State::Disconnected;
		_disconnected.fire({});
		return;
	}
	_state = State::ConnectingWebSocket;
	connectNextWebSocket();
}

void WebSocketFallbackSocket::startTcpFallback() {
	if (_state == State::ConnectingTcpFallback
		|| _state == State::ConnectedTcpFallback
		|| _state == State::Error
		|| _state == State::Disconnected) {
		return;
	}
	_state = State::ConnectingTcpFallback;
	closeWebSocket();
	_tcpFallback = std::make_unique<TcpSocket>(
		thread(),
		_fallbackProxy,
		_protocolForFiles);
	_tcpFallback->setDebugId(_debugId + u"_tcp"_q);
	_tcpFallback->connected() | rpl::on_next([=] {
		tcpFallbackConnected();
	}, _tcpFallbackLifetime);
	_tcpFallback->disconnected() | rpl::on_next([=] {
		if (_state == State::ConnectedTcpFallback) {
			_state = State::Disconnected;
			_disconnected.fire({});
		}
	}, _tcpFallbackLifetime);
	_tcpFallback->readyRead() | rpl::on_next([=] {
		_readyRead.fire({});
	}, _tcpFallbackLifetime);
	_tcpFallback->error() | rpl::on_next([=] {
		tcpFallbackError();
	}, _tcpFallbackLifetime);
	_tcpFallback->connectToHost(_fallbackAddress, _fallbackPort);
}

void WebSocketFallbackSocket::tcpFallbackConnected() {
	if (_state != State::ConnectingTcpFallback) {
		return;
	}
	_state = State::ConnectedTcpFallback;
	flushReplayWrites();
	if (!_logicalConnected) {
		_logicalConnected = true;
		_connected.fire({});
	}
}

void WebSocketFallbackSocket::tcpFallbackError() {
	if (_state == State::Disconnected || _state == State::Error) {
		return;
	}
	_state = State::Error;
	_error.fire({});
}

void WebSocketFallbackSocket::flushReplayWrites() {
	if (_replayWrites.empty()) {
		return;
	}
	if (_state == State::ConnectedWebSocket && _webSocket) {
		for (const auto &write : _replayWrites) {
			_webSocket->sendBinary(write);
		}
	} else if (_state == State::ConnectedTcpFallback && _tcpFallback) {
		for (const auto &write : _replayWrites) {
			_tcpFallback->write({}, bytes::make_span(write));
		}
	}
	_replayWrites.clear();
	_replayBytes = 0;
}

void WebSocketFallbackSocket::appendIncoming(QByteArray data) {
	if (data.isEmpty()) {
		return;
	}
	_incoming.append(data);
	_readyRead.fire({});
}

void WebSocketFallbackSocket::closeWebSocket() {
	if (const auto socket = std::exchange(_webSocket, nullptr)) {
		socket->close();
		socket->deleteLater();
	}
}

bool WebSocketFallbackSocket::isGoodStartNonce(bytes::const_span nonce) {
	Expects(nonce.size() >= 2 * sizeof(uint32));

	const auto data = nonce.data();
	const auto zero = gsl::to_integer<uchar>(nonce.front());
	const auto first = qFromUnaligned<uint32>(data);
	const auto second = qFromUnaligned<uint32>(data + sizeof(uint32));
	return (zero != 0xEFU)
		&& (first != 0x44414548U)
		&& (first != 0x54534F50U)
		&& (first != 0x20544547U)
		&& (first != 0xEEEEEEEEU)
		&& (first != 0xDDDDDDDDU)
		&& (first != 0x02010316U)
		&& (second != 0x00000000U);
}

void WebSocketFallbackSocket::timedOut() {
	if ((_state == State::ConnectingWebSocket
			|| _state == State::ConnectedWebSocket)
		&& !_receivedData) {
		MarkTargetFailed(_activeTarget);
		closeWebSocket();
		_state = State::ConnectingWebSocket;
		connectNextWebSocket();
	} else if (_state == State::ConnectingTcpFallback && _tcpFallback) {
		_tcpFallback->timedOut();
	}
}

bool WebSocketFallbackSocket::isConnected() {
	return (_state == State::ConnectedWebSocket)
		|| (_state == State::ConnectedTcpFallback
			&& _tcpFallback
			&& _tcpFallback->isConnected());
}

bool WebSocketFallbackSocket::hasBytesAvailable() {
	return (_incomingOffset < _incoming.size())
		|| (_state == State::ConnectedTcpFallback
			&& _tcpFallback
			&& _tcpFallback->hasBytesAvailable());
}

int64 WebSocketFallbackSocket::read(bytes::span buffer) {
	const auto available = _incoming.size() - _incomingOffset;
	if (available > 0) {
		const auto count = int(std::min<std::size_t>(
			available,
			buffer.size()));
		memcpy(
			buffer.data(),
			_incoming.constData() + _incomingOffset,
			count);
		_incomingOffset += count;
		if (_incomingOffset == _incoming.size()) {
			_incoming.clear();
			_incomingOffset = 0;
		} else if (_incomingOffset >= 64 * 1024
			&& _incomingOffset >= _incoming.size() / 2) {
			_incoming.remove(0, _incomingOffset);
			_incomingOffset = 0;
		}
		return count;
	}
	return (_state == State::ConnectedTcpFallback && _tcpFallback)
		? _tcpFallback->read(buffer)
		: 0;
}

void WebSocketFallbackSocket::write(
		bytes::const_span prefix,
		bytes::const_span buffer) {
	Expects(!buffer.empty());

	if (_state == State::ConnectedTcpFallback && _tcpFallback) {
		_tcpFallback->write(prefix, buffer);
		return;
	}
	if (_state != State::ConnectedWebSocket && _state != State::ConnectingTcpFallback) {
		return;
	}
	auto data = QByteArray(
		int(prefix.size() + buffer.size()),
		Qt::Uninitialized);
	if (!prefix.empty()) {
		memcpy(data.data(), prefix.data(), prefix.size());
	}
	memcpy(data.data() + prefix.size(), buffer.data(), buffer.size());
	if (!_receivedData) {
		_replayBytes += data.size();
		_replayWrites.push_back(data);
		if (_replayBytes > kMaxReplayBytes) {
			MarkTargetFailed(_activeTarget);
			startTcpFallback();
			return;
		}
	}
	if (_state == State::ConnectedWebSocket && _webSocket) {
		_webSocket->sendBinary(data);
	}
}

int32 WebSocketFallbackSocket::debugState() {
	return int32(_state);
}

QString WebSocketFallbackSocket::debugPostfix() const {
	return (_state == State::ConnectingTcpFallback
		|| _state == State::ConnectedTcpFallback)
		? u"_tcp_fallback"_q
		: u"_ws"_q;
}

} // namespace MTP::details
