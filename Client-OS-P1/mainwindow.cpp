#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkRequest>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // Conectar la señal para manejar la respuesta del servidor
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkReply);

    // Realizar GET a tu servidor
    QUrl url("http://18.224.60.241:18080/");
    QNetworkRequest request(url);
    networkManager->get(request);
}

void MainWindow::handleNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString response = reply->readAll();
        qDebug() << "Respuesta del servidor:" << response;

        // Mostrar en la UI (usa un QLabel o QTextEdit en el .ui)
        ui->textEdit->setText(response);  // Asegúrate de tener un QTextEdit llamado `textEdit`
    } else {
        ui->textEdit->setText("Error: " + reply->errorString());
    }

    reply->deleteLater();
}

MainWindow::~MainWindow()
{
    delete ui;
}
