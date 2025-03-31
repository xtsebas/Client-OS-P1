#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_webSocket(new QWebSocket())
    , m_connected(false)
    , m_currentUsername("")
    , m_currentStatus("ACTIVO")
    , m_inactivityTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Initially hide the user info sidebar
    ui->userInfoSidebar->hide();

    // Set up WebSocket connections
    connect(m_webSocket, &QWebSocket::connected, this, &MainWindow::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &MainWindow::onWebSocketDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &MainWindow::onWebSocketError);

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
    if (m_connected) {
        m_webSocket->close();
    }

    delete m_webSocket;
    delete ui;
}

void MainWindow::setupInitialUI()
{
    // Set window title
    setWindowTitle("Chat Application");

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
    if (dialog.exec() == QDialog::Accepted) {
        m_currentUsername = dialog.username();

        // Create WebSocket connection URL
        QString url = QString("ws://%1:%2?name=%3")
                          .arg(dialog.server())
                          .arg(dialog.port())
                          .arg(m_currentUsername);

        // Update status bar
        ui->statusbar->showMessage("Connecting to server...");

        // Connect to server
        m_webSocket->open(QUrl(url));
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
    m_webSocket->close();
}

void MainWindow::onWebSocketConnected()
{
    m_connected = true;

    // Update UI for connected state
    ui->actionConnect->setEnabled(false);
    ui->actionDisconnect->setEnabled(true);

    // Update user profile
    ui->currentUsername->setText(m_currentUsername);
    updateUserAvatar();

    // Set default status to ACTIVE
    ui->statusComboBox->setCurrentIndex(0); // ACTIVO

    // Update status bar
    ui->statusbar->showMessage("Connected to server");

    // Start inactivity timer
    m_inactivityTimer->start();

    // Request user list
    requestUserList();

    // Show the general broadcast chat
    ui->chatTabs->setCurrentIndex(1); // Broadcast tab
    onBroadcastItemClicked(ui->broadcastListWidget->item(0));

    // Add system message to chat
    addSystemMessage("Connected to server. You can now chat with other users.");
}

void MainWindow::onWebSocketDisconnected()
{
    m_connected = false;

    // Update UI for disconnected state
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);

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

void MainWindow::onWebSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    // Update status bar
    ui->statusbar->showMessage("Connection error: " + m_webSocket->errorString());

    // Show error message
    QMessageBox::critical(this, "Connection Error",
                          "Failed to connect to server: " + m_webSocket->errorString());
}

void MainWindow::onTextMessageReceived(const QString &message)
{
    // Reset inactivity timer on any received message
    if (m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }

    // Parse the message as JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "Received invalid JSON message";
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "userList") {
        // Handle user list response
        handleUserListResponse(obj);
    } else if (type == "userInfo") {
        // Handle user info response
        handleUserInfoResponse(obj);
    } else if (type == "statusUpdate") {
        // Handle status update
        handleStatusUpdateResponse(obj);
    } else if (type == "message") {
        // Handle chat message
        handleChatMessage(obj);
    } else if (type == "broadcastMessage") {
        // Handle broadcast message
        handleBroadcastMessage(obj);
    } else if (type == "error") {
        // Handle error message
        handleErrorMessage(obj);
    }
}

void MainWindow::onSendButtonClicked()
{
    if (!m_connected) {
        QMessageBox::information(this, "Not Connected",
                                 "You must connect to a server before sending messages.");
        return;
    }

    QString message = ui->messageInput->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Reset inactivity timer on sending a message
    if (m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }

    QJsonObject msgObj;
    QString currentChatTitle = ui->chatTitle->text();

    if (currentChatTitle == "General Chat") {
        // Send broadcast message
        msgObj["type"] = "broadcastMessage";
        msgObj["message"] = message;
    } else {
        // Send direct message
        msgObj["type"] = "message";
        msgObj["to"] = currentChatTitle;
        msgObj["message"] = message;
    }

    // Send the message
    m_webSocket->sendTextMessage(QJsonDocument(msgObj).toJson());

    // Add the message to the chat display
    if (currentChatTitle == "General Chat") {
        // Add to broadcast chat
        addChatMessage(m_currentUsername, message, MessageBubble::Sent);
    } else {
        // Add to direct chat
        addChatMessage(m_currentUsername, message, MessageBubble::Sent);

        // Update last message in the user list
        updateUserLastMessage(currentChatTitle, "You: " + message);
    }

    // Clear input field
    ui->messageInput->clear();
}

void MainWindow::onMessageInputChanged()
{
    // Reset inactivity timer when typing
    if (m_inactivityTimer->isActive()) {
        m_inactivityTimer->start();
    }
}

void MainWindow::onUserItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    // Get the username from the item
    UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));
    if (!chatItem) return;

    QString username = chatItem->username();

    // Set the chat title
    ui->chatTitle->setText(username);

    // Set the chat status
    ui->chatStatus->setText(chatItem->status());

    // Clear the chat area and load messages for this user
    ui->messageDisplay->clear();
    loadDirectChatHistory(username);

    // Request user info
    requestUserInfo(username);
}

void MainWindow::onBroadcastItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    // Set the chat title
    ui->chatTitle->setText("General Chat");

    // Clear the chat status
    ui->chatStatus->clear();

    // Clear the chat area and load broadcast history
    ui->messageDisplay->clear();
    loadBroadcastChatHistory();

    // Hide user info sidebar if visible
    ui->userInfoSidebar->hide();
}

void MainWindow::onStatusChanged(int index)
{
    if (!m_connected) return;

    QString newStatus;
    switch (index) {
    case 0: newStatus = "ACTIVO"; break;
    case 1: newStatus = "OCUPADO"; break;
    case 2: newStatus = "INACTIVO"; break;
    default: newStatus = "ACTIVO"; break;
    }

    // Only send update if status changed
    if (m_currentStatus != newStatus) {
        m_currentStatus = newStatus;

        // Send status update request
        QJsonObject obj;
        obj["type"] = "statusUpdate";
        obj["status"] = m_currentStatus;

        m_webSocket->sendTextMessage(QJsonDocument(obj).toJson());

        // Update status bar
        ui->statusbar->showMessage("Status changed to " + m_currentStatus);
    }
}

void MainWindow::onInfoButtonClicked()
{
    QString currentChatTitle = ui->chatTitle->text();

    // Only show user info for direct chats
    if (currentChatTitle != "General Chat" && currentChatTitle != "Select a chat") {
        // Show/hide the user info sidebar
        ui->userInfoSidebar->setVisible(!ui->userInfoSidebar->isVisible());

        if (ui->userInfoSidebar->isVisible()) {
            // Request user info
            requestUserInfo(currentChatTitle);
        }
    }
}

void MainWindow::onCloseInfoButtonClicked()
{
    ui->userInfoSidebar->hide();
}

void MainWindow::onRefreshInfoButtonClicked()
{
    QString username = ui->userInfoName->text();
    if (username != "User Information") {
        requestUserInfo(username);
    }
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
    if (m_currentStatus != "INACTIVO") {
        ui->statusComboBox->setCurrentIndex(2); // INACTIVO
    }
}

void MainWindow::updateUserAvatar()
{
    // Get first letter of username
    QString firstLetter = m_currentUsername.isEmpty() ? QString("?") : QString(m_currentUsername.at(0).toUpper());
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

void MainWindow::requestUserList()
{
    if (!m_connected) return;

    QJsonObject obj;
    obj["type"] = "getUserList";

    m_webSocket->sendTextMessage(QJsonDocument(obj).toJson());
}

void MainWindow::requestUserInfo(const QString &username)
{
    if (!m_connected) return;

    QJsonObject obj;
    obj["type"] = "getUserInfo";
    obj["username"] = username;

    m_webSocket->sendTextMessage(QJsonDocument(obj).toJson());
}

void MainWindow::handleUserListResponse(const QJsonObject &obj)
{
    QJsonArray users = obj["users"].toArray();

    // Clear the user list
    ui->userListWidget->clear();

    // Add users to the list
    for (const QJsonValue &val : users) {
        QJsonObject user = val.toObject();
        QString username = user["username"].toString();
        QString status = user["status"].toString();

        // Skip the current user
        if (username == m_currentUsername) continue;

        QListWidgetItem *item = new QListWidgetItem(ui->userListWidget);
        item->setSizeHint(QSize(0, 70));

        UserChatItem *chatItem = new UserChatItem(username, status, "No messages yet");
        ui->userListWidget->setItemWidget(item, chatItem);
    }

    // Enable message input if connected
    ui->messageInput->setEnabled(m_connected);
    ui->sendButton->setEnabled(m_connected);
}

void MainWindow::handleUserInfoResponse(const QJsonObject &obj)
{
    QString username = obj["username"].toString();
    QString status = obj["status"].toString();
    QString ip = obj["ip"].toString();

    // Update the user info sidebar
    ui->userInfoName->setText(username);
    ui->userInfoIP->setText(ip);
    ui->userInfoStatusValue->setText(status);

    // Update avatar
    QString firstLetter = username.isEmpty() ? QString("?") : QString(username.at(0).toUpper());
    ui->userInfoAvatar->setText(firstLetter);

    // Generate avatar color
    int hash = 0;
    for (const QChar &c : username) {
        hash = ((hash << 5) - hash) + c.unicode();
    }

    QColor avatarColor;
    if (username.isEmpty()) {
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

    // Update status text
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

    // Show the sidebar
    ui->userInfoSidebar->show();
}

void MainWindow::handleStatusUpdateResponse(const QJsonObject &obj)
{
    QString username = obj["username"].toString();
    QString status = obj["status"].toString();

    // Update user in list
    for (int i = 0; i < ui->userListWidget->count(); ++i) {
        QListWidgetItem *item = ui->userListWidget->item(i);
        UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->userListWidget->itemWidget(item));

        if (chatItem && chatItem->username() == username) {
            chatItem->setStatus(status);
            break;
        }
    }

    // Update chat status if this is the current chat
    if (ui->chatTitle->text() == username) {
        ui->chatStatus->setText(status);
    }

    // If this is the current user's status being updated
    if (username == m_currentUsername) {
        m_currentStatus = status;

        // Update status dropdown without triggering onStatusChanged
        ui->statusComboBox->blockSignals(true);
        if (status == "ACTIVO") {
            ui->statusComboBox->setCurrentIndex(0);
        } else if (status == "OCUPADO") {
            ui->statusComboBox->setCurrentIndex(1);
        } else if (status == "INACTIVO") {
            ui->statusComboBox->setCurrentIndex(2);
        }
        ui->statusComboBox->blockSignals(false);
    }
}

void MainWindow::handleChatMessage(const QJsonObject &obj)
{
    QString from = obj["from"].toString();
    QString message = obj["message"].toString();

    // Check if this is for the current chat
    QString currentChatTitle = ui->chatTitle->text();

    if (currentChatTitle == from) {
        // Add message to chat display
        addChatMessage(from, message, MessageBubble::Received);
    }

    // Update user's last message in the list
    updateUserLastMessage(from, message);

    // If the user info is showing for this sender, we may want to refresh it
    if (ui->userInfoSidebar->isVisible() && ui->userInfoName->text() == from) {
        requestUserInfo(from);
    }
}

void MainWindow::handleBroadcastMessage(const QJsonObject &obj)
{
    QString from = obj["from"].toString();
    QString message = obj["message"].toString();

    // Don't display our own broadcast messages again
    if (from == m_currentUsername) return;

    // Check if the broadcast chat is currently active
    QString currentChatTitle = ui->chatTitle->text();

    if (currentChatTitle == "General Chat") {
        // Add message to chat display
        addChatMessage(from, message, MessageBubble::Received);
    }

    // Update General Chat's last message
    for (int i = 0; i < ui->broadcastListWidget->count(); ++i) {
        QListWidgetItem *item = ui->broadcastListWidget->item(i);
        UserChatItem *chatItem = qobject_cast<UserChatItem*>(ui->broadcastListWidget->itemWidget(item));

        if (chatItem && chatItem->username() == "General Chat") {
            chatItem->setLastMessage(from + ": " + message);
            break;
        }
    }
}

void MainWindow::handleErrorMessage(const QJsonObject &obj)
{
    QString errorMessage = obj["message"].toString();

    // Show error message
    QMessageBox::warning(this, "Error", errorMessage);

    // Add system message to chat
    addSystemMessage("Error: " + errorMessage);
}

void MainWindow::addChatMessage(const QString &sender, const QString &message, MessageBubble::MessageType type)
{
    // Create a MessageBubble widget for the message
    QDateTime timestamp = QDateTime::currentDateTime();
    MessageBubble *bubble = new MessageBubble(sender, message, timestamp, type);

    // Add the bubble to the chat display
    QTextBrowser *display = ui->messageDisplay;

    // Create a QTextBlockFormat for proper spacing
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(10);

    // Create a new block for the message
    QTextCursor cursor = display->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (cursor.position() > 0) {
        cursor.insertBlock(blockFormat);
    }

    // Add the widget to the document
    QTextDocument *doc = display->document();
    cursor.insertHtml("<div style='width:100%;'></div>");

    // Move to the end and scroll to see latest message
    display->verticalScrollBar()->setValue(display->verticalScrollBar()->maximum());
}

void MainWindow::addSystemMessage(const QString &message)
{
    // Create a MessageBubble for the system message
    QDateTime timestamp = QDateTime::currentDateTime();
    MessageBubble *bubble = new MessageBubble("System", message, timestamp, MessageBubble::System);

    // Add the bubble to the chat display
    QTextBrowser *display = ui->messageDisplay;

    // Create a QTextBlockFormat for proper spacing
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(10);

    // Create a new block for the message
    QTextCursor cursor = display->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (cursor.position() > 0) {
        cursor.insertBlock(blockFormat);
    }

    // Add the widget to the document
    QTextDocument *doc = display->document();
    cursor.insertHtml("<div style='width:100%;'></div>");

    // Move to the end and scroll to see latest message
    display->verticalScrollBar()->setValue(display->verticalScrollBar()->maximum());
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
    // In a real application, we would load chat history from storage
    // For now, just add a system message
    addSystemMessage("Starting chat with " + username);
}

void MainWindow::loadBroadcastChatHistory()
{
    // In a real application, we would load broadcast history from storage
    // For now, just add a system message
    addSystemMessage("General Chat - Messages here are sent to all connected users");
}
