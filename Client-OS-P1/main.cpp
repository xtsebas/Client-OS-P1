#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Chat Application");
    app.setOrganizationName("UVG OS");
    app.setOrganizationDomain("uvg.edu.gt");
    
    app.setStyleSheet(
        "QMainWindow { background-color: #f5f5f5; }"
        "QMenuBar { background-color: #ffffff; border-bottom: 1px solid #e0e0e0; }"
        "QMenu { background-color: #ffffff; border: 1px solid #e0e0e0; }"
        "QMenu::item:selected { background-color: #f0f2f5; }"
        "QPushButton { padding: 6px 12px; }"
    );
    
    MainWindow w;
    w.show();
    
    return app.exec();
}