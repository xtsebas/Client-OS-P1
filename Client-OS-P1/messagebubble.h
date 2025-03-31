#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>

class MessageBubble : public QWidget
{
    Q_OBJECT

public:
    enum MessageType {
        Sent,       // Messages sent by current user
        Received,   // Messages received from other users
        System      // System notifications
    };

    explicit MessageBubble(const QString &sender, const QString &message, const QDateTime &timestamp,
                           MessageType type = Received, QWidget *parent = nullptr)
        : QWidget(parent), m_sender(sender), m_message(message), m_timestamp(timestamp), m_type(type)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        // Main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // Create inner widget to apply the bubble styling
        QWidget *bubbleWidget = new QWidget(this);

        // Inner layout for the bubble content
        QVBoxLayout *bubbleLayout = new QVBoxLayout(bubbleWidget);
        bubbleLayout->setSpacing(2);
        bubbleLayout->setContentsMargins(12, 8, 12, 8);

        // Setup the bubble based on message type
        setupBubbleStyle(bubbleWidget);

        // Add sender name if it's a received message
        if (m_type == Received) {
            QLabel *senderLabel = new QLabel(m_sender, bubbleWidget);
            QFont senderFont = senderLabel->font();
            senderFont.setBold(true);
            senderLabel->setFont(senderFont);
            senderLabel->setStyleSheet("color: #075E54;");
            bubbleLayout->addWidget(senderLabel);
        }

        // Message content
        QLabel *messageLabel = new QLabel(m_message, bubbleWidget);
        messageLabel->setWordWrap(true);
        messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bubbleLayout->addWidget(messageLabel);

        // Timestamp
        QLabel *timeLabel = new QLabel(m_timestamp.toString("hh:mm AP"), bubbleWidget);
        timeLabel->setStyleSheet("color: #666666; font-size: 10px;");
        timeLabel->setAlignment(Qt::AlignRight);
        bubbleLayout->addWidget(timeLabel);

        // Align bubble based on message type
        QHBoxLayout *alignmentLayout = new QHBoxLayout();
        alignmentLayout->setContentsMargins(0, 0, 0, 0);

        if (m_type == Sent) {
            alignmentLayout->addStretch();
            alignmentLayout->addWidget(bubbleWidget);
        } else if (m_type == System) {
            alignmentLayout->addStretch();
            alignmentLayout->addWidget(bubbleWidget);
            alignmentLayout->addStretch();
        } else {
            alignmentLayout->addWidget(bubbleWidget);
            alignmentLayout->addStretch();
        }

        mainLayout->addLayout(alignmentLayout);
    }

    QSize sizeHint() const override {
        // Ensure the widget takes up the necessary space
        return QSize(200, 80);
    }

private:
    void setupBubbleStyle(QWidget *bubbleWidget) {
        QString baseStyle = "QWidget {"
                            "border-radius: 10px;"
                            "padding: 5px;"
                            "}";

        switch (m_type) {
        case Sent:
            bubbleWidget->setStyleSheet(baseStyle + "background-color: #DCF8C6;"); // Light green
            break;
        case Received:
            bubbleWidget->setStyleSheet(baseStyle + "background-color: #FFFFFF;"); // White
            break;
        case System:
            bubbleWidget->setStyleSheet(baseStyle +
                                        "background-color: #E1F3FB;"  // Light blue
                                        "color: #555555;");
            break;
        }

        // Set maximum width for the bubble
        bubbleWidget->setMaximumWidth(400);
    }

    QString m_sender;
    QString m_message;
    QDateTime m_timestamp;
    MessageType m_type;
};

#endif // MESSAGEBUBBLE_H
