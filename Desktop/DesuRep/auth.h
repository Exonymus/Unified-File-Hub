#ifndef AUTH_H
#define AUTH_H

#include <QWidget>
#include <QTimer>
#include <QMessageBox>

#include "globals.h"
#include "enhasher.h"

namespace Ui {
class AuthForm;
}

class AuthForm : public QWidget
{
    Q_OBJECT

public:
    explicit AuthForm(QWidget *parent = nullptr);
    ~AuthForm();

private:
    void authProceeded(QString authType);

private slots:
    void checkInputs();

    void on_exit_btn_clicked();

    void on_credentials_si_submit_btn_clicked();

    void on_session_si_submit_btn_clicked();

    void on_su_submit_btn_clicked();

private:
    Ui::AuthForm *ui;
    QTimer *timer;

};

#endif // AUTH_H
