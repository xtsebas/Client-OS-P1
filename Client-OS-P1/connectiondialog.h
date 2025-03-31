#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QMessageBox>

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Connect to Chat Server");
        resize(400, 200);

        // Create form layout
        QFormLayout *formLayout = new QFormLayout();

        // Username field
        m_usernameEdit = new QLineEdit(this);
        m_usernameEdit->setPlaceholderText("Enter your username");
        formLayout->addRow("Username:", m_usernameEdit);

        // Server host/IP field
        m_serverEdit = new QLineEdit(this);
        m_serverEdit->setPlaceholderText("Server IP or hostname");
        formLayout->addRow("Server:", m_serverEdit);

        // Port field
        m_portEdit = new QSpinBox(this);
        m_portEdit->setRange(1, 65535);
        m_portEdit->setValue(8080); // Default port
        formLayout->addRow("Port:", m_portEdit);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();

        m_connectButton = new QPushButton("Connect", this);
        m_connectButton->setDefault(true);
        m_connectButton->setStyleSheet("QPushButton {"
                                       "background-color: #128C7E;"
                                       "color: white;"
                                       "border-radius: 4px;"
                                       "padding: 6px 16px;"
                                       "}"
                                       "QPushButton:hover {"
                                       "background-color: #075E54;"
                                       "}");

        m_cancelButton = new QPushButton("Cancel", this);

        buttonLayout->addStretch();
        buttonLayout->addWidget(m_cancelButton);
        buttonLayout->addWidget(m_connectButton);

        // Main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(formLayout);
        mainLayout->addStretch();
        mainLayout->addLayout(buttonLayout);

        // Connect signals
        connect(m_connectButton, &QPushButton::clicked, this, &ConnectionDialog::onConnectClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        // Set initial focus
        m_usernameEdit->setFocus();
    }

    QString username() const {
        return m_usernameEdit->text();
    }

    QString server() const {
        return m_serverEdit->text();
    }

    int port() const {
        return m_portEdit->value();
    }

private slots:
    void onConnectClicked() {
        // Validate input
        if (m_usernameEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Username cannot be empty.");
            m_usernameEdit->setFocus();
            return;
        }

        if (m_serverEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Server address cannot be empty.");
            m_serverEdit->setFocus();
            return;
        }

        // All validations passed, accept the dialog
        accept();
    }

private:
    QLineEdit *m_usernameEdit;
    QLineEdit *m_serverEdit;
    QSpinBox *m_portEdit;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;
};

#endif // CONNECTIONDIALOG_H
