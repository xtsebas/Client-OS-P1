#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H
#pragma once

#include <QObject>
#include <QWebSocket>

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(const QUrl& url, const QString& username, QObject* parent = nullptr);
    void sendMessage(const QString& recipient, const QString& message);
    void getChatHistory(const QString& chatName);
    void changeUserStatus(quint8 newStatus);
    bool isConnected() const;
    void onDisconnected();

signals:
    void messageReceived(const QString& sender, const QString& message);
    void userListReceived(const QStringList& users);
    void userStatusReceived(quint8 status);
    void connected();
    void disconnected();
    void statusChanged(quint8 newStatus);
    void connectionRejected();

private slots:
    void onConnected();
    void onBinaryMessageReceived(const QByteArray& message);
    void handleError(QDataStream& in);

private:
    QWebSocket socket;
    QString username;

    QString getString8(QDataStream &in, size_t &offset);
};

#endif // WEBSOCKETCLIENT_H
