#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>

loginwindow::loginwindow(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::loginwindow)
{
    ui->setupUi(this);
    setWindowTitle("Conectarse al Chat");
}

loginwindow::~loginwindow()
{
    delete ui;
}

QString loginwindow::getUsername() const
{
    return username;
}

void loginwindow::on_connectButton_clicked()
{
    QString input = ui->usernameInput->text().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, ingrese un nombre de usuario válido.");
        return;
    }

    username = input;
    accept(); // Cierra la ventana (porque QDialog no tiene exec/accept)
}
