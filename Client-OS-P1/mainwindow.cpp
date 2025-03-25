#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
    , wsClient(nullptr)
{
    ui->setupUi(this);

    // Obtener la IP local real (ej. 192.168.0.23)
    QString ip = getLocalIPAddress();

    // 1. Realizar GET /
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkReply);
    QUrl httpUrl("http://18.224.60.241:18080/");
    QNetworkRequest request(httpUrl);
    networkManager->get(request);

    // 2. Conectarse al WebSocket
    QUrl wsUrl("ws://18.224.60.241:18080/ws");
    QString username = "Sebastian";
    wsClient = new WebSocketClient(wsUrl, username, this);

    connect(wsClient, &WebSocketClient::messageReceived, this, [=](const QString &msg) {
        ui->textEdit_2->append(">> " + msg);
    });

    connect(wsClient, &WebSocketClient::connected, this, [=]() {
        ui->textEdit_2->append("✅ WebSocket conectado");
    });

    connect(wsClient, &WebSocketClient::disconnected, this, [=]() {
        ui->textEdit_2->append("❌ WebSocket desconectado");
    });
}

QString MainWindow::getLocalIPAddress() {
    const auto interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            for (const QNetworkAddressEntry& entry : interface.addressEntries()) {
                QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                    return ip.toString();
                }
            }
        }
    }

    return "127.0.0.1";
}

void MainWindow::handleNetworkReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString response = reply->readAll();
        ui->textEdit->append("GET / ➤ " + response);
    } else {
        ui->textEdit->append("Error GET: " + reply->errorString());
    }
    reply->deleteLater();
}

MainWindow::~MainWindow()
{
    delete ui;
}
