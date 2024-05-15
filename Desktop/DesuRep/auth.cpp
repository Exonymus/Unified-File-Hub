#include "auth.h"
#include "ui_auth.h"

AuthForm::AuthForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AuthForm)
{
    ui->setupUi(this);
    switchBtn(ui->onl_si_btn, false);
    switchBtn(ui->onl_su_btn, false);
    switchBtn(ui->off_si_btn, false);

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(checkInputs()));
    timer->start(100);

    ui->off_si_savedUsers->addItems(database->GetLocallySavedUsers());

    // Сигналы авторизации
    connect(webApi, &ApiController::authSucceed, this, [this] {
        authProceeded("onl_si");
        database->SaveUserLocal(windControl->session->getData(), "online");
    });
    connect(webApi, &ApiController::authFailed, this, [this](const QString& message) {
        QMessageBox::critical(this, "", "⚠ Sign in failed!\n\n" + message + " "
                                     "Please check your input.");
        windControl->clearSession();
    });

    dropShadow(ui->title_label);
    windControl->PrepareWindow(this);
}

AuthForm::~AuthForm()
{
    delete ui;
}

void AuthForm::checkInputs() {
    if (windControl->updates["a"])
    {
        QList<QString> onl_si_inputs = {ui->onl_si_username_input->text(), ui->onl_si_pass_input->text()};
        QList<QString> onl_su_inputs = {ui->onl_su_username_input->text(), ui->onl_su_email_input->text(),
                                        ui->onl_su_pass_input->text(), ui->onl_su_rpass_input->text()};
        QList<QString> off_si_inputs = {ui->off_si_savedUsers->currentText(), ui->off_si_pass_input->text()};

        // Online login
        if (!onl_si_inputs.contains("")) {
            switchBtn(ui->onl_si_btn, true);
        } else {
            switchBtn(ui->onl_si_btn, false);
        }

        // Online register
        if (!onl_su_inputs.contains("")) {
            switchBtn(ui->onl_su_btn, true);
        } else {
            switchBtn(ui->onl_su_btn, false);
        }

        // Offline Login
        if (!off_si_inputs.contains("")) {
            switchBtn(ui->off_si_btn, true);
        } else {
            switchBtn(ui->off_si_btn, false);
        }
    }
}

void AuthForm::authProceeded(QString authType)
{
    if (windControl->session)
    {
        windControl->WindowSwap("m");
        windControl->updates["m"] = true;

        if (authType == "onl_si") {
            isOfflineMode = false;

            ui->onl_si_pass_input->clear();
        } else if (authType == "off_si") {
            isOfflineMode = true;

            ui->off_si_pass_input->clear();
        } if (authType == "onl_su") {
            isOfflineMode = false;

            ui->onl_su_username_input->clear();
            ui->onl_su_email_input->clear();
            ui->onl_su_pass_input->clear();
            ui->onl_su_rpass_input->clear();
        }

        ui->off_si_savedUsers->clear();
        ui->off_si_savedUsers->addItems(database->GetLocallySavedUsers());
    }

//    ui->pass->setText("");
}


void AuthForm::on_exit_btn_clicked()
{
    delete windControl;
}


void AuthForm::on_onl_si_btn_clicked()
{
    QString username = ui->onl_si_username_input->text();
    QString pass = ui->onl_si_pass_input->text();
    QString passHash = Enhasher::hashPassword(pass);

    webApi->authenticate(username, pass, *windControl->session);
    windControl->session->setPassword(passHash);
}


void AuthForm::on_onl_su_btn_clicked()
{
    if (ui->onl_su_pass_input->text() != ui->onl_su_rpass_input->text()) {
        QMessageBox::critical(0,"","  ⚠   Sign Up error!\n\n" "Passwords don't match! "
                                   "Please repeat your password correctly.");
        ui->onl_su_rpass_input->setText("");
    }
    else {
        QString username = ui->onl_su_username_input->text();
        QString email = ui->onl_su_email_input->text();
        QString pass = ui->onl_su_pass_input->text();
        windControl->session =  database->SignUpUser(username, email, pass);

        authProceeded("onl_su");
    }
}


void AuthForm::on_off_si_btn_clicked()
{
    QString username = ui->off_si_savedUsers->currentText();
    QString pass = ui->off_si_pass_input->text();
    windControl->session =  database->SignInUser(username, pass, "offline");

    authProceeded("off_si");
}

