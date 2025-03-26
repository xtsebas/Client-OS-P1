#include "mainwindow.h"
#include "loginwindow.h"
#include <QDialog>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    loginwindow login;
    if (login.exec() == QDialog::Accepted) {
        QString username = login.getUsername();

        MainWindow w(nullptr, username);
        w.show();

        return a.exec();
    }

    return 0; // Cancelado
}
