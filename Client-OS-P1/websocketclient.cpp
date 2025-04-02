#include "websocketclient.h"
#include <QUrlQuery>
#include <QDebug>

WebSocketClient::WebSocketClient(const QUrl& url, const QString& username, QObject* parent)
    : QObject(parent), username(username)
{
    connect(&socket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived);
    connect(&socket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);

    QUrl fullUrl = url;
    QUrlQuery query;
    query.addQueryItem("name", username);
    fullUrl.setQuery(query);

    socket.open(fullUrl);
}

void WebSocketClient::onConnected() {
    emit connected();

    // Solicitar lista de usuarios al conectar
    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << quint8(1);  // Cambiado de 0x01 a 1 - Solicitar lista de usuarios
    socket.sendBinaryMessage(payload);

    // Solicitar información del usuario actual para obtener el estado
    QByteArray userInfoPayload;
    QDataStream outInfo(&userInfoPayload, QIODevice::WriteOnly);
    outInfo << quint8(2) << quint8(username.size());  // Cambiado de 0x02 a 2
    outInfo.writeRawData(username.toUtf8().data(), username.size());
    socket.sendBinaryMessage(userInfoPayload);
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray& message) {
    QDataStream in(message);
    quint8 opcode;
    in >> opcode;

    switch (opcode) {
    case 51: { // Cambiado de 0x51 a 51 - Lista de usuarios conectados
        size_t offset = 1;
        quint8 numUsers;
        in >> numUsers;

        QStringList users;
        for (int i = 0; i < numUsers; ++i) {
            QString user = getString8(in, offset);
            quint8 status;
            in >> status;

            QString statusText;
            switch (status) {
            case 1: statusText = "Activo"; break;  // Valores no cambiados porque ya son decimales
            case 2: statusText = "Ocupado"; break;
            case 3: statusText = "Inactivo"; break;
            default: statusText = "Desconectado";
            }

            users.append(user + " (" + statusText + ")");
        }

        emit userListReceived(users);
        break;
    }

    case 52: { // Cambiado de 0x52 a 52 - Información del usuario (estado actual)
        size_t offset = 1;
        QString username = getString8(in, offset);
        quint8 status;
        in >> status;
        emit userStatusReceived(status);
        break;
    }

    case 53: { // Cambiado de 0x53 a 53 - Usuario conectado
        size_t offset = 1;
        QString username = getString8(in, offset);

        emit messageReceived("~", "🔔 " + username + " se ha conectado.");

        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(1);  // Cambiado de 0x01 a 1
        socket.sendBinaryMessage(payload);

        break;
    }

    case 54: { // Cambiado de 0x54 a 54 - Cambio de estado
        size_t offset = 1;
        QString username = getString8(in, offset);
        quint8 newStatus;
        in >> newStatus;

        static QHash<QString, quint8> lastStatus;
        static QHash<QString, QDateTime> lastNotificationTime;

        // Verificamos si ya recibimos esta misma notificación hace menos de 1 segundo
        if (lastStatus.contains(username) &&
            lastStatus[username] == newStatus &&
            lastNotificationTime.contains(username) &&
            lastNotificationTime[username].msecsTo(QDateTime::currentDateTime()) < 1000) {
            break; // ignoramos duplicado
        }

        lastStatus[username] = newStatus;
        lastNotificationTime[username] = QDateTime::currentDateTime();

        QString message;
        switch (newStatus) {
        case 0: message = "🚪 " + username + " se ha desconectado."; break;
        case 1: message = "✅ " + username + " está activo."; break;
        case 2: message = "🔴 " + username + " está ocupado."; break;
        case 3: message = "💤 " + username + " está inactivo."; break;
        default: break;
        }

        emit messageReceived("~", message);

        if (username == this->username) {
            emit userStatusReceived(newStatus);
        } else {
            emit userStatusChanged(username, newStatus);
        }

        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(1);  // Cambiado de 0x01 a 1
        socket.sendBinaryMessage(payload);

        break;
    }

    case 55: { // Cambiado de 0x55 a 55 - Nuevo mensaje recibido (mensaje normal)
        size_t offset = 1;
        QString sender = getString8(in, offset);
        QString msg = getString8(in, offset);
        bool isHistory = false;
        emit messageReceivedWithFlag(sender, msg, isHistory);
        break;
    }

    case 56: { // Cambiado de 0x56 a 56 - Historial recibido
        size_t offset = 1;
        quint8 numMessages;
        in >> numMessages;
    
        qDebug() << "WebSocketClient: Recibidos" << numMessages << "mensajes en el historial.";
    
        emit clearMessages();
    
        for (int i = 0; i < numMessages; ++i) {
            QString sender = getString8(in, offset);
            QString msg = getString8(in, offset);
            bool isHistory = true;
            qDebug() << "WebSocketClient: Historial #" << i << "- De:" << sender << "- Mensaje:" << msg.left(30);
            emit messageReceivedWithFlag(sender, msg, isHistory);
        }
        break;
    }

    case 50: // Cambiado de 0x50 a 50 - Códigos de error
        handleError(in);
        break;

    default:
        break;
    }
}

void WebSocketClient::handleError(QDataStream& in) {
    quint8 errorCode;
    in >> errorCode;

    QString errorMessage;
    switch (errorCode) {
    case 1: // Cambiado de 0x01 a 1
        errorMessage = "El usuario ya está conectado. Por favor, elige otro nombre.";
        emit connectionRejected();
        break;
    case 2: // Cambiado de 0x02 a 2
        errorMessage = "Estado inválido.";
        break;
    case 3: // Cambiado de 0x03 a 3
        errorMessage = "Mensaje vacío.";
        break;
    case 4: // Cambiado de 0x04 a 4
        errorMessage = "El destinatario está desconectado.";
        break;
    default:
        errorMessage = "Error desconocido.";
    }

    qDebug() << "❌ Error recibido: " << errorMessage;
}

void WebSocketClient::sendMessage(const QString& recipient, const QString& message) {
    qDebug() << "DEBUG - WebSocketClient::sendMessage: Enviando a" << recipient << "- Mensaje:" << message.left(30);

    if (recipient.isEmpty()) {
        qDebug() << "Error en WebSocketClient::sendMessage: destinatario vacío";
        return;
    }

    if (message.isEmpty()) {
        qDebug() << "Error en WebSocketClient::sendMessage: mensaje vacío";
        return;
    }

    if (!socket.isValid()) {
        qDebug() << "Error en WebSocketClient::sendMessage: socket no válido";
        return;
    }

    try {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);

        out << quint8(4);  // Cambiado de 0x04 a 4 - Enviar mensaje
        out << quint8(recipient.size());
        out.writeRawData(recipient.toUtf8().data(), recipient.size());
        out << quint8(message.size());
        out.writeRawData(message.toUtf8().data(), message.size());

        qDebug() << "DEBUG - WebSocketClient: enviando paquete de" << payload.size() << "bytes";
        
        QString hexDump;
        for (char byte : payload) {
            hexDump += QString("%1 ").arg((quint8)byte, 2, 16, QLatin1Char('0'));
        }
        qDebug() << "DEBUG - Payload hexadecimal:" << hexDump;
        
        socket.sendBinaryMessage(payload);
    } catch (const std::exception& e) {
        qDebug() << "Excepción en WebSocketClient::sendMessage:" << e.what();
    } catch (...) {
        qDebug() << "Excepción desconocida en WebSocketClient::sendMessage";
    }
}

void WebSocketClient::getChatHistory(const QString& chatName) {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(5);  // Cambiado de 0x05 a 5 - Obtener historial
        out << quint8(chatName.size());
        out.writeRawData(chatName.toUtf8().data(), chatName.size());
        socket.sendBinaryMessage(payload);
    }
}

void WebSocketClient::changeUserStatus(quint8 newStatus) {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(3);  // Cambiado de 0x03 a 3 - Cambiar estado

        out << quint8(username.size());
        out.writeRawData(username.toUtf8().data(), username.size());

        out << newStatus;

        socket.sendBinaryMessage(payload);
        emit statusChanged(newStatus);
        QByteArray debugPayload = payload;
        QString hexDump;
        for (char byte : debugPayload) {
            hexDump += QString("%1 ").arg((quint8)byte, 2, 16, QLatin1Char('0'));
        }
        qDebug() << "[WebSocketClient] Payload (hex):" << hexDump;
    }
}

bool WebSocketClient::isConnected() const {
    return socket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::onDisconnected() {
    // Simplemente cerrar la conexión sin enviar el mensaje 0x06
    socket.close();
    emit disconnected();
}

QString WebSocketClient::getString8(QDataStream &in, size_t &offset) {
    quint8 length;
    in >> length;
    QByteArray data;
    data.resize(length);
    in.readRawData(data.data(), length);
    offset += 1 + length;
    return QString::fromUtf8(data);
}