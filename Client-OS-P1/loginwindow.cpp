#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include "mainwindow.h"

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

void loginwindow::on_connectButton_clicked() {
    QString username = ui->usernameInput->text().trimmed();

    if (username.isEmpty()) {
        QMessageBox::warning(this, "Error", "El nombre de usuario no puede estar vacío.");
        return;  // No permite iniciar sesión
    }

    // Intentar conectar solo si el nombre no está vacío
    MainWindow *mainWindow = new MainWindow(nullptr, username);
    mainWindow->show();
    this->close();
}

