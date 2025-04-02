#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QDebug>
#include <QUrlQuery>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_webSocketClient(nullptr)
    , m_connected(false)
    , m_currentUsername("")
    , m_currentChat("~")
    , m_currentStatus("ACTIVO")
    , m_inactivityTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_requestedHistoryChat("~") // Inicializar con valor predeterminado
{
    ui->setupUi(this);
    ui->userAvatar->setCursor(Qt::PointingHandCursor);
    ui->userAvatar->installEventFilter(this);
    
    // Set window title
    setWindowTitle("Chat Application");

    // Initially hide the user info sidebar
    ui->userInfoSidebar->hide();

    // Set up UI connections
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::onConnectTriggered);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::onDisconnectTriggered);
    connect(ui->actionExit, &QAction::triggered, this, &QApplication::quit);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAboutTriggered);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::onHelpTriggered);

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(ui->messageInput, &QTextEdit::textChanged, this, &MainWindow::onMessageInputChanged);

    connect(ui->userListWidget, &QListWidget::itemClicked, this, &MainWindow::onUserItemClicked);
    connect(ui->broadcastListWidget, &QListWidget::itemClicked, this, &MainWindow::onBroadcastItemClicked);
    
    // Conexión para limpiar mensajes solicitada por el WebSocketClient
    connect(m_webSocketClient, &WebSocketClient::clearMessages, this, [=]() {
        ui->messageDisplay->clear();
        qDebug() << "MainWindow: Mensajes limpiados por solicitud del WebSocketClient";
    });

    connect(ui->statusComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStatusChanged);

    connect(ui->infoButton, &QPushButton::clicked, this, &MainWindow::onInfoButtonClicked);
    connect(ui->closeInfoButton, &QPushButton::clicked, this, &MainWindow::onCloseInfoButtonClicked);
    connect(ui->refreshInfoButton, &QPushButton::clicked, this, &MainWindow::onRefreshInfoButtonClicked);
    connect(m_webSocketClient, &WebSocketClient::userStatusChanged,
            this, &MainWindow::onExternalUserStatusChanged);

    // Create a shortcut for sending messages with Enter
    QShortcut *sendShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onSendButtonClicked);

    // Set up the inactivity timer
    connect(m_inactivityTimer, &QTimer::timeout, this, &MainWindow::onInactivityTimeout);
    m_inactivityTimer->setInterval(300000); // 5 minutes por defecto

    // Initial UI setup
    setupInitialUI();
}

MainWindow::~MainWindow()
{
    // Make sure we disconnect cleanly
    if (m_connected && m_webSocketClient) {
        m_webSocketClient->onDisconnected();
    }

    delete m_webSocketClient;
    delete ui;
}

void MainWindow::setupInitialUI()
{
    // Disable disconnect action initially
    ui->actionDisconnect->setEnabled(false);

    // Add General Chat to broadcast list
    QListWidgetItem *generalChatItem = new QListWidgetItem(ui->broadcastListWidget);
    generalChatItem->setSizeHint(QSize(0, 70));

    UserChatItem *generalChat = new UserChatItem("General Chat", "ACTIVO", "Broadcast messages to all users");
    ui->broadcastListWidget->setItemWidget(generalChatItem, generalChat);

    // Initialize message input
    ui->messageInput->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // Set placeholder text
    ui->messageDisplay->setPlaceholderText("Connect to a server to start chatting");

    // Hide user info sidebar
    ui->userInfoSidebar->hide();

    // Set status bar message
    ui->statusbar->showMessage("Not connected");
}

void MainWindow::onConnectTriggered()
{
    if (m_connected) {
        QMessageBox::information(this, "Already Connected",
                                 "You are already connected to a chat server.");
        return;
    }

    ConnectionDialog dialog(this);
    // Pre-populate with known working server address and port
    dialog.setServerAddress("18.224.60.241");
    dialog.setServerPort(18080);

    if (dialog.exec() == QDialog::Accepted) {
        m_currentUsername = dialog.username();

        // Create WebSocket connection URL
        QUrl url;
        url.setScheme("ws");
        url.setHost(dialog.server());
        url.setPort(dialog.port());
        url.setPath("/");

        // Update status bar
        ui->statusbar->showMessage("Connecting to server...");

        // Initialize WebSocketClient
        m_webSocketClient = new WebSocketClient(url, m_currentUsername, this);
        
        // Connect WebSocketClient signals to MainWindow slots
        connect(m_webSocketClient, &WebSocketClient::connected, this, &MainWindow::onWebSocketConnected);
        connect(m_webSocketClient, &WebSocketClient::disconnected, this, &MainWindow::onWebSocketDisconnected);
        // Nueva conexión para la señal con bandera
        connect(m_webSocketClient, &WebSocketClient::messageReceivedWithFlag, 
                this, &MainWindow::onMessageReceivedWithFlag);
        // Mantener la conexión existente para compatibilidad
        connect(m_webSocketClient, &WebSocketClient::messageReceived, 
                this, &MainWindow::onMessageReceived);
        connect(m_webSocketClient, &WebSocketClient::userListReceived, this, &MainWindow::onUserListReceived);
        connect(m_webSocketClient, &WebSocketClient::userStatusReceived, this, &MainWindow::onUserStatusReceived);
        connect(m_webSocketClient, &WebSocketClient::connectionRejected, this, [=]() {
            QMessageBox::warning(this, "Connection Rejected", 
                                "The username is already in use. Please try again with a different username.");
            m_webSocketClient->deleteLater();
            m_webSocketClient = nullptr;
            onDisconnectTriggered();
        });

        // Verify HTTP server
        QUrl httpUrl("http://" + dialog.server() + ":" + QString::number(dialog.port()) + "/");
        QNetworkRequest request(httpUrl);
        m_networkManager->get(request);
    }
}

void MainWindow::onDisconnectTriggered()
{
    if (!m_connected) {
        QMessageBox::information(this, "Not Connected",
                                 "You are not connected to any chat server.");
        return;
    }

    // Close the WebSocket connection
    if (m_webSocketClient) {
        m_webSocketClient->onDisconnected();
        m_webSocketClient->deleteLater();
        m_webSocketClient = nullptr;
    }

    // Update UI for disconnected state
    m_connected = false;
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->messageInput->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // Clear user lists
    ui->userListWidget->clear();

    // Clear chat area
    ui->messageDisplay->clear();
    ui->chatTitle->setText("Select a chat");
    ui->chatStatus->clear();

    // Stop inactivity timer
    m_inactivityTimer->stop();

    // Hide user info sidebar if visible
    ui->userInfoSidebar->hide();

    // Update status bar
    ui->statusbar->showMessage("Disconnected from server");

    // Add system message to chat
    addSystemMessage("Disconnected from server.");
}

void MainWindow::onWebSocketConnected()
{
    qDebug() << "WebSocket conectado con éxito";
    
    m_connected = true;

    // Update UI for connected state
    ui->actionConnect->setEnabled(false);
    ui->actionDisconnect->setEnabled(true);
    ui->messageInput->setEnabled(true);
    ui->sendButton->setEnabled(true);

    // Update user profile
    ui->currentUsername->setText(m_currentUsername);
    updateUserAvatar();

    // Set default status to ACTIVE
    ui->statusComboBox->setCurrentIndex(0); // ACTIVO
    m_currentStatus = "ACTIVO";

    // Update status bar
    ui->statusbar->showMessage("Connected to server");

    // Start inactivity timer
    m_inactivityTimer->start();

    // Set the current chat to general chat
    m_currentChat = "~";

    // Show the general broadcast chat
    ui->chatTabs->setCurrentIndex(1); // Broadcast tab
    
    // Clear the message display first
    ui->messageDisplay->clear();
    
    // Show the general broadcast chat
    if (ui->broadcastListWidget->count() > 0) {
        onBroadcastItemClicked(ui->broadcastListWidget->item(0));
    }

    // Add system message to chat
    addSystemMessage("Connected to server. You can now chat with other users.");

    if (ui->userInfoSidebar->isVisible()) {
        showCurrentUserInfo();
    }
}

void MainWindow::onWebSocketDisconnected()
{
    // Already handled in onDisconnectTriggered
    if (m_connected) {
        onDisconnectTriggered();
    }
}

// NUEVO MÉTODO: Procesa el mensaje con bandera que indica si es historial o no
void MainWindow::onMessageReceivedWithFlag(const QString &sender, const QString &message, bool isHistory)
{
    qDebug() << "DEBUG - onMessageReceivedWithFlag: Emisor=" << sender 
             << "Mensaje=" << message.left(30) 
             << "Es Historial=" << isHistory;
    
    // Restablecer el temporizador de inactividad
    if (m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }

    // CASO 1: Mensaje del sistema (desde ~)
    if (sender == "~") {
        qDebug() << "DEBUG - Tipo identificado: MENSAJE DEL SISTEMA";
        addSystemMessage(message);
        return;
    }
    
    // CASO 2: Es un mensaje que YO he enviado
    else if (sender == m_currentUsername) {
        qDebug() << "DEBUG - Tipo identificado: MENSAJE ENVIADO POR MÍ";
        
        // Si es un mensaje del historial
        if (isHistory) {
            qDebug() << "DEBUG - Procesando mensaje propio del historial";
            qDebug() << "DEBUG - Chat actual:" << m_currentChat 
                     << "- Chat solicitado:" << m_requestedHistoryChat;
            
            // Para historial de chat general
            if (m_requestedHistoryChat == "~" && m_currentChat == "~") {
                addChatMessage("Tú", message, MessageBubble::Sent);
                return;
            }
            
            // Para historial de chat directo: mostramos nuestros mensajes del historial
            // solo cuando estamos en el chat correcto
            if (m_requestedHistoryChat != "~" && m_currentChat == m_requestedHistoryChat) {
                addChatMessage("Tú", message, MessageBubble::Sent);
                return;
            }
            
            // Si no corresponde al chat actual, se ignora
            qDebug() << "DEBUG - Ignorando mensaje, no pertenece al chat actual";
            return;
        } 
        else {
            // No es mensaje del historial, se ignora para evitar duplicados
            return;
        }
    }
    
    // CASO 3: Mensaje recibido de otro usuario
    else {
        qDebug() << "DEBUG - Tipo identificado: MENSAJE RECIBIDO DE OTRO";
        
        if (isHistory) {
            // Para historial de chat general
            if (m_requestedHistoryChat == "~" && m_currentChat == "~") {
                addChatMessage(sender, message, MessageBubble::Received);
                return;
            }
            
            // Para historial de chat directo: mostramos solo si el remitente coincide con el chat solicitado
            if (m_requestedHistoryChat != "~" && sender == m_requestedHistoryChat && 
                m_currentChat == m_requestedHistoryChat) {
                addChatMessage(sender, message, MessageBubble::Received);
                return;
            }
            
            // Si no coincide, se ignora
            qDebug() << "DEBUG - Ignorando mensaje recibido, no pertenece al chat solicitado";
            return;
        }
        // Mensaje normal (no historial)
        else {
            // Mostrar el mensaje solo si se trata del chat general o privado con este remitente
            if (m_currentChat == "~" || m_currentChat == sender) {
                addChatMessage(sender, message, MessageBubble::Received);
            }
        }
    }
}

// Método existente: delega al nuevo método con bandera = false
void MainWindow::onMessageReceived(const QString &sender, const QString &message)
{
    onMessageReceivedWithFlag(sender, message, false);
}

void MainWindow::onSendButtonClicked()
{
    if (!m_connected || !m_webSocketClient) {
        QMessageBox::information(this, "Not Connected",
                                 "You must connect to a server before sending messages.");
        return;
    }

    QString message = ui->messageInput->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    qDebug() << "DEBUG - onSendButtonClicked: Enviando mensaje a:" << m_currentChat << "- Mensaje:" << message;

    // Validar el destinatario
    if (m_currentChat.isEmpty()) {
        qDebug() << "Error: destinatario vacío";
        QMessageBox::warning(this, "Error", "No se ha seleccionado un destinatario válido.");
        return;
    }

    try {
        // Reset inactivity timer on sending a message
        if (m_inactivityTimer->isActive()) {
            m_inactivityTimer->start();
        }

        // Asegurarse de que no se actualice la UI durante el envío para evitar posibles crashes
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // Send the message using WebSocketClient with REAL username (NOT "Tú")
        m_webSocketClient->sendMessage(m_currentChat, message);
        
        // Mostrar mensaje localmente inmediatamente
        addChatMessage("Tú", message, MessageBubble::Sent);

        // Clear input field
        ui->messageInput->clear();

        // Restaurar cursor
        QApplication::restoreOverrideCursor();
    } catch (const std::exception& e) {
        QApplication::restoreOverrideCursor();
        qDebug() << "Excepción al enviar mensaje:" << e.what();
        QMessageBox::critical(this, "Error", "Error al enviar mensaje: " + QString(e.what()));
    } catch (...) {
        QApplication::restoreOverrideCursor();
        qDebug() << "Error desconocido al enviar mensaje";
        QMessageBox::critical(this, "Error", "Error desconocido al enviar mensaje.");
    }
}

void MainWindow::onMessageInputChanged()
{
    // Reset inactivity timer when typing
    if (m_inactivityTimer->isActive() && m_currentStatus != "INACTIVO") {
        m_inactivityTimer->start();
    }
}

void MainWindow::onUserItemClicked(QListWidgetItem *item)
{
    if (!item) {
        qDebug() << "Error: item nulo seleccionado";
        return;
    }

    // Get the username from the item
    UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));
    if (!chatItem) {
        qDebug() << "Error: no se pudo obtener UserChatItem del item";
        return;
    }

    QString username = chatItem->username();
    
    qDebug() << "DEBUG - onUserItemClicked: Usuario seleccionado=" << username;
    
    if (m_currentChat == username) {
        qDebug() << "Ya estamos en el chat con" << username;
        return;
    }

    if (username.contains("(")) {
        int startPos = username.indexOf("(");
        username = username.left(startPos).trimmed();
        qDebug() << "DEBUG - Nombre de usuario extraído sin estado:" << username;
    }

    if (username.isEmpty()) {
        qDebug() << "Error: nombre de usuario vacío";
        QMessageBox::warning(this, "Error", "No se pudo determinar el usuario seleccionado.");
        return;
    }

    if (m_currentChat == username) {
        qDebug() << "Ya estamos en el chat con" << username;
        return;
    }

    ui->userListWidget->blockSignals(true);
    m_currentChat = username;
    qDebug() << "DEBUG - Cambiando chat actual a: " << m_currentChat;
    ui->chatTitle->setText(username);
    ui->chatStatus->setText(chatItem->status());
    
    // Limpiar el área de chat ANTES de solicitar historial
    ui->messageDisplay->clear();
    
    // Agregar mensaje de sistema indicando chat privado
    addSystemMessage("Chat privado con " + username);

    // Solicitar historial de chat (esto actualizará m_requestedHistoryChat)
    if (m_connected && m_webSocketClient) {
        getChatHistory(username);
    } else {
        qDebug() << "Advertencia: No se puede obtener historial, no conectado";
    }
    
    ui->userListWidget->blockSignals(false);
    ui->messageInput->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->messageInput->setFocus();
}

void MainWindow::clearMessageDisplay()
{
    ui->messageDisplay->clear();
    qDebug() << "MainWindow: Mensajes limpiados";
}

void MainWindow::onBroadcastItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    qDebug() << "Cambiando a chat general";

    ui->broadcastListWidget->blockSignals(true);
    m_currentChat = "~";
    ui->chatTitle->setText("General Chat");
    ui->chatStatus->clear();
    ui->messageDisplay->clear();
    addSystemMessage("General Chat - Messages here are sent to all connected users");
    
    if (m_connected && m_webSocketClient) {
        getChatHistory("~");
    }

    ui->broadcastListWidget->blockSignals(false);
    ui->userInfoSidebar->hide();
    ui->messageInput->setEnabled(m_connected);
    ui->sendButton->setEnabled(m_connected);
}

void MainWindow::onStatusChanged(int index)
{
    if (!m_connected || !m_webSocketClient) {
        qDebug() << "No se puede cambiar el estado: no conectado o cliente no inicializado";
        return;
    }

    quint8 newStatus;
    switch (index) {
    case 0: newStatus = 0x01; m_currentStatus = "ACTIVO"; break;
    case 1: newStatus = 0x02; m_currentStatus = "OCUPADO"; break;
    case 2: newStatus = 0x03; m_currentStatus = "INACTIVO"; break;
    default: newStatus = 0x01; m_currentStatus = "ACTIVO"; break;
    }

    qDebug() << "Intentando cambiar el estado a:" << m_currentStatus << "(" << newStatus << ")";

    if (newStatus != 0x03 && m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }
    
    m_webSocketClient->changeUserStatus(newStatus);
    ui->statusbar->showMessage("Status changed to " + m_currentStatus);
    updateUserAvatar();

    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == m_currentUsername) {
        showCurrentUserInfo();
    }
}

void MainWindow::onInfoButtonClicked()
{
    QString currentChatTitle = ui->chatTitle->text();

    if (currentChatTitle == "General Chat" || currentChatTitle == "Select a chat") {
        showCurrentUserInfo();
        return;
    }

    ui->userInfoSidebar->setVisible(!ui->userInfoSidebar->isVisible());

    if (ui->userInfoSidebar->isVisible()) {
        ui->userInfoName->setText(currentChatTitle);
        
        QString firstLetter = currentChatTitle.isEmpty() ? 
                             QString("?") : 
                             QString(currentChatTitle.at(0).toUpper());
        ui->userInfoAvatar->setText(firstLetter);
        
        int hash = 0;
        for (const QChar &c : currentChatTitle) {
            hash = ((hash << 5) - hash) + c.unicode();
        }
        
        QColor avatarColor;
        if (currentChatTitle.isEmpty()) {
            avatarColor = QColor("#128C7E");
        } else {
            int hue = qAbs(hash) % 360;
            avatarColor = QColor::fromHsv(hue, 200, 200);
        }
        
        ui->userInfoAvatar->setStyleSheet(QString("QLabel {"
                                                 "background-color: %1;"
                                                 "border-radius: 50px;"
                                                 "color: white;"
                                                 "font-weight: bold;"
                                                 "font-size: 36px;"
                                                 "}").arg(avatarColor.name()));
        
        for (int i = 0; i < ui->userListWidget->count(); ++i) {
            QListWidgetItem *item = ui->userListWidget->item(i);
            UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));
            
            if (chatItem && chatItem->username() == currentChatTitle) {
                QString status = chatItem->status();
                ui->userInfoStatusValue->setText(status);
                
                if (status == "ACTIVO") {
                    ui->userInfoStatus->setText("Active");
                    ui->userInfoStatus->setStyleSheet("color: #2ecc71;");
                } else if (status == "OCUPADO") {
                    ui->userInfoStatus->setText("Busy");
                    ui->userInfoStatus->setStyleSheet("color: #e74c3c;");
                } else if (status == "INACTIVO") {
                    ui->userInfoStatus->setText("Inactive");
                    ui->userInfoStatus->setStyleSheet("color: #f1c40f;");
                } else {
                    ui->userInfoStatus->setText(status);
                    ui->userInfoStatus->setStyleSheet("color: #95a5a6;");
                }
                
                break;
            }
        }
        
        ui->userInfoIP->setText("N/A");
    }
}

void MainWindow::showCurrentUserInfo()
{
    ui->userInfoSidebar->setVisible(true);
    ui->userInfoName->setText(m_currentUsername);
    
    QString firstLetter = m_currentUsername.isEmpty() ? 
                         QString("?") : 
                         QString(m_currentUsername.at(0).toUpper());
    ui->userInfoAvatar->setText(firstLetter);
    
    int hash = 0;
    for (const QChar &c : m_currentUsername) {
        hash = ((hash << 5) - hash) + c.unicode();
    }
    
    QColor avatarColor;
    if (m_currentUsername.isEmpty()) {
        avatarColor = QColor("#128C7E");
    } else {
        int hue = qAbs(hash) % 360;
        avatarColor = QColor::fromHsv(hue, 200, 200);
    }
    
    ui->userInfoAvatar->setStyleSheet(QString("QLabel {"
                                             "background-color: %1;"
                                             "border-radius: 50px;"
                                             "color: white;"
                                             "font-weight: bold;"
                                             "font-size: 36px;"
                                             "}").arg(avatarColor.name()));
    
    ui->userInfoStatusValue->setText(m_currentStatus);
    
    if (m_currentStatus == "ACTIVO") {
        ui->userInfoStatus->setText("Active");
        ui->userInfoStatus->setStyleSheet("color: #2ecc71;");
    } else if (m_currentStatus == "OCUPADO") {
        ui->userInfoStatus->setText("Busy");
        ui->userInfoStatus->setStyleSheet("color: #e74c3c;");
    } else if (m_currentStatus == "INACTIVO") {
        ui->userInfoStatus->setText("Inactive");
        ui->userInfoStatus->setStyleSheet("color: #f1c40f;");
    } else {
        ui->userInfoStatus->setText(m_currentStatus);
        ui->userInfoStatus->setStyleSheet("color: #95a5a6;");
    }
    
    ui->userInfoIP->setText(getLocalIPAddress());
}

void MainWindow::onCloseInfoButtonClicked()
{
    ui->userInfoSidebar->hide();
}

void MainWindow::onRefreshInfoButtonClicked()
{
    // Actualmente no hay forma de refrescar la información de usuario
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->userAvatar && event->type() == QEvent::MouseButtonPress) {
        showCurrentUserInfo();
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onAboutTriggered()
{
    QMessageBox::about(this, "About Chat Application",
                       "Chat Application\nVersion 1.0\n\n"
                       "A simple chat client for the Operating Systems class project.\n"
                       "Universidad del Valle de Guatemala");
}

void MainWindow::onHelpTriggered()
{
    QMessageBox::information(this, "Help",
                             "Chat Application Help\n\n"
                             "- Connect to a server using File -> Connect\n"
                             "- Send messages to all users in the 'Broadcast' tab\n"
                             "- Send private messages by selecting a user in the 'Direct' tab\n"
                             "- Change your status using the dropdown in your profile\n"
                             "- View user information by clicking the info button\n"
                             "- Disconnect using File -> Disconnect\n");
}

void MainWindow::onInactivityTimeout()
{
    if (m_connected && m_webSocketClient && m_currentStatus != "INACTIVO") {
        //ui->statusComboBox->setCurrentIndex(2); // INACTIVO
    }
}

void MainWindow::onUserListReceived(const QStringList &users)
{
    qDebug() << "Lista de usuarios recibida:" << users;
    ui->userListWidget->clear();

    for (const QString &userWithStatus : users) {
        QString username = userWithStatus;
        QString statusText = "ACTIVO";

        if (username.contains("(")) {
            int startPos = username.indexOf("(");
            int endPos = username.indexOf(")");

            if (startPos != -1 && endPos != -1) {
                statusText = username.mid(startPos + 1, endPos - startPos - 1).trimmed();
                username = username.left(startPos).trimmed();
            }
        }

        if (username == m_currentUsername)
            continue;

        QString upperStatus = statusText.toUpper();
        if (upperStatus == "DESCONECTADO" || upperStatus == "DISCONNECTED" || upperStatus == "OFFLINE")
            continue;

        QListWidgetItem *item = new QListWidgetItem(ui->userListWidget);
        item->setSizeHint(QSize(0, 70));

        UserChatItem *chatItem = new UserChatItem(username, statusText, "No messages yet");
        ui->userListWidget->setItemWidget(item, chatItem);
    }
}

void MainWindow::onUserStatusReceived(quint8 status)
{
    qDebug() << "Estado de usuario recibido:" << status;
    
    switch (status) {
    case 0x01:
        m_currentStatus = "ACTIVO";
        ui->statusComboBox->setCurrentIndex(0);
        break;
    case 0x02:
        m_currentStatus = "OCUPADO";
        ui->statusComboBox->setCurrentIndex(1);
        break;
    case 0x03:
        m_currentStatus = "INACTIVO";
        ui->statusComboBox->setCurrentIndex(2);
        break;
    default:
        break;
    }
    
    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == m_currentUsername) {
        showCurrentUserInfo();
    }
}

void MainWindow::updateUserAvatar()
{
    QString firstLetter = m_currentUsername.isEmpty() ? 
                         QString("?") : 
                         QString(m_currentUsername.at(0).toUpper());
    ui->userAvatar->setText(firstLetter);

    int hash = 0;
    for (const QChar &c : m_currentUsername) {
        hash = ((hash << 5) - hash) + c.unicode();
    }

    QColor avatarColor;
    if (m_currentUsername.isEmpty()) {
        avatarColor = QColor("#128C7E");
    } else {
        int hue = qAbs(hash) % 360;
        avatarColor = QColor::fromHsv(hue, 200, 200);
    }

    ui->userAvatar->setStyleSheet(QString("QLabel {"
                                          "background-color: %1;"
                                          "border-radius: 30px;"
                                          "color: white;"
                                          "font-weight: bold;"
                                          "font-size: 24px;"
                                          "}").arg(avatarColor.name()));
}

void MainWindow::getChatHistory(const QString &chatName)
{
    qDebug() << "DEBUG - Solicitando historial de chat para:" << chatName;
    
    if (!m_connected || !m_webSocketClient) {
        qDebug() << "No se puede obtener historial: no conectado o cliente no inicializado";
        return;
    }
    
    if (chatName.isEmpty()) {
        qDebug() << "Error: nombre de chat vacío";
        return;
    }
    
    // Guardar para qué chat se está solicitando el historial
    m_requestedHistoryChat = chatName;
    qDebug() << "DEBUG - Guardando chat solicitado para historial:" << m_requestedHistoryChat;
    
    // Mostrar mensaje de carga y limpiar el área de mensajes
    addSystemMessage("Cargando historial de mensajes...");
    ui->messageDisplay->clear();
    
    try {
        // Solicitar el historial al WebSocketClient
        m_webSocketClient->getChatHistory(chatName);
    } catch (const std::exception& e) {
        qDebug() << "Excepción al obtener historial:" << e.what();
        addSystemMessage("Error al obtener historial: " + QString(e.what()));
    } catch (...) {
        qDebug() << "Error desconocido al obtener historial";
        addSystemMessage("Error desconocido al obtener historial del chat.");
    }
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

void MainWindow::addChatMessage(const QString &sender, const QString &message, MessageBubble::MessageType type)
{
    qDebug() << "DEBUG - addChatMessage: Emisor=" << sender 
             << "Tipo=" << (type == MessageBubble::System ? "Sistema" : 
                           (type == MessageBubble::Sent ? "Enviado" : "Recibido"));
    
    QTextBrowser *display = ui->messageDisplay;
    QDateTime timestamp = QDateTime::currentDateTime();
    
    QString html;
    
    switch (type) {
        case MessageBubble::System:
            html = QString(
                "<table width='100%' cellspacing='0' cellpadding='0' border='0'>"
                "<tr><td align='center'>"
                "<div style='display:inline-block; background-color:#e1f3fb; color:#555; "
                "border-radius:5px; padding:8px; margin:5px; max-width:80%;'>"
                "<b>Sistema:</b> %1"
                "</div>"
                "</td></tr>"
                "</table>"
            ).arg(message.toHtmlEscaped());
            break;
            
        case MessageBubble::Sent:
            html = QString(
                "<table width='100%' cellspacing='0' cellpadding='0' border='0'>"
                "<tr><td align='right'>"
                "<div style='display:inline-block; background-color:#dcf8c6; color:#000; "
                "border-radius:10px; padding:8px; margin:5px; max-width:80%;'>"
                "<b>%1:</b> %2<br>"
                "<span style='font-size:10px; color:#888;'>%3</span>"
                "</div>"
                "</td></tr>"
                "</table>"
            ).arg(sender).arg(message.toHtmlEscaped()).arg(timestamp.toString("hh:mm AP"));
            break;
            
        case MessageBubble::Received:
        default:
            html = QString(
                "<table width='100%' cellspacing='0' cellpadding='0' border='0'>"
                "<tr><td align='left'>"
                "<div style='display:inline-block; background-color:#ffffff; color:#000; "
                "border:1px solid #ddd; border-radius:10px; padding:8px; margin:5px; max-width:80%;'>"
                "<b>%1:</b> %2<br>"
                "<span style='font-size:10px; color:#888;'>%3</span>"
                "</div>"
                "</td></tr>"
                "</table>"
            ).arg(sender).arg(message.toHtmlEscaped()).arg(timestamp.toString("hh:mm AP"));
            break;
    }
    
    qDebug() << "DEBUG - HTML generado:" << html.left(150) << "...";
    display->append(html);
    display->verticalScrollBar()->setValue(display->verticalScrollBar()->maximum());
}

void MainWindow::addSystemMessage(const QString &message)
{
    qDebug() << "DEBUG - addSystemMessage:" << message.left(50);
    
    QString html = QString(
        "<table width='100%' cellspacing='0' cellpadding='0' border='0'>"
        "<tr><td align='center'>"
        "<div style='display:inline-block; background-color:#e1f3fb; color:#555; "
        "border-radius:5px; padding:8px; margin:5px; max-width:80%;'>"
        "<b>Sistema:</b> %1"
        "</div>"
        "</td></tr>"
        "</table>"
    ).arg(message.toHtmlEscaped());
    
    ui->messageDisplay->append(html);
    ui->messageDisplay->verticalScrollBar()->setValue(ui->messageDisplay->verticalScrollBar()->maximum());
}

void MainWindow::updateUserLastMessage(const QString &username, const QString &message)
{
    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        QListWidgetItem *item = ui->userListWidget->item(i);
        UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));

        if (chatItem && chatItem->username() == username) {
            chatItem->setLastMessage(message);
            QListWidgetItem *topItem = ui->userListWidget->takeItem(i);
            ui->userListWidget->insertItem(0, topItem);
            ui->userListWidget->setItemWidget(topItem, chatItem);
            break;
        }
    }
}

void MainWindow::onExternalUserStatusChanged(const QString& username, quint8 newStatus)
{
    qDebug() << "Cambio de estado detectado: " << username << " -> " << newStatus;

    QString newStatusText;
    switch (newStatus) {
    case 0x01: newStatusText = "ACTIVO"; break;
    case 0x02: newStatusText = "OCUPADO"; break;
    case 0x03: newStatusText = "INACTIVO"; break;
    default: newStatusText = "DESCONOCIDO"; break;
    }

    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        QListWidgetItem *item = ui->userListWidget->item(i);
        UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));

        if (chatItem && chatItem->username() == username) {
            chatItem->setStatus(newStatusText);
            break;
        }
    }

    // Si el sidebar está mostrando a ese usuario, actualiza también ahí
    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == username) {
        ui->userInfoStatusValue->setText(newStatusText);

        if (newStatusText == "ACTIVO") {
            ui->userInfoStatus->setText("Active");
            ui->userInfoStatus->setStyleSheet("color: #2ecc71;");
        } else if (newStatusText == "OCUPADO") {
            ui->userInfoStatus->setText("Busy");
            ui->userInfoStatus->setStyleSheet("color: #e74c3c;");
        } else if (newStatusText == "INACTIVO") {
            ui->userInfoStatus->setText("Inactive");
            ui->userInfoStatus->setStyleSheet("color: #f1c40f;");
        } else {
            ui->userInfoStatus->setText(newStatusText);
            ui->userInfoStatus->setStyleSheet("color: #95a5a6;");
        }
    }
}


void MainWindow::loadDirectChatHistory(const QString &username)
{
    getChatHistory(username);
}

void MainWindow::loadBroadcastChatHistory()
{
    getChatHistory("~");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_connected && m_webSocketClient) {
        m_webSocketClient->onDisconnected();
    }
    event->accept();
}
