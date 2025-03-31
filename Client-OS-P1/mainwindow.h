#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidgetItem>
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
    explicit MainWindow(QWidget *parent = nullptr, const QString& username = "");
    ~MainWindow();

private slots:
    void handleNetworkReply(QNetworkReply* reply);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    WebSocketClient *wsClient;
    QString username;

    QString currentChat = "~";  // Chat actual (general por defecto)
    QString getLocalIPAddress();
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
