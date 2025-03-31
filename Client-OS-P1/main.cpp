#include "loginwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    loginwindow login;
    if (login.exec() == QDialog::Accepted) {
        QString username = login.getUsername();
        return a.exec();
    }

    return 0; // Cancelado
}
