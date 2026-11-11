/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "mtproto/details/mtproto_abstract_socket.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtNetwork/QNetworkProxy>

#include <rpl/lifetime.h>

#include <memory>
#include <vector>

namespace Core {
class WebSocketClient;
} // namespace Core

namespace MTP::details {

class TcpSocket;

// Mokagram: use Telegram's WebSocket frontends, with TCP fallback before the
// MTProto connection has received its first response.
class WebSocketFallbackSocket final : public AbstractSocket {
public:
	WebSocketFallbackSocket(
		not_null<QThread*> thread,
		QString fallbackAddress,
		int fallbackPort,
		int16 protocolDcId,
		bool protocolForFiles,
		const QNetworkProxy &fallbackProxy);
	~WebSocketFallbackSocket();

	void connectToHost(const QString &address, int port) override;
	bool isGoodStartNonce(bytes::const_span nonce) override;
	void timedOut() override;
	bool isConnected() override;
	bool hasBytesAvailable() override;
	int64 read(bytes::span buffer) override;
	void write(bytes::const_span prefix, bytes::const_span buffer) override;

	int32 debugState() override;
	QString debugPostfix() const override;

private:
	enum class State {
		NotConnected,
		ConnectingWebSocket,
		ConnectedWebSocket,
		ConnectingTcpFallback,
		ConnectedTcpFallback,
		Disconnected,
		Error,
	};

	void prepareTargets(int16 protocolDcId, bool protocolForFiles);
	void connectNextWebSocket();
	void webSocketConnected();
	void webSocketData(QByteArray data);
	void webSocketClosed();
	void startTcpFallback();
	void tcpFallbackConnected();
	void tcpFallbackError();
	void flushReplayWrites();
	void appendIncoming(QByteArray data);
	void closeWebSocket();

	const QString _fallbackAddress;
	const int _fallbackPort = 0;
	const bool _protocolForFiles = false;
	const QNetworkProxy _fallbackProxy;
	std::vector<QString> _targets;
	QString _requestTarget;
	int _targetIndex = 0;
	QString _activeTarget;
	Core::WebSocketClient *_webSocket = nullptr;
	std::unique_ptr<TcpSocket> _tcpFallback;
	rpl::lifetime _tcpFallbackLifetime;
	QByteArray _incoming;
	int _incomingOffset = 0;
	std::vector<QByteArray> _replayWrites;
	int64 _replayBytes = 0;
	State _state = State::NotConnected;
	bool _logicalConnected = false;
	bool _receivedData = false;

};

} // namespace MTP::details
