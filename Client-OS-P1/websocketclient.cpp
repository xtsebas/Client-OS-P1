#include "websocketclient.h"
#include <QUrlQuery>
#include <QDebug>

WebSocketClient::WebSocketClient(const QUrl& url, const QString& username, QObject* parent)
    : QObject(parent), username(username)
{
    connect(&socket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&socket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);

    QUrl fullUrl = url;
    QUrlQuery query;
    query.addQueryItem("name", username);
    fullUrl.setQuery(query);

    socket.open(fullUrl);
}

void WebSocketClient::onConnected() {
    emit connected();
    socket.sendTextMessage("¡Hola Usuario!");
}

void WebSocketClient::onTextMessageReceived(const QString& message) {
    emit messageReceived(message);
}

bool WebSocketClient::isConnected() const {
    return socket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::onDisconnected() {
    emit disconnected();
}

void WebSocketClient::sendMessage(const QString& message) {
    if (socket.isValid()) {
        socket.sendTextMessage(message);
    }
}
