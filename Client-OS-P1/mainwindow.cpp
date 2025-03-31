#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent, const QString& username)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
    , wsClient(nullptr)
    , username(username)
    , currentChat("~")
{
    ui->setupUi(this);

    // Obtener IP local
    QString ip = getLocalIPAddress();

    // 1. Verificar si el servidor HTTP responde
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkReply);
    QUrl httpUrl("http://localhost:18080/");
    QNetworkRequest request(httpUrl);
    networkManager->get(request);

    // 2. Establecer conexión WebSocket con el nombre proporcionado
    QUrl wsUrl("ws://localhost:18080/");
    wsClient = new WebSocketClient(wsUrl, username, this);

    // Mostrar mensaje recibido en el chat actual
    connect(wsClient, &WebSocketClient::messageReceived, this, [=](const QString &sender, const QString &msg) {
        if (currentChat == "~" || currentChat == sender || sender == "~") {
            ui->textEdit_2->append(sender + ": " + msg);
        }
    });

    // Confirmar conexión con un único log
    connect(wsClient, &WebSocketClient::connected, this, [=]() {
        ui->textEdit_2->append("✅ WebSocket conectado");
    });

    // Mostrar lista de usuarios conectados
    connect(wsClient, &WebSocketClient::userListReceived, this, [=](const QStringList &users) {
        ui->userListWidget->clear();  // Limpiar lista anterior
        ui->userListWidget->addItems(users);
    });


    // Cambiar estado del usuario
    connect(ui->statusComboBox, &QComboBox::currentIndexChanged, this, [=](int index) {
        if (index == 0) {
            wsClient->changeUserStatus(0x01);  // Activo
        } else if (index == 1) {
            wsClient->changeUserStatus(0x02);  // Ocupado
        } else if (index == 2) {
            wsClient->changeUserStatus(0x03);  // Inactivo
        }
    });

    // Actualizar estado del comboBox si cambia el estado
    connect(wsClient, &WebSocketClient::userStatusReceived, this, [=](quint8 status) {
        ui->statusComboBox->setCurrentIndex(status - 1);
    });

    // Obtener historial del chat general solo al conectar
    connect(wsClient, &WebSocketClient::connected, this, [=]() {
        wsClient->getChatHistory("~");  // Cargar historial del chat general al conectar
    });


    // Enviar mensaje desde caja de texto
    connect(ui->sendButton, &QPushButton::clicked, this, [=]() {
        QString message = ui->messageInput->text().trimmed();
        if (!message.isEmpty()) {
            wsClient->sendMessage(currentChat, message);
            ui->messageInput->clear();

            // Mostrar solo si es chat privado para evitar duplicados
            if (currentChat != "~") {
                ui->textEdit_2->append("Tú para " + currentChat + ": " + message);
            }
        }
    });


    // Cambiar a chat privado al seleccionar un usuario
    connect(ui->userListWidget, &QListWidget::itemClicked, this, [=](QListWidgetItem *item) {
        QString selectedUser = item->text().split(" ").first();
        if (selectedUser != currentChat) {
            currentChat = selectedUser;
            ui->textEdit_2->clear();
            ui->textEdit_2->append("💬 Ahora estás chateando con: " + selectedUser);

            // Obtener historial del chat privado
            wsClient->getChatHistory(selectedUser);
        }
    });


    // Regresar al chat general
    connect(ui->generalChatButton, &QPushButton::clicked, this, [=]() {
        if (currentChat != "~") {
            currentChat = "~";
            ui->textEdit_2->clear();
            ui->textEdit_2->append("🌐 Has vuelto al chat general.");

            // Obtener historial del chat general
            wsClient->getChatHistory("~");
        }
    });



    // Cerrar la ventana si el nombre de usuario ya está conectado
    connect(wsClient, &WebSocketClient::connectionRejected, this, [=]() {
        QMessageBox::warning(this, "Error", "El nombre de usuario ya está en uso. La aplicación se cerrará.");
        QTimer::singleShot(1000, this, &MainWindow::close);  // Esperar 1 segundo antes de cerrar
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
    reply->deleteLater();  // Limpiar después de recibir respuesta
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (wsClient->isConnected()) {
        wsClient->onDisconnected();  // Llamar para notificar la desconexión
    }
    event->accept();  // Cerrar ventana correctamente
}

MainWindow::~MainWindow()
{
    delete ui;
}
