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
    out << quint8(0x01);  // 0x01 → Solicitar lista de usuarios
    socket.sendBinaryMessage(payload);

    // Solicitar información del usuario actual para obtener el estado
    QByteArray userInfoPayload;
    QDataStream outInfo(&userInfoPayload, QIODevice::WriteOnly);
    outInfo << quint8(0x02) << quint8(username.size());
    outInfo.writeRawData(username.toUtf8().data(), username.size());
    socket.sendBinaryMessage(userInfoPayload);
}


void WebSocketClient::onBinaryMessageReceived(const QByteArray& message) {
    QDataStream in(message);
    quint8 opcode;
    in >> opcode;

    switch (opcode) {
    case 0x51: { // Lista de usuarios conectados
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
            case 0x01: statusText = "Activo"; break;
            case 0x02: statusText = "Ocupado"; break;
            case 0x03: statusText = "Inactivo"; break;
            default: statusText = "Desconocido";
            }

            users.append(user + " (" + statusText + ")");
        }

        // Emitir señal para actualizar la lista de usuarios en la UI
        emit userListReceived(users);
        break;
    }

    case 0x52: { // Información del usuario (estado actual)
        size_t offset = 1;
        QString username = getString8(in, offset);
        quint8 status;
        in >> status;
        emit userStatusReceived(status);
        break;
    }

    case 0x53: { // Usuario conectado
        size_t offset = 1;
        QString username = getString8(in, offset);

        // Mostrar notificación de usuario conectado
        emit messageReceived("~", "🔔 " + username + " se ha conectado.");

        // Solicitar lista actualizada de usuarios
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x01);  // 0x01 → Solicitar lista de usuarios
        socket.sendBinaryMessage(payload);

        break;
    }

    case 0x54: { // Usuario desconectado
        size_t offset = 1;
        QString username = getString8(in, offset);

        // Mostrar notificación de usuario desconectado
        emit messageReceived("~", "🚪 " + username + " se ha desconectado.");

        // Solicitar lista actualizada de usuarios
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x01);  // 0x01 → Solicitar lista de usuarios
        socket.sendBinaryMessage(payload);

        break;
    }

    case 0x55: { // Nuevo mensaje recibido
        size_t offset = 1;
        QString sender = getString8(in, offset);
        QString msg = getString8(in, offset);
        emit messageReceived(sender, msg);
        break;
    }

    case 0x56: { // Historial recibido
        size_t offset = 1;
        quint8 numMessages;
        in >> numMessages;

        for (int i = 0; i < numMessages; ++i) {
            QString sender = getString8(in, offset);
            QString msg = getString8(in, offset);

            emit messageReceived(sender, msg);
        }
        break;
    }

    case 0x57: { // Notificación de usuario desconectado
        size_t offset = 1;
        QString username = getString8(in, offset);

        // Mostrar notificación de desconexión
        emit messageReceived("~", "🚪 " + username + " se ha desconectado.");

        // Solicitar lista actualizada de usuarios después de desconexión
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x01);  // 0x01 → Solicitar lista de usuarios
        socket.sendBinaryMessage(payload);

        break;
    }



    case 0x50: // Error recibido
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
    case 0x01:
        errorMessage = "El usuario ya está conectado. Por favor, elige otro nombre.";
        emit connectionRejected();  // Nueva señal para notificar rechazo
        break;
    case 0x02:
        errorMessage = "Estado inválido.";
        break;
    case 0x03:
        errorMessage = "Mensaje vacío.";
        break;
    case 0x04:
        errorMessage = "El destinatario está desconectado.";
        break;
    default:
        errorMessage = "Error desconocido.";
    }

    qDebug() << "❌ Error recibido: " << errorMessage;
}

void WebSocketClient::sendMessage(const QString& recipient, const QString& message) {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x04);
        out << quint8(recipient.size());
        out.writeRawData(recipient.toUtf8().data(), recipient.size());
        out << quint8(message.size());
        out.writeRawData(message.toUtf8().data(), message.size());
        socket.sendBinaryMessage(payload);
    }
}

void WebSocketClient::getChatHistory(const QString& chatName) {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x05);  // 0x05 → Solicitar historial
        out << quint8(chatName.size());
        out.writeRawData(chatName.toUtf8().data(), chatName.size());
        socket.sendBinaryMessage(payload);
    }
}


void WebSocketClient::changeUserStatus(quint8 newStatus) {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out << quint8(0x03);
        out << newStatus;
        socket.sendBinaryMessage(payload);
        emit statusChanged(newStatus);
    }
}

bool WebSocketClient::isConnected() const {
    return socket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::onDisconnected() {
    if (socket.isValid()) {
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);

        // 0x06 → Código para desconectar
        out << quint8(0x06);
        socket.sendBinaryMessage(payload);
    }

    socket.close();  // Cierra el socket de forma segura
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
