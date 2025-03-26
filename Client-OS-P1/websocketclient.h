#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H
#pragma once

#include <QObject>
#include <QWebSocket>

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(const QUrl& url, const QString& username, QObject* parent = nullptr);
    void sendMessage(const QString& message);

signals:
    void messageReceived(const QString& message);
    void connected();
    void disconnected();

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onDisconnected();

private:
    QWebSocket socket;
    bool isConnected() const;
    QString username;
};

#endif // WEBSOCKETCLIENT_H
