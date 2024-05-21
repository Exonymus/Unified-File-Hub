#include "mainwindow.h"
#include "QtCore/qtimer.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->download_btn, SIGNAL(clicked()), this, SLOT(on_actionDownload_triggered()));
    connect(ui->upload_btn, SIGNAL(clicked()), this, SLOT(on_actionUpload_triggered()));

    desuStorage = new FileTreeWidget(ui->desurep_files, "UFH Storage", ui->desurep_file_info, this);

    sessionTimer = new QTimer();
    actionsTimer = new QTimer();

    hideFileStatus();
    dropShadow(ui->file_status_label);
    fileOperationInProgress = false;

    // Сигнал таймеров
    connect(sessionTimer, SIGNAL(timeout()), this, SLOT(sessionCheck()));
    connect(actionsTimer, SIGNAL(timeout()), this, SLOT(actionsCheck()));

    // Сигнал обновления файлов
    connect(webApi, &ApiController::refreshDesuFiles, this, &MainWindow::on_refresh_files_clicked);
    connect(desuStorage, &FileTreeWidget::spaceUsageUpdate, this, &MainWindow::updateStorageUsage);

    // Сигнал истечения сессии
    connect(webApi, &ApiController::sessionExpired, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("User session has expired, please re-login."));
        on_so_btn_clicked();
    });

    // Сигналы выгрузки файла
    connect(webApi, &ApiController::uploadSucceed, this, [this]() {
        QMessageBox::information(this, tr("Upload Complete"), tr("File uploaded successfully."));
        uploadDialog.clearData();

        fileOperationInProgress = false;
        hideFileStatus();

        desuStorage->refreshFiles();
    });
    connect(webApi, &ApiController::uploadFailed, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("Failed to upload the file."));

        fileOperationInProgress = false;
        hideFileStatus();

        on_actionUpload_triggered();
    });

    // Сигналы загрузки файла
    connect(webApi, &ApiController::downloadSucceed, this, [this] {
        fileOperationInProgress = false;
        hideFileStatus();
        QMessageBox::information(this, tr("Download Complete"), tr("File downloaded successfully."));
    });
    connect(webApi, &ApiController::downloadFailed, this, [this](const QString& message) {
        fileOperationInProgress = false;
        hideFileStatus();
        QMessageBox::critical(this, tr("Download Error"), message);
    });

    // Сигналы изменения пользователя
    connect(webApi, &ApiController::userEditSucceed, this, [this](const QString& message) {
        if (message == "email")
        {
            windControl->session->
                    setEmail(changedUserMetadata.value("email").toString());
        }
        else if (message == "password")
        {
            windControl->session->
                    setPassword(Enhasher::hashPassword(changedUserMetadata.value("password").toString()));
            database->saveUserSessionLocal(windControl->session->getData(), windControl->session->getToken());
        }
        else if (message == "secret")
        {
            windControl->session->setSecret(changedUserMetadata.value("secret_num").toInt(),
                                            changedUserMetadata.value("secret_answer").toString());
        }

        QMessageBox::information(this, tr("Edit Complete"), tr("Changes applied succesfully."));
        changedUserMetadata = QJsonObject();
    });
    connect(webApi, &ApiController::userEditFailed, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("Failed to change user info."));
        changedUserMetadata = QJsonObject();
    });

    connect(webApi, &ApiController::userEditFailed, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("Failed to change user info."));
        changedUserMetadata = QJsonObject();
    });

    // Сигналы Google Auth
    google_auth = new GoogleDriveAuth(this);

    connect(google_auth, &GoogleDriveAuth::accessTokenReceived, this, [this]() {
        // driveApi = new GoogleDriveAPI(accessToken, this);
        webApi->linkGDrive(*windControl->session, google_auth->accessToken);
    });
    connect(google_auth, &GoogleDriveAuth::userInfoReceived, this, [this]() {
        google_auth->isLinked = true;
        updateGoogleLinkButton();
        switchBtn(ui->linkGDrive_btn, true);
    });
    connect(google_auth, &GoogleDriveAuth::authorizationError, this, [this](const QString& error) {
        QMessageBox::critical(this, "Google Authorization Error", error);
        google_auth->isLinked = false;
        updateGoogleLinkButton();
    });
    connect(webApi, &ApiController::gDriveChecked, this, [this]() {
        switchBtn(ui->linkGDrive_btn, true);
    });

    updateGoogleLinkButton();
    switchBtn(ui->linkGDrive_btn, false);

    sessionTimer->start(100);

    copyBuffer = File();

    dropShadow(ui->title_label);
    windControl->PrepareWindow(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showFileStatus(QString status)
{

    ui->file_status_label->setText(status);
    ui->file_status_label->show();
    ui->file_status_pb->show();
}

void MainWindow::hideFileStatus()
{
    ui->file_status_label->hide();
    ui->file_status_label->setText("");
    ui->file_status_pb->hide();
    ui->file_status_pb->setValue(0);
}

void MainWindow::sessionCheck()
{
    if (windControl->session && windControl->updates["m"])
    {
        windControl->updates["m"] = false;

        // Установить имя пользователя
        ui->title_label->setText(ui->title_label->text() + windControl->session->getUsername());
        ui->username_label->setText(windControl->session->getUsername());

        // Обновить список файлов
        on_refresh_files_clicked();

        actionsTimer->start(100);
        sessionTimer->stop();

        // Google Drive
        webApi->getGDrive(*windControl->session, *google_auth);
    }
}

void MainWindow::updateStorageUsage()
{
    ui->drep_info->setValue(desuStorage->spaceUsage());
}


QString getPath(QTreeWidgetItem *item)
{
    QStringList pathSegments;
    while (item) {
        pathSegments.prepend(item->text(0));
        item = item->parent();
    }
    return "/" + pathSegments.join("/");
}

void MainWindow::actionsCheck()
{
    QTreeWidgetItem *selectedItem = ui->desurep_files->currentItem();

    if (selectedItem && ui->storages_tabWidget->currentIndex() == 1) {
        // Текущие свойства
        QString filePath = getPath(selectedItem);
        bool editAccess = filePath.split("/", Qt::SkipEmptyParts)[0] != "Public Files";
        bool isFolder = selectedItem->data(0, Qt::UserRole).isNull();

        // Нередактирующие действия
        ui->actionCopy->setEnabled(!isFolder);
        ui->actionPaste->setEnabled((!copyBuffer.isEmptyFile() || !cutBuffer.isEmptyFile()) && editAccess);

        ui->actionDownload->setEnabled(!isFolder);
        ui->actionUpload->setEnabled(true);

        switchBtn(ui->download_btn, !isFolder);
        switchBtn(ui->upload_btn, true);

        // Редактирующие действия
        ui->actionEdit->setEnabled(!selectedItem->data(0, Qt::UserRole).isNull() && editAccess);
        ui->actionCut->setEnabled(!selectedItem->data(0, Qt::UserRole).isNull() && editAccess);
        ui->actionDelete->setEnabled(!selectedItem->data(0, Qt::UserRole).isNull() && editAccess);
    } else if (ui->storages_tabWidget->currentIndex() != 1) {
        switchBtn(ui->download_btn, false);
        switchBtn(ui->upload_btn, false);

        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionDownload->setEnabled(false);
        ui->actionEdit->setEnabled(false);
        ui->actionDelete->setEnabled(false);
        ui->actionUpload->setEnabled(false);
    } else {
        switchBtn(ui->download_btn, false);
        switchBtn(ui->upload_btn, true);

        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionDownload->setEnabled(false);
        ui->actionEdit->setEnabled(false);
        ui->actionDelete->setEnabled(false);
        ui->actionUpload->setEnabled(true);
    }

    if (fileOperationInProgress) {
        ui->actionUpload->setEnabled(false);
        ui->actionDownload->setEnabled(false);
        switchBtn(ui->download_btn, false);
        switchBtn(ui->upload_btn, false);
    }

    // Check Google Drive
    updateGoogleLinkButton();
}

void MainWindow::on_actionCopy_triggered()
{
    // Обработка события копирования
    QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
    copyBuffer = findFileById(selectedFileId, desuStorage->getFiles());

    copyBuffer.setOwner(windControl->session->getId());
    if (!cutBuffer.isEmptyFile()) { cutBuffer = File(); }
}

void MainWindow::on_actionCut_triggered()
{
    // Обработка события вырезания
    QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
    cutBuffer = findFileById(selectedFileId, desuStorage->getFiles());

    if (!copyBuffer.isEmptyFile()) { copyBuffer = File(); }
}

void MainWindow::on_actionPaste_triggered()
{
    // Обработка события вставки
    QString filePath = getPath(ui->desurep_files->currentItem()).removeFirst();
    QString filePath_formatted = filePath != "My Files"? filePath.mid(filePath.indexOf("/")).removeFirst() : ".";

    // Обработка события вставки
    if (!copyBuffer.isEmptyFile()) {
        // Копирование
        if (filePath_formatted != copyBuffer.getPath())
        {
            copyBuffer.setPath(filePath_formatted);
        }

        if (copyBuffer.getSizeInMB() < desuStorage->spaceAvailableMB())
        {
            webApi->copyFile(*windControl->session, copyBuffer.getId().toString(), filePath_formatted);
        } else {
            QMessageBox::critical(this, "", "⚠ File size error!\n\nThe copied file has exceeded your space limit! "
                                         "\nPlease select another file to copy.");
        }

    } else if (!cutBuffer.isEmptyFile()) {
        // Перемещение
        if (filePath_formatted != cutBuffer.getPath())
        {
            cutBuffer.setPath(filePath_formatted);
            webApi->updateFile(*windControl->session, cutBuffer.getMetaData());

            cutBuffer = File();
        }
    }
}


File MainWindow::findFileById(QUuid id, const QList<File> *fileList)
{
    for (const File &file : *fileList) {
        if (file.getId() == id) {
            return file;
        }
    }

    return File();
}

void MainWindow::on_actionEdit_triggered()
{
    // Обработка события изменения файла
    QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
    File selectedFile = findFileById(selectedFileId, desuStorage->getFiles());
    editDialog.setFileProperties(selectedFile.getMetaData());
    bool changed = editDialog.exec();

    if (changed) {
        QJsonObject ed_MetaData = editDialog.getEditedData();
        ed_MetaData["id"] = selectedFileId.toString();
        webApi->updateFile(*windControl->session, ed_MetaData);
    }
}

bool MainWindow::showDeleteConfirmationDialog(QString fileName)
{
    QDialog deleteDialog(this);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&deleteDialog);
    dialogLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLabel *label = new QLabel("Do you really want to delete '" + fileName + "' ?", &deleteDialog);
    dialogLayout->addWidget(label);

    QPushButton *confirmButton = new QPushButton("Delete", &deleteDialog);
    dialogLayout->addWidget(confirmButton);
    connect(confirmButton, &QPushButton::clicked, &deleteDialog, &QDialog::accept);

    QPushButton *cancelButton = new QPushButton("Cancel", &deleteDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &deleteDialog, &QDialog::reject);

    return deleteDialog.exec() == QDialog::Accepted;
}


bool MainWindow::showChangeEmailDialog()
{
    QDialog changeEmailDialog(this);
    changeEmailDialog.setWindowTitle("Change Account Email");
    changeEmailDialog.setFixedSize(300, 180);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&changeEmailDialog);

    QLabel *label = new QLabel("Current Email: " + windControl->session->getEmail(), &changeEmailDialog);
    dialogLayout->addWidget(label);

    QLineEdit *newEmailLineEdit = new QLineEdit(&changeEmailDialog);
    newEmailLineEdit->setPlaceholderText("email@example.com");
    dialogLayout->addWidget(newEmailLineEdit);

    QLineEdit *userPasswordEdit = new QLineEdit(&changeEmailDialog);
    userPasswordEdit->setPlaceholderText("password");
    userPasswordEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(userPasswordEdit);

    QPushButton *confirmButton = new QPushButton("Change Email", &changeEmailDialog);
    dialogLayout->addWidget(confirmButton);

    static const QRegularExpression emailRegex(R"((\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,7}\b))");

    connect(confirmButton, &QPushButton::clicked, this, [this, newEmailLineEdit, userPasswordEdit, &changeEmailDialog]()
    {
        QString newEmail = newEmailLineEdit->text();
        QString password = userPasswordEdit->text();

        // Validate email format
        QRegularExpressionMatch match = emailRegex.match(newEmail);
        if (!match.hasMatch()) {
            QMessageBox::warning(&changeEmailDialog, "Invalid Email", "Please enter a valid email address.");
            userPasswordEdit->clear();
            return;
        }

        // Validate password
        if (!Enhasher::checkPassword(password, windControl->session->getPassword())) {
            QMessageBox::warning(&changeEmailDialog, "Invalid Password", "The password you entered is incorrect.");
            userPasswordEdit->clear();
            return;
        }

        // If both checks pass
        changedUserMetadata = QJsonObject{{"email", newEmail}};
        changeEmailDialog.accept();
    });

    QPushButton *cancelButton = new QPushButton("Cancel", &changeEmailDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &changeEmailDialog, &QDialog::reject);

    return changeEmailDialog.exec() == QDialog::Accepted;
}

bool MainWindow::showChangePasswordDialog()
{
    QDialog changePasswordDialog(this);
    changePasswordDialog.setWindowTitle("Change Account Password");
    changePasswordDialog.setFixedSize(300, 180);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&changePasswordDialog);

    QLineEdit *currentPasswordLineEdit = new QLineEdit(&changePasswordDialog);
    currentPasswordLineEdit->setPlaceholderText("current password");
    currentPasswordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(currentPasswordLineEdit);

    QLineEdit *newPasswordLineEdit = new QLineEdit(&changePasswordDialog);
    newPasswordLineEdit->setPlaceholderText("new password");
    newPasswordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(newPasswordLineEdit);

    QPushButton *confirmButton = new QPushButton("Change Password", &changePasswordDialog);
    dialogLayout->addWidget(confirmButton);

    connect(confirmButton, &QPushButton::clicked, this, [ currentPasswordLineEdit, newPasswordLineEdit, &changePasswordDialog, this]()
    {
        QString currentPassword = currentPasswordLineEdit->text();
        QString newPassword = newPasswordLineEdit->text();

        // Валидация пароля
        if (!Enhasher::checkPassword(currentPassword, windControl->session->getPassword()))
        {
            QMessageBox::warning(&changePasswordDialog, "Invalid Password", "The current password you entered is incorrect.");
            return;
        }

        if (currentPassword == newPassword) {
            QMessageBox::warning(&changePasswordDialog, "Invalid Password", "The new password cannot be the same as the current password.");
            return;
        }

        changedUserMetadata = QJsonObject{{"password", newPassword}};
        changePasswordDialog.accept();
    });

    QPushButton *cancelButton = new QPushButton("Cancel", &changePasswordDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &changePasswordDialog, &QDialog::reject);

    return changePasswordDialog.exec() == QDialog::Accepted;
}

bool MainWindow::showSecretQuestionRecoveryDialog()
{
    QDialog secretQuestionDialog(this);
    secretQuestionDialog.setWindowTitle("Restore Secret Question");
    secretQuestionDialog.setFixedSize(300, 240);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&secretQuestionDialog);

    QComboBox *questionComboBox = new QComboBox(&secretQuestionDialog);
    questionComboBox->setCurrentIndex(-1);
    questionComboBox->setPlaceholderText("<Select secret question>");
    questionComboBox->addItems(secret_questions);
    dialogLayout->addWidget(questionComboBox);

    QLineEdit *answerLineEdit = new QLineEdit(&secretQuestionDialog);
    answerLineEdit->setPlaceholderText("Answer");
    dialogLayout->addWidget(answerLineEdit);

    QLineEdit *passwordLineEdit = new QLineEdit(&secretQuestionDialog);
    passwordLineEdit->setPlaceholderText("Password");
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(passwordLineEdit);

    QLabel *registrationDateLabel = new QLabel("Enter account registration date (MM/YYYY):", &secretQuestionDialog);
    dialogLayout->addWidget(registrationDateLabel);

    QLineEdit *regDatehLineEdit = new QLineEdit(&secretQuestionDialog);
    regDatehLineEdit->setPlaceholderText("MM/YYYY");
    dialogLayout->addWidget(regDatehLineEdit);

    QPushButton *confirmButton = new QPushButton("Restore", &secretQuestionDialog);
    dialogLayout->addWidget(confirmButton);

    connect(confirmButton, &QPushButton::clicked, this, [this, &secretQuestionDialog,
            passwordLineEdit, regDatehLineEdit, questionComboBox, answerLineEdit]() {
        QString enteredPassword = passwordLineEdit->text();
        QString enteredDateStr = regDatehLineEdit->text();
        QStringList dateParts = enteredDateStr.split("/");

        // Проверка выбора вопроса и ответа
        if (questionComboBox->currentIndex() == -1) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Question", "Please select a secret question.");
            return;
        }

        if (answerLineEdit->text().isEmpty()) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Answer", "The answer cannot be empty.");
            return;
        }

        // Валидация пароля
        if (!Enhasher::checkPassword(enteredPassword, windControl->session->getPassword())) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Password", "The password you entered is incorrect.");
            return;
        }

        if (dateParts.size() != 2) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Date Format", "Please enter the date in MM/YYYY format.");
            return;
        }

        static const QRegularExpression dateRegex(R"(^((0[1-9])|(1[0-2]))\/(2023|202[4-9]|20[3-9]\d{2}|[2-9]\d{3})$)");
        QRegularExpressionMatch match = dateRegex.match(enteredDateStr);

        if (!match.hasMatch()) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Date Format", "Please enter the date in MM/YYYY format with year later than 2023.");
            return;
        }

        QString enteredMonth = dateParts[0];
        QString enteredYear = dateParts[1];

        QDate enteredDate(enteredYear.toInt(), enteredMonth.toInt(), 1);
        QDate currentDate = QDate::currentDate();
        QDate minValidDate(2023, 1, 1);
        if (enteredDate < minValidDate || enteredDate > currentDate) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Date", "The date must be between January 2023 and the current date.");
            return;
        }

        // Валидация даты регистрации
        QDate regDate = windControl->session->getRegDate().date();
        if (enteredDate.year() != regDate.year() || enteredDate.month() != regDate.month()) {
            QMessageBox::warning(&secretQuestionDialog, "Invalid Date", "The registration date you entered is incorrect.");
            return;
        }

        changedUserMetadata = QJsonObject{
            {"secret_num", questionComboBox->currentIndex()},
            {"secret_answer", answerLineEdit->text()}
        };

        secretQuestionDialog.accept();
    });

    QPushButton *cancelButton = new QPushButton("Cancel", &secretQuestionDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &secretQuestionDialog, &QDialog::reject);

    return secretQuestionDialog.exec() == QDialog::Accepted;
}


void MainWindow::on_actionDelete_triggered()
{
    QString selectedFileId = ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();

    // Обработка события удаления файла
    if (showDeleteConfirmationDialog(ui->desurep_files->currentItem()->text(0))) {
        webApi->deleteFile(*windControl->session, selectedFileId);
    }
}

void MainWindow::on_actionDownload_triggered()
{
    // Обработка события загрузки файла
    QTreeWidgetItem *selectedItem = ui->desurep_files->currentItem();

    if (selectedItem) {
        // Получение ссылки на скачивание и имени файла из пользовательских данных
        QString selectedFileId = ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();
        QString fileName = selectedItem->text(0) + "." +
                File::getFileExtensionFromMimeType(selectedItem->data(0, Qt::UserRole).toJsonObject()["mime_type"].toString());
        QString savePath = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/Downloads/" + fileName);

        // Выполнение скачивания файла
        showFileStatus("Downloading file from Storage:");
        fileOperationInProgress = true;
        webApi->downloadFile(*windControl->session,
                             selectedFileId,
                             savePath,
                             ui->file_status_pb);
    }
}

void MainWindow::on_actionUpload_triggered()
{
    // Обработка события выгрузки файла
    uploadDialog.setRestrictions(desuStorage->spaceAvailableMB());
    if (uploadDialog.exec() == QDialog::Accepted)
    {
        showFileStatus("Uploading file to Storage:");
        fileOperationInProgress = true;
        webApi->uploadFile(*windControl->session,
                           uploadDialog.getUploadData(),
                           ui->file_status_pb);
    }
    else
    {
        uploadDialog.clearData();
    }
}


void MainWindow::on_refresh_files_clicked()
{
    desuStorage->refreshFiles();
}


void MainWindow::on_exit_btn_clicked()
{
    delete windControl;
}

void MainWindow::on_so_btn_clicked()
{
    // Очистим рабочее окно
    ui->desurep_files->clear();
    ui->desurep_file_info->clear();
    ui->title_label->setText("Welcome, ");
    ui->desurep_files->headerItem()->setText(0, "UFH Storage");

    // Буфферы файлов
    copyBuffer = File();
    cutBuffer = File();

    // Переключиимся на предыдущее окно
    windControl->clearSession();
    windControl->WindowSwap("a");
    windControl->updates["a"] = true;

    // Менеджмент таймеров
    sessionTimer->start();
    actionsTimer->stop();

    // Очистим Google Drive
    google_auth->clear();
    updateGoogleLinkButton();
    switchBtn(ui->linkGDrive_btn, false);
}

// Редактирование профиля
void MainWindow::on_change_email_btn_clicked()
{
    if (showChangeEmailDialog())
    {
        webApi->editUser(*windControl->session, changedUserMetadata, "email");
    }

}

void MainWindow::on_change_pass_btn_clicked()
{
    if (showChangePasswordDialog())
    {
        webApi->editUser(*windControl->session, changedUserMetadata, "password");
    }
}

void MainWindow::on_change_sq_btn_clicked()
{
    if (showSecretQuestionRecoveryDialog())
    {
        webApi->editUser(*windControl->session, changedUserMetadata, "secret");
    }
}

// Интеграция Google Drive
void MainWindow::on_linkGDrive_btn_clicked()
{
    if (google_auth->isLinked)
    {
        // Unlink Google Drive
        google_auth->accessToken.clear();
        google_auth->userEmail.clear();
        google_auth->userAvatarData.clear();
        google_auth->isLinked = false;
        webApi->unlinkGDrive(*windControl->session);
        QMessageBox::information(this, "Google Drive", "Successfully unlinked Google Drive.");
    }
    else
    {
        // Link Google Drive
        google_auth->authenticate();
    }
}

void MainWindow::updateGoogleLinkButton()
{
    if (google_auth->isLinked)
    {
        ui->linkGDrive_btn->setText("Unlink " + google_auth->userEmail);
        QPixmap pixmap;
        pixmap.loadFromData(google_auth->userAvatarData);
        QIcon icon(pixmap);
        ui->linkGDrive_btn->setIcon(icon);
    }
    else
    {
        ui->linkGDrive_btn->setText("Link Google Drive");
        ui->linkGDrive_btn->setIcon(QIcon(":icons/gdrive"));
    }
}
