#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#include <QMainWindow>
#include <QWebSocket>
#include <QListWidgetItem>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QAbstractSocket>

#include "connectiondialog.h"
#include "userchatitem.h"
#include "messagebubble.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectTriggered();
    void onDisconnectTriggered();
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onTextMessageReceived(const QString &message);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onSendButtonClicked();
    void onMessageInputChanged();
    void onUserItemClicked(QListWidgetItem *item);
    void onBroadcastItemClicked(QListWidgetItem *item);
    void onStatusChanged(int index);
    void onInfoButtonClicked();
    void onCloseInfoButtonClicked();
    void onRefreshInfoButtonClicked();
    void onAboutTriggered();
    void onHelpTriggered();
    void onInactivityTimeout();

private:
    void setupInitialUI();
    void updateUserAvatar();
    void requestUserList();
    void requestUserInfo(const QString &username);
    void handleUserListResponse(const QJsonObject &obj);
    void handleUserInfoResponse(const QJsonObject &obj);
    void handleStatusUpdateResponse(const QJsonObject &obj);
    void handleChatMessage(const QJsonObject &obj);
    void handleBroadcastMessage(const QJsonObject &obj);
    void handleErrorMessage(const QJsonObject &obj);
    void addChatMessage(const QString &sender, const QString &message, MessageBubble::MessageType type);
    void addSystemMessage(const QString &message);
    void updateUserLastMessage(const QString &username, const QString &message);
    void loadDirectChatHistory(const QString &username);
    void loadBroadcastChatHistory();

    Ui::MainWindow *ui;
    QWebSocket *m_webSocket;
    bool m_connected;
    QString m_currentUsername;
    QString m_currentStatus;
    QTimer *m_inactivityTimer;
};

#endif // MAINWINDOW_H
