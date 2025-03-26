#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent, const QString& username)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
    , wsClient(nullptr)
{
    ui->setupUi(this);

    // Obtener IP local solo como utilidad
    QString ip = getLocalIPAddress();

    // 1. Verificar si el servidor HTTP responde
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkReply);
    QUrl httpUrl("http://18.224.60.241:18080/");
    QNetworkRequest request(httpUrl);
    networkManager->get(request);

    // 2. Establecer conexión WebSocket con el nombre proporcionado
    QUrl wsUrl("ws://18.224.60.241:18080/ws");
    wsClient = new WebSocketClient(wsUrl, username, this);

    // Mostrar mensaje recibido
    connect(wsClient, &WebSocketClient::messageReceived, this, [=](const QString &msg) {
        ui->textEdit_2->append(">> " + msg);
    });

    // Confirmar conexión
    connect(wsClient, &WebSocketClient::connected, this, [=]() {
        ui->textEdit_2->append("✅ WebSocket conectado");
    });

    // Confirmar desconexión
    connect(wsClient, &WebSocketClient::disconnected, this, [=]() {
        ui->textEdit_2->append("❌ WebSocket desconectado");
    });

    // Enviar mensaje desde caja de texto
    connect(ui->sendButton, &QPushButton::clicked, this, [=]() {
        QString message = ui->messageInput->text().trimmed();
        if (!message.isEmpty()) {
            wsClient->sendMessage(message);
            ui->messageInput->clear();
        }
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
        ui->textEdit_2->append("GET / ➤ " + response);
    } else {
        ui->textEdit_2->append("Error GET: " + reply->errorString());
    }
    reply->deleteLater();
}

MainWindow::~MainWindow()
{
    delete ui;
}
