#include "auth.h"
#include "ui_auth.h"

AuthForm::AuthForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AuthForm)
{
    ui->setupUi(this);
    switchBtn(ui->credentials_si_submit_btn, false);
    switchBtn(ui->session_si_submit_btn, false);
    switchBtn(ui->su_submit_btn, false);

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(checkInputs()));
    timer->start(100);

    ui->su_form_sq_cb->addItems(secret_questions);
    ui->session_si_saved_users_cb->addItems(database->getLocallySavedUserSessions());

    // Сигналы авторизации
    connect(webApi, &ApiController::authSucceed, this, [this] {

        if (ui->credentials_remember_session->isChecked())
        {
            database->saveUserSessionLocal(windControl->session->getData(), windControl->session->getToken());
        }

        authProceeded("credentials_sign_in");
    });
    connect(webApi, &ApiController::authFailed, this, [this](const QString& message) {
        QMessageBox::critical(this, "", "⚠ Sign in failed!\n\n" + message + " "
                                     "Please check your input.");
        windControl->clearSession();
    });

    // Сигналы восстановления аккаунта пользователя
    connect(webApi, &ApiController::userRecoverSucceed, this, [this]() {
        QMessageBox::information(this, "Success", "Account recovery succeed.\n\n"
                                " Please, proceed auth.");
        database->updateUserPasswordLocal(recoveryUserMetadata.value("username").toString(),
                                          recoveryUserMetadata.value("password").toString());
        recoveryUserMetadata = QJsonObject();
    });
    connect(webApi, &ApiController::userRecoverFailed, this, [this](const QString& message) {
        QMessageBox::critical(this, "Error", "⚠ Account recovery failed!\n\n"
                              + message + " Please check your input.");
        recoveryUserMetadata = QJsonObject();
    });

    dropShadow(ui->content_title_label);
    windControl->PrepareWindow(this);
}

AuthForm::~AuthForm()
{
    delete ui;
}

void AuthForm::checkInputs()
{
    if (windControl->updates["a"])
    {
        QList<QString> credentials_si_inputs = {
            ui->credentials_si_username_input->text(),
            ui->credentials_si_pass_input->text(),
            QString::number(ui->credentials_remember_session->isChecked())
        };
        QList<QString> session_si_inputs = {
            ui->session_si_saved_users_cb->currentText(),
            ui->session_si_pass_input->text()
        };
        QList<QString> su_inputs = {
            ui->su_form_username_input->text(),
            ui->su_form_pass_input->text(),
            ui->su_form_rpass_input->text(),
            ui->su_form_email_input->text(),
            QString::number(ui->su_form_sq_cb->currentIndex()),
            ui->su_form_sq_input->text()
        };

        // Отслеживание состояния кнопок
        switchBtn(ui->credentials_si_submit_btn, !credentials_si_inputs.contains(""));
        switchBtn(ui->session_si_submit_btn, !session_si_inputs.contains(""));
        switchBtn(ui->su_submit_btn, !su_inputs.contains(""));
    }
}

void AuthForm::authProceeded(QString authType)
{
    if (windControl->session)
    {
        windControl->WindowSwap("m");
        windControl->updates["m"] = true;

        if (authType == "credentials_sign_in")
        {
            ui->credentials_si_pass_input->clear();
            ui->credentials_remember_session->setChecked(false);
        }

        ui->session_si_saved_users_cb->clear();
        ui->session_si_saved_users_cb->addItems(database->getLocallySavedUserSessions());
    }
    else
    {
        if (authType == "credentials_sign_in")
        {
            ui->credentials_si_pass_input->clear();
            ui->credentials_remember_session->setChecked(false);
        }
    }
}


void AuthForm::on_exit_btn_clicked()
{
    delete windControl;
}


void AuthForm::on_credentials_si_submit_btn_clicked()
{
    QString username = ui->credentials_si_username_input->text();
    QString password = ui->credentials_si_pass_input->text();
    QString passHash = Enhasher::hashPassword(password);

    webApi->authenticate(*windControl->session, username, password);
    windControl->session->setPassword(passHash);
}

void AuthForm::on_session_si_submit_btn_clicked()
{
    QString username = ui->session_si_saved_users_cb->currentText();
    QString password = ui->session_si_pass_input->text();
    QString passHash = Enhasher::hashPassword(password);

    QString token = database->getUserToken(username, password);
    if (token.contains("ERROR"))
    {
        QMessageBox::critical(this, "", "⚠ Sign in failed!\n\n" + token.split("_")[1]);
        ui->session_si_pass_input->clear();
    }
    else
    {
        windControl->session->setTokenValue(token);
        windControl->session->setPassword(passHash);
        webApi->getUserInfo(*windControl->session);
        ui->session_si_saved_users_cb->setCurrentIndex(-1);
        ui->session_si_pass_input->clear();
    }
}


void AuthForm::on_su_submit_btn_clicked()
{
    if (ui->su_form_pass_input->text() != ui->su_form_rpass_input->text())
    {
        QMessageBox::critical(0,"","  ⚠   Sign Up error!\n\n" "Passwords don't match! "
                                   "Please repeat your password correctly.");
        ui->su_form_rpass_input->setText("");
    }
    else
    {
        QJsonObject user_metadata = {
            {"username", ui->su_form_username_input->text()},
            {"email", ui->su_form_email_input->text()},
            {"password", ui->su_form_pass_input->text()},
            {"secret_num", ui->su_form_sq_cb->currentIndex()},
            {"secret_answer", ui->su_form_sq_input->text()},
        };

        webApi->signUp(*windControl->session, user_metadata);

        ui->su_form_username_input->clear();
        ui->su_form_email_input->clear();
        ui->su_form_pass_input->clear();
        ui->su_form_rpass_input->clear();
        ui->su_form_sq_cb->setCurrentIndex(-1);
        ui->su_form_sq_input->clear();
    }
}


bool AuthForm::showPasswordRecoveryDialog()
{
    QDialog passwordRecoveryDialog(this);
    passwordRecoveryDialog.setWindowTitle("Recover User Account");
    passwordRecoveryDialog.setFixedSize(300, 260);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&passwordRecoveryDialog);

    QLineEdit *usernameLineEdit = new QLineEdit(&passwordRecoveryDialog);
    usernameLineEdit->setPlaceholderText("username");
    dialogLayout->addWidget(usernameLineEdit);

    QLineEdit *emailLineEdit = new QLineEdit(&passwordRecoveryDialog);
    emailLineEdit->setPlaceholderText("email");
    dialogLayout->addWidget(emailLineEdit);

    QComboBox *questionComboBox = new QComboBox(&passwordRecoveryDialog);
    questionComboBox->setCurrentIndex(-1);
    questionComboBox->setPlaceholderText("<Select secret question>");
    questionComboBox->addItems(secret_questions);
    dialogLayout->addWidget(questionComboBox);

    QLineEdit *answerLineEdit = new QLineEdit(&passwordRecoveryDialog);
    answerLineEdit->setPlaceholderText("answer");
    dialogLayout->addWidget(answerLineEdit);

    QLineEdit *passwordLineEdit = new QLineEdit(&passwordRecoveryDialog);
    passwordLineEdit->setPlaceholderText("new password");
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(passwordLineEdit);

    QPushButton *confirmButton = new QPushButton("Recover", &passwordRecoveryDialog);
    dialogLayout->addWidget(confirmButton);

    static const QRegularExpression emailRegex(R"((\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,7}\b))");

    connect(confirmButton, &QPushButton::clicked, this, [this, &passwordRecoveryDialog,
            usernameLineEdit, emailLineEdit, questionComboBox, answerLineEdit, passwordLineEdit]() {
        if (usernameLineEdit->text().isEmpty()) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid Username", "The username cannot be empty.");
            return;
        }

        if (emailLineEdit->text().isEmpty()) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid Email", "The email cannot be empty.");
            return;
        }

        QRegularExpressionMatch match = emailRegex.match(emailLineEdit->text());
        if (!match.hasMatch()) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid Email", "Please enter a valid email address.");
            emailLineEdit->clear();
            return;
        }

        if (questionComboBox->currentIndex() == -1) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid Question", "Please select a secret question.");
            return;
        }

        if (answerLineEdit->text().isEmpty()) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid Answer", "The answer cannot be empty.");
            return;
        }

        if (passwordLineEdit->text().isEmpty()) {
            QMessageBox::warning(&passwordRecoveryDialog, "Invalid New password", "New password cannot be empty.");
            return;
        }

        recoveryUserMetadata = QJsonObject{
            {"username", usernameLineEdit->text()},
            {"email", emailLineEdit->text()},
            {"password", passwordLineEdit->text()},
            {"secret_num", questionComboBox->currentIndex()},
            {"secret_answer", answerLineEdit->text()}
        };

        passwordRecoveryDialog.accept();
    });

    QPushButton *cancelButton = new QPushButton("Cancel", &passwordRecoveryDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &passwordRecoveryDialog, &QDialog::reject);

    return passwordRecoveryDialog.exec() == QDialog::Accepted;
}

void AuthForm::on_credentials_si_recover_clicked()
{
    if (showPasswordRecoveryDialog())
    {
        webApi->recoverUser(*windControl->session, recoveryUserMetadata);
    }
}
