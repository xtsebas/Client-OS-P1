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

    connect(ui->statusComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStatusChanged);

    connect(ui->infoButton, &QPushButton::clicked, this, &MainWindow::onInfoButtonClicked);
    connect(ui->closeInfoButton, &QPushButton::clicked, this, &MainWindow::onCloseInfoButtonClicked);
    connect(ui->refreshInfoButton, &QPushButton::clicked, this, &MainWindow::onRefreshInfoButtonClicked);

    // Create a shortcut for sending messages with Enter
    QShortcut *sendShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onSendButtonClicked);

    // Set up the inactivity timer
    connect(m_inactivityTimer, &QTimer::timeout, this, &MainWindow::onInactivityTimeout);
    m_inactivityTimer->setInterval(300000); // 5 minutes by default

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
        connect(m_webSocketClient, &WebSocketClient::messageReceived, this, &MainWindow::onMessageReceived);
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

void MainWindow::onMessageReceived(const QString &sender, const QString &message)
{
    qDebug() << "Mensaje recibido de:" << sender << "- Mensaje:" << message;
    
    // Reset inactivity timer on any received message
    if (m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }

    // Mostrar mensajes SOLO si pertenecen al chat activo
    if (m_currentChat == "~") {
        // Chat general
        if (sender == m_currentUsername) {
            addChatMessage("Tú", message, MessageBubble::Sent);
        } else if (sender == "~") {
            addSystemMessage(message);
        } else {
            addChatMessage(sender, message, MessageBubble::Received);
        }
    }
    else if (m_currentChat == sender) {
        // Mensaje recibido de otro usuario en chat privado
        addChatMessage(sender, message, MessageBubble::Received);
    }
    else if (sender == m_currentUsername && m_currentChat != "~") {
        // Mensaje enviado por mí en chat privado
        addChatMessage("Tú", message, MessageBubble::Sent);
    }
    else {
        qDebug() << "Mensaje ignorado, no pertenece al chat actual.";
    }
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

    qDebug() << "Intentando enviar mensaje a:" << m_currentChat << "- Mensaje:" << message;

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

        // Clear input field
        ui->messageInput->clear();

        // Restaurar cursor
        QApplication::restoreOverrideCursor();
    } catch (const std::exception& e) {
        QApplication::restoreOverrideCursor(); // asegurarse de restaurar el cursor
        qDebug() << "Excepción al enviar mensaje:" << e.what();
        QMessageBox::critical(this, "Error", "Error al enviar mensaje: " + QString(e.what()));
    } catch (...) {
        QApplication::restoreOverrideCursor(); // asegurarse de restaurar el cursor
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
    if (m_currentChat == username) {
        qDebug() << "Ya estamos en el chat con" << username;
        return;
    }

    // Extract the username without status
    if (username.contains("(")) {
        int startPos = username.indexOf("(");
        username = username.left(startPos).trimmed();
        qDebug() << "Nombre de usuario extraído sin estado:" << username;
    }

    // Validar que el username no esté vacío
    if (username.isEmpty()) {
        qDebug() << "Error: nombre de usuario vacío";
        QMessageBox::warning(this, "Error", "No se pudo determinar el usuario seleccionado.");
        return;
    }

    // No cambiar de chat si ya estamos en ese chat
    if (m_currentChat == username) {
        qDebug() << "Ya estamos en el chat con" << username;
        return;
    }

    // Bloquear señales temporalmente
    ui->userListWidget->blockSignals(true);

    // Set the current chat
    m_currentChat = username;

    // Set the chat title
    ui->chatTitle->setText(username);

    // Set the chat status
    ui->chatStatus->setText(chatItem->status());

    // Clear the chat area and load messages for this user
    ui->messageDisplay->clear();

    // Add system message indicating private chat
    addSystemMessage("Chat privado con " + username);

    // Get chat history if connected
    if (m_connected && m_webSocketClient) {
        getChatHistory(username);
    } else {
        qDebug() << "Advertencia: No se puede obtener historial, no conectado";
    }

    // Reactivar señales
    ui->userListWidget->blockSignals(false);
}
void MainWindow::onBroadcastItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    qDebug() << "Cambiando a chat general";

    // Bloquear señales para evitar cambios múltiples
    ui->broadcastListWidget->blockSignals(true);

    // Set the current chat to general
    m_currentChat = "~";

    // Set the chat title
    ui->chatTitle->setText("General Chat");

    // Clear the chat status
    ui->chatStatus->clear();

    // Clear the chat area
    ui->messageDisplay->clear();
    
    // Add system message for general chat
    addSystemMessage("General Chat - Messages here are sent to all connected users");
    
    // Get chat history
    if (m_connected && m_webSocketClient) {
        getChatHistory("~");
    }

    // Desbloquear señales
    ui->broadcastListWidget->blockSignals(false);

    // Hide user info sidebar if visible
    ui->userInfoSidebar->hide();
    
    // Enable message input
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
    
    // Send status update via WebSocketClient
    m_webSocketClient->changeUserStatus(newStatus);

    // Update status bar
    ui->statusbar->showMessage("Status changed to " + m_currentStatus);
    
    // Set current avatar status
    updateUserAvatar();

    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == m_currentUsername) {
        showCurrentUserInfo();
    }
}

void MainWindow::onInfoButtonClicked()
{
    QString currentChatTitle = ui->chatTitle->text();

    // If in General Chat or no chat selected, show current user info
    if (currentChatTitle == "General Chat" || currentChatTitle == "Select a chat") {
        showCurrentUserInfo();
        return;
    }

    // If we're in a direct chat, show the chat partner's info
    ui->userInfoSidebar->setVisible(!ui->userInfoSidebar->isVisible());

    if (ui->userInfoSidebar->isVisible()) {
        // Set basic user info
        ui->userInfoName->setText(currentChatTitle);
        
        // Set avatar
        QString firstLetter = currentChatTitle.isEmpty() ? 
                             QString("?") : 
                             QString(currentChatTitle.at(0).toUpper());
        ui->userInfoAvatar->setText(firstLetter);
        
        // Generate avatar color
        int hash = 0;
        for (const QChar &c : currentChatTitle) {
            hash = ((hash << 5) - hash) + c.unicode();
        }
        
        QColor avatarColor;
        if (currentChatTitle.isEmpty()) {
            avatarColor = QColor("#128C7E"); // Default color
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
        
        // Set status based on UI
        for (int i = 0; i < ui->userListWidget->count(); ++i) {
            QListWidgetItem *item = ui->userListWidget->item(i);
            UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));
            
            if (chatItem && chatItem->username() == currentChatTitle) {
                QString status = chatItem->status();
                ui->userInfoStatusValue->setText(status);
                
                // Update status text color
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
        
        // For other users, we don't have their IP address from the server
        ui->userInfoIP->setText("N/A");
    }
}

void MainWindow::showCurrentUserInfo()
{
    // Show user info sidebar if it's not already visible
    ui->userInfoSidebar->setVisible(true);

    // Set current user info
    ui->userInfoName->setText(m_currentUsername);
    
    // Set avatar
    QString firstLetter = m_currentUsername.isEmpty() ? 
                         QString("?") : 
                         QString(m_currentUsername.at(0).toUpper());
    ui->userInfoAvatar->setText(firstLetter);
    
    // Generate avatar color based on username (same as updateUserAvatar)
    int hash = 0;
    for (const QChar &c : m_currentUsername) {
        hash = ((hash << 5) - hash) + c.unicode();
    }
    
    QColor avatarColor;
    if (m_currentUsername.isEmpty()) {
        avatarColor = QColor("#128C7E"); // Default color
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
    
    // Set status based on current user's status
    ui->userInfoStatusValue->setText(m_currentStatus);
    
    // Update status text color
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
    
    // Set IP address using the local IP address
    ui->userInfoIP->setText(getLocalIPAddress());
}

void MainWindow::onCloseInfoButtonClicked()
{
    ui->userInfoSidebar->hide();
}

void MainWindow::onRefreshInfoButtonClicked()
{
    // Currently no way to refresh user     info with this protocol
    // Keep as placeholder for future enhancement
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
    // Change status to INACTIVE after inactivity
    if (m_connected && m_webSocketClient && m_currentStatus != "INACTIVO") {
        //ui->statusComboBox->setCurrentIndex(2); // INACTIVO
    }
}

void MainWindow::onUserListReceived(const QStringList &users)
{
    qDebug() << "Lista de usuarios recibida:" << users;

    // Clear the user list
    ui->userListWidget->clear();

    // Add users to the list
    for (const QString &userWithStatus : users) {
        // Parse status from the user string
        QString username = userWithStatus;
        QString statusText = "ACTIVO"; // Default status

        // Extract username and status
        if (username.contains("(")) {
            int startPos = username.indexOf("(");
            int endPos = username.indexOf(")");

            if (startPos != -1 && endPos != -1) {
                statusText = username.mid(startPos + 1, endPos - startPos - 1).trimmed();
                username = username.left(startPos).trimmed();
            }
        }

        // Skip if it's the current user
        if (username == m_currentUsername)
            continue;

        // Skip if user is disconnected (case-insensitive)
        QString upperStatus = statusText.toUpper();
        if (upperStatus == "DESCONECTADO" || upperStatus == "DISCONNECTED" || upperStatus == "OFFLINE")
            continue;

        // Add to the UI
        QListWidgetItem *item = new QListWidgetItem(ui->userListWidget);
        item->setSizeHint(QSize(0, 70));

        UserChatItem *chatItem = new UserChatItem(username, statusText, "No messages yet");
        ui->userListWidget->setItemWidget(item, chatItem);
    }
}


void MainWindow::onUserStatusReceived(quint8 status)
{
    qDebug() << "Estado de usuario recibido:" << status;
    
    // Update the status dropdown to match server state
    switch (status) {
    case 0x01: // ACTIVO
        m_currentStatus = "ACTIVO";
        ui->statusComboBox->setCurrentIndex(0);
        break;
    case 0x02: // OCUPADO  
        m_currentStatus = "OCUPADO";
        ui->statusComboBox->setCurrentIndex(1);
        break;
    case 0x03: // INACTIVO
        m_currentStatus = "INACTIVO";
        ui->statusComboBox->setCurrentIndex(2);
        break;
    default:
        break;
    }
    
    // If user info sidebar is showing the current user, update it
    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == m_currentUsername) {
        showCurrentUserInfo();
    }
}

void MainWindow::updateUserAvatar()
{
    // Get first letter of username
    QString firstLetter = m_currentUsername.isEmpty() ? 
                         QString("?") : 
                         QString(m_currentUsername.at(0).toUpper());
    ui->userAvatar->setText(firstLetter);

    // Generate a color based on the username
    int hash = 0;
    for (const QChar &c : m_currentUsername) {
        hash = ((hash << 5) - hash) + c.unicode();
    }

    QColor avatarColor;
    if (m_currentUsername.isEmpty()) {
        avatarColor = QColor("#128C7E"); // Default color
    } else {
        // Generate a hue based on the hash
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
    qDebug() << "Solicitando historial de chat para:" << chatName;
    
    if (!m_connected || !m_webSocketClient) {
        qDebug() << "No se puede obtener historial: no conectado o cliente no inicializado";
        return;
    }
    
    // Validar el nombre del chat
    if (chatName.isEmpty()) {
        qDebug() << "Error: nombre de chat vacío";
        return;
    }
    
    // Clear current messages
    ui->messageDisplay->clear();
    
    try {
        // Request history from the WebSocketClient
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
    qDebug() << "Añadiendo mensaje al chat desde:" << sender << "tipo:" << type;
    
    // Create a MessageBubble widget for the message
    QDateTime timestamp = QDateTime::currentDateTime();
    MessageBubble *bubble = new MessageBubble(sender, message, timestamp, type);

    // Add the bubble to the chat display
    QTextBrowser *display = ui->messageDisplay;

    // Simplified approach: just add text directly to the text browser
    QString formattedText;
    
    if (type == MessageBubble::System) {
        formattedText = QString("<div style='text-align:center; margin:5px; padding:5px; background-color:#e1f3fb; border-radius:5px; color:#555;'><b>System:</b> %1</div>").arg(message);
    } else if (type == MessageBubble::Sent) {
        formattedText = QString("<div style='text-align:right; margin:5px;'>"
                                "<div style='display:inline-block; background-color:#dcf8c6; padding:8px; border-radius:10px; max-width:80%%; white-space: pre-wrap;'>"
                                "<b>%1:</b> %2<br><span style='font-size:10px; color:#888;'>%3</span>"
                                "</div></div>")
                            .arg(sender)
                            .arg(message.toHtmlEscaped())
                            .arg(timestamp.toString("hh:mm AP"));
    } else { // Received
        formattedText = QString("<div style='text-align:left; margin:5px;'>"
                                "<div style='display:inline-block; background-color:#ffffff; padding:8px; border-radius:10px; max-width:80%%; border: 1px solid #ddd; white-space: pre-wrap;'>"
                                "<b>%1:</b> %2<br><span style='font-size:10px; color:#888;'>%3</span>"
                                "</div></div>")
                            .arg(sender)
                            .arg(message.toHtmlEscaped())
                            .arg(timestamp.toString("hh:mm AP"));
    }
    
    display->append(formattedText);
    
    // Make sure we scroll to see the new message
    display->verticalScrollBar()->setValue(display->verticalScrollBar()->maximum());
}

void MainWindow::addSystemMessage(const QString &message)
{
    qDebug() << "Añadiendo mensaje de sistema:" << message;
    
    // Reutilizar el método addChatMessage con tipo System
    addChatMessage("System", message, MessageBubble::System);
}

void MainWindow::updateUserLastMessage(const QString &username, const QString &message)
{
    // Find the user in the list and update the last message
    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        QListWidgetItem *item = ui->userListWidget->item(i);
        UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));

        if (chatItem && chatItem->username() == username) {
            chatItem->setLastMessage(message);

            // Move this user to the top of the list
            QListWidgetItem *topItem = ui->userListWidget->takeItem(i);
            ui->userListWidget->insertItem(0, topItem);
            ui->userListWidget->setItemWidget(topItem, chatItem);
            break;
        }
    }
}

void MainWindow::loadDirectChatHistory(const QString &username)
{
    // This is now handled by getChatHistory and the WebSocketClient
    getChatHistory(username);
}

void MainWindow::loadBroadcastChatHistory()
{
    // This is now handled by getChatHistory and the WebSocketClient
    getChatHistory("~");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Ensure clean disconnection when closing the window
    if (m_connected && m_webSocketClient) {
        m_webSocketClient->onDisconnected();
    }
    event->accept();
}
