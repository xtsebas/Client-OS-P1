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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCloseEvent>

#include "connectiondialog.h"
#include "userchatitem.h"
#include "messagebubble.h"
#include "websocketclient.h"

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

public slots:
    void clearMessageDisplay();  // Nuevo slot

private slots:
    // Connection handling
    void onConnectTriggered();
    void onDisconnectTriggered();
    
    // WebSocket events
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    
    // Message handling
    void onMessageReceived(const QString &sender, const QString &message);
    // NUEVO SLOT para procesar mensajes con bandera de historial
    void onMessageReceivedWithFlag(const QString &sender, const QString &message, bool isHistory);
    void onSendButtonClicked();
    void onMessageInputChanged();
    
    // User interaction
    void onUserItemClicked(QListWidgetItem *item);
    void onBroadcastItemClicked(QListWidgetItem *item);
    void onStatusChanged(int index);
    
    // Info panel
    void onInfoButtonClicked();
    void onCloseInfoButtonClicked();
    void onRefreshInfoButtonClicked();
    void showCurrentUserInfo();
    
    // Menu actions
    void onAboutTriggered();
    void onHelpTriggered();
    
    // User list and status
    void onUserListReceived(const QStringList &users);
    void onUserStatusReceived(quint8 status);
    
    // Timer events
    void onInactivityTimeout();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // Core UI setup
    void setupInitialUI();
    void updateUserAvatar();
    
    // Connection and messaging
    void getChatHistory(const QString &chatName);
    QString getLocalIPAddress();
    
    // Chat message handling
    void addChatMessage(const QString &sender, const QString &message, MessageBubble::MessageType type);
    void addSystemMessage(const QString &message);
    void updateUserLastMessage(const QString &username, const QString &message);
    
    // Chat history
    void loadDirectChatHistory(const QString &username);
    void loadBroadcastChatHistory();

    Ui::MainWindow *ui;
    WebSocketClient *m_webSocketClient;
    bool m_connected;
    QString m_currentUsername;
    QString m_currentChat;
    QString m_currentStatus;
    QTimer *m_inactivityTimer;
    QNetworkAccessManager *m_networkManager;

    // NUEVA VARIABLE: Almacena para qué chat se está solicitando el historial
    QString m_requestedHistoryChat;
};

#endif // MAINWINDOW_H
