#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>

namespace Ui {
class loginwindow;
}

class loginwindow : public QDialog
{
    Q_OBJECT

public:
    explicit loginwindow(QDialog *parent = nullptr);
    ~loginwindow();

    QString getUsername() const;

private slots:
    void on_connectButton_clicked();

private:
    Ui::loginwindow *ui;
    QString username;
};

#endif // LOGINWINDOW_H
