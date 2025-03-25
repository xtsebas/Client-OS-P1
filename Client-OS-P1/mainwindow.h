#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleNetworkReply(QNetworkReply* reply);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    WebSocketClient *wsClient;

    QString getLocalIPAddress();
};
#endif // MAINWINDOW_H
