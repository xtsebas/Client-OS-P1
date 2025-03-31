#ifndef USERCHATITEM_H
#define USERCHATITEM_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <QPainter>

class UserChatItem : public QWidget
{
    Q_OBJECT

public:
    explicit UserChatItem(const QString &username, const QString &status, const QString &lastMessage, QWidget *parent = nullptr)
        : QWidget(parent), m_username(username), m_status(status), m_lastMessage(lastMessage)
    {
        setFixedHeight(70);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // Main layout
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);

        // Avatar - circular label with first letter of username
        m_avatarLabel = new QLabel(this);
        m_avatarLabel->setFixedSize(50, 50);
        m_avatarLabel->setAlignment(Qt::AlignCenter);

        // Get first letter of username
        QString firstLetter = username.isEmpty() ? QString("?") : QString(username.at(0).toUpper());
        m_avatarLabel->setText(firstLetter);

        // User info layout
        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(2);

        // Username label
        m_usernameLabel = new QLabel(username, this);
        QFont usernameFont = m_usernameLabel->font();
        usernameFont.setBold(true);
        m_usernameLabel->setFont(usernameFont);

        // Status indicator
        m_statusIndicator = new QLabel(this);
        updateStatusIndicator(status);

        // Last message label
        m_lastMessageLabel = new QLabel(lastMessage, this);
        m_lastMessageLabel->setMaximumWidth(200);
        m_lastMessageLabel->setStyleSheet("color: #666666; font-size: 12px;");
        m_lastMessageLabel->setWordWrap(true);
        m_lastMessageLabel->setMaximumHeight(35);

        // Add to layouts
        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->addWidget(m_usernameLabel);
        headerLayout->addWidget(m_statusIndicator);
        headerLayout->addStretch();

        infoLayout->addLayout(headerLayout);
        infoLayout->addWidget(m_lastMessageLabel);

        mainLayout->addWidget(m_avatarLabel);
        mainLayout->addLayout(infoLayout);
        mainLayout->addStretch();

        // Style the avatar
        updateAvatar();
    }

    void setUsername(const QString &username) {
        m_username = username;
        m_usernameLabel->setText(username);
        updateAvatar();
    }

    void setStatus(const QString &status) {
        m_status = status;
        updateStatusIndicator(status);
    }

    void setLastMessage(const QString &message) {
        m_lastMessage = message;
        m_lastMessageLabel->setText(message);
    }

    QString username() const {
        return m_username;
    }

    QString status() const {
        return m_status;
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        // Draw bottom border
        QPainter painter(this);
        painter.setPen(QColor("#f0f0f0"));
        painter.drawLine(10, height() - 1, width() - 10, height() - 1);
    }

private:
    void updateAvatar() {
        QString firstLetter = m_username.isEmpty() ? QString("?") : QString(m_username.at(0).toUpper());
        m_avatarLabel->setText(firstLetter);

        // Generate a color based on the username
        int hash = 0;
        for (const QChar &c : m_username) {
            hash = ((hash << 5) - hash) + c.unicode();
        }

        QColor avatarColor;
        if (m_username.isEmpty()) {
            avatarColor = QColor("#128C7E"); // Default color
        } else {
            // Generate a hue based on the hash
            int hue = qAbs(hash) % 360;
            avatarColor = QColor::fromHsv(hue, 200, 200);
        }

        m_avatarLabel->setStyleSheet(QString("QLabel {"
                                             "background-color: %1;"
                                             "border-radius: 25px;"
                                             "color: white;"
                                             "font-weight: bold;"
                                             "font-size: 18px;"
                                             "}").arg(avatarColor.name()));
    }

    void updateStatusIndicator(const QString &status) {
        QString statusColor;
        QString statusSymbol;

        if (status == "ACTIVO") {
            statusColor = "#2ecc71"; // Green
            statusSymbol = "●";
        } else if (status == "OCUPADO") {
            statusColor = "#e74c3c"; // Red
            statusSymbol = "●";
        } else if (status == "INACTIVO") {
            statusColor = "#f1c40f"; // Yellow
            statusSymbol = "●";
        } else {
            statusColor = "#95a5a6"; // Gray
            statusSymbol = "○";
        }

        m_statusIndicator->setText(statusSymbol);
        m_statusIndicator->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(statusColor));
    }

    QString m_username;
    QString m_status;
    QString m_lastMessage;

    QLabel *m_avatarLabel;
    QLabel *m_usernameLabel;
    QLabel *m_statusIndicator;
    QLabel *m_lastMessageLabel;
};

#endif // USERCHATITEM_H
