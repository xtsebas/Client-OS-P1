/********************************************************************************
** Form generated from reading UI file 'loginwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINWINDOW_H
#define UI_LOGINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

class Ui_loginwindow
{
public:
    QDialogButtonBox *connectButton;
    QLabel *label;
    QLineEdit *usernameInput;

    void setupUi(QDialog *loginwindow)
    {
        if (loginwindow->objectName().isEmpty())
            loginwindow->setObjectName("loginwindow");
        loginwindow->resize(400, 300);
        connectButton = new QDialogButtonBox(loginwindow);
        connectButton->setObjectName("connectButton");
        connectButton->setGeometry(QRect(290, 20, 81, 241));
        connectButton->setOrientation(Qt::Orientation::Vertical);
        connectButton->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);
        label = new QLabel(loginwindow);
        label->setObjectName("label");
        label->setGeometry(QRect(20, 20, 201, 17));
        usernameInput = new QLineEdit(loginwindow);
        usernameInput->setObjectName("usernameInput");
        usernameInput->setGeometry(QRect(30, 70, 211, 25));

        retranslateUi(loginwindow);
        QObject::connect(connectButton, &QDialogButtonBox::accepted, loginwindow, qOverload<>(&QDialog::accept));
        QObject::connect(connectButton, &QDialogButtonBox::rejected, loginwindow, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(loginwindow);
    } // setupUi

    void retranslateUi(QDialog *loginwindow)
    {
        loginwindow->setWindowTitle(QCoreApplication::translate("loginwindow", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("loginwindow", "Ingresa nombre de usuario:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class loginwindow: public Ui_loginwindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINWINDOW_H
