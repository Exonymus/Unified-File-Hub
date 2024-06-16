#include "mainwindow.h"
#include "QtCore/qtimer.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->download_btn, SIGNAL(clicked()), this, SLOT(on_actionDownload_triggered()));
    connect(ui->download_gdrive_btn, SIGNAL(clicked()), this, SLOT(on_actionDownload_triggered()));
    connect(ui->download_ftp_btn, SIGNAL(clicked()), this, SLOT(on_actionDownload_triggered()));

    connect(ui->upload_btn, SIGNAL(clicked()), this, SLOT(on_actionUpload_triggered()));
    connect(ui->upload_gdrive_btn, SIGNAL(clicked()), this, SLOT(on_actionUpload_triggered()));
    connect(ui->upload_ftp_btn, SIGNAL(clicked()), this, SLOT(on_actionUpload_triggered()));

    desuStorage = new FileTreeWidget(ui->desurep_files, "UFH Storage", ui->desurep_file_info, "ufh", this);
    gdriveStorage = new FileTreeWidget(ui->gdrive_files, "Google Drive", ui->gdrive_file_info, "gdrive", this);
    ftpStorage = new FileTreeWidget(ui->ftp_files, "FTP Server", ui->ftp_file_info, "ftp", this);

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
    connect(google_api, &GoogleDriveAPI::filesNeedUpdate, this, &MainWindow::on_refresh_files_gdrive_btn_clicked);
    connect(ftp_api, &FTPController::filesNeedUpdate, this, &MainWindow::on_refresh_files_ftp_btn_clicked);

    connect(desuStorage, &FileTreeWidget::spaceUsageUpdate, this, &MainWindow::updateStorageUsage);
    connect(gdriveStorage, &FileTreeWidget::spaceUsageUpdate, this, &MainWindow::updateStorageUsage);
    connect(ftpStorage, &FileTreeWidget::spaceUsageUpdate, this, &MainWindow::updateStorageUsage);

    // Сигнал истечения сессии
    connect(webApi, &ApiController::sessionExpired, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("User session has expired, please re-login."));
        on_so_btn_clicked();
    });

    // Сигналы выгрузки файла
    auto uploadSucceedHandler = [this] {
        QMessageBox::information(this, tr("Upload Complete"), tr("File uploaded successfully."));
        uploadDialog.clearData();

        fileOperationInProgress = false;
        hideFileStatus();

        if (fileUploader == "ufh") {
            desuStorage->refreshFiles();
        } else if (fileUploader == "gdrive") {
            gdriveStorage->refreshFiles();
        } else if (fileUploader == "ftp") {
            ftpStorage->refreshFiles();
        }

        fileUploader.clear();
    };
    connect(webApi, &ApiController::uploadSucceed, this, uploadSucceedHandler); // UFH Storage
    connect(google_api, &GoogleDriveAPI::uploadSucceed, this, uploadSucceedHandler); // Google Drive
    connect(ftp_api, &FTPController::uploadSucceed, this, uploadSucceedHandler); // FTP Server

    auto uploadFailedHandler = [this] {
        QMessageBox::warning(this, tr("Error"), tr("Failed to upload the file."));

        fileOperationInProgress = false;
        hideFileStatus();

        on_actionUpload_triggered();
    };
    connect(webApi, &ApiController::uploadFailed, this, uploadFailedHandler); // UFH Storage
    connect(google_api, &GoogleDriveAPI::uploadFailed, this, uploadFailedHandler); // Google Drive
    connect(ftp_api, &FTPController::uploadFailed, this, uploadFailedHandler); // FTP Server

    // Сигналы загрузки файла
    auto downloadSucceedHandler = [this] {
        fileOperationInProgress = false;
        hideFileStatus();
        QMessageBox::information(this, tr("Download Complete"), tr("File downloaded successfully."));
    };
    connect(webApi, &ApiController::downloadSucceed, this, downloadSucceedHandler); // UFH Storage
    connect(google_api, &GoogleDriveAPI::downloadSucceed, this, downloadSucceedHandler); // Google Drive
    connect(ftp_api, &FTPController::downloadSucceed, this, downloadSucceedHandler); // FTP Server

    auto downloadFailedHandler = [this](const QString& message) {
        fileOperationInProgress = false;
        hideFileStatus();
        QMessageBox::critical(this, tr("Download Error"), message);
    };
    connect(webApi, &ApiController::downloadFailed, this, downloadFailedHandler); // UFH Storage
    connect(google_api, &GoogleDriveAPI::downloadFailed, this, downloadFailedHandler); // Google Drive
    connect(ftp_api, &FTPController::downloadFailed, this, downloadFailedHandler); // FTP Server


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
        webApi->linkGDrive(*windControl->session,
                           google_auth->accessToken,
                           google_auth->refreshToken);

    });
    connect(google_auth, &GoogleDriveAuth::userInfoReceived, this, [this]() {
        google_auth->isLinked = true;
        updateGoogleLinkButton();
        switchBtn(ui->linkGDrive_btn, true);

        google_api->setData(google_auth->accessToken, google_auth->userEmail);
        gdriveStorage->refreshFiles();
    });
    connect(google_auth, &GoogleDriveAuth::authorizationError, this, [this](const QString& error) {
        QMessageBox::critical(this, "Google Authorization Error", error);
        google_auth->isLinked = false;
        updateGoogleLinkButton();
        switchBtn(ui->linkGDrive_btn, true);
    });
    connect(webApi, &ApiController::gDriveChecked, this, [this]() {
        switchBtn(ui->linkGDrive_btn, true);
    });

    updateGoogleLinkButton();
    switchBtn(ui->linkGDrive_btn, false);

    // Сигналы FTP
    connect(webApi, &ApiController::ftpChecked, this, [this]() {
        if (ftp_api->getConnections().size() > 0) {
            addServerDialog.clearData();

            int currentIndexServer = ui->ftp_connection_cb->currentIndex();
            int currentIndexServerUsage = ui->ftp_storage_selector->currentIndex();

            ui->ftp_connection_cb->clear();
            ui->ftp_storage_selector->clear();

            foreach (FTPConnection connection, ftp_api->getConnections()) {
                ui->ftp_connection_cb->addItem(connection.name, connection.id);
                ui->ftp_storage_selector->addItem(connection.name, connection.id);
            }

            ui->ftp_connection_cb->setCurrentIndex(currentIndexServer);
            ui->ftp_storage_selector->setCurrentIndex(currentIndexServerUsage);
        }
    });

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

        // FTP
        webApi->getFtpConns(*windControl->session, *ftp_api);
    }
}

void MainWindow::updateStorageUsage()
{
    ui->drep_info->setValue(desuStorage->spaceUsage());
    if (google_api->isLinked) {
        google_api->fetchStorageUsage(ui->gdrive_info);
    }
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
    if (fileOperationInProgress) {
        ui->actionUpload->setEnabled(false);
        ui->actionDownload->setEnabled(false);

        switchBtn(ui->download_btn, false);
        switchBtn(ui->download_gdrive_btn, false);
        switchBtn(ui->download_ftp_btn, false);

        switchBtn(ui->upload_btn, false);
        switchBtn(ui->upload_gdrive_btn, false);
        switchBtn(ui->upload_ftp_btn, false);
        return;
    }

    // Check Google Drive
    updateGoogleLinkButton();

    int currentIndex = ui->storages_tabWidget->currentIndex();
    QTreeWidgetItem *selectedItem = nullptr;

    if (currentIndex == 1) {
        selectedItem = ui->desurep_files->currentItem();
    } else if (currentIndex == 2) {
        selectedItem = ui->gdrive_files->currentItem();
    } else if (currentIndex == 4) {
        selectedItem = ui->ftp_files->currentItem();
    }

    bool isDesurepTab = (currentIndex == 1);
    bool isGDriveTab = (currentIndex == 2);
    bool isFTPTab = (currentIndex == 4);

    if (selectedItem) {
        QString filePath = getPath(selectedItem);
        bool isFolder = selectedItem->data(0, Qt::UserRole).isNull();
        bool editAccess = (isDesurepTab)
                          ? filePath.split("/", Qt::SkipEmptyParts)[0] != "Public Files"
                          : filePath.split("/", Qt::SkipEmptyParts)[0] != "Available Files";

        // Non-editing actions
        ui->actionCopy->setEnabled(!isFolder);
        ui->actionPaste->setEnabled((!copyBuffer.isEmptyFile() || !cutBuffer.isEmptyFile()) && editAccess);
        ui->actionDownload->setEnabled(!isFolder);
        ui->actionUpload->setEnabled(true);

        if (isDesurepTab) {
            switchBtn(ui->download_btn, !isFolder);
            switchBtn(ui->upload_btn, true);
        } else if (isGDriveTab && google_auth->isLinked) {
            switchBtn(ui->download_gdrive_btn, !isFolder);
            switchBtn(ui->upload_gdrive_btn, true);
        } else if (isGDriveTab && !google_auth->isLinked) {
            switchBtn(ui->download_gdrive_btn, false);
            switchBtn(ui->upload_gdrive_btn, false);
        } else if (isFTPTab && ftp_api->isConnectionSelected()) {
            switchBtn(ui->download_ftp_btn, !isFolder);
            switchBtn(ui->upload_ftp_btn, true);
        } else if (isFTPTab && !ftp_api->isConnectionSelected()) {
            switchBtn(ui->download_ftp_btn, false);
            switchBtn(ui->upload_ftp_btn, false);
        }

        // Editing actions
        ui->actionEdit->setEnabled(editAccess && !isFolder);
        ui->actionCut->setEnabled(editAccess && !isFolder);
        ui->actionDelete->setEnabled(editAccess && !isFolder);
    } else {
        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionDownload->setEnabled(false);
        ui->actionEdit->setEnabled(false);
        ui->actionDelete->setEnabled(false);
        ui->actionUpload->setEnabled(!isDesurepTab && !isGDriveTab && !isFTPTab);

        if (isDesurepTab) {
            switchBtn(ui->download_btn, false);
            switchBtn(ui->upload_btn, false);
        } else if (isGDriveTab) {
            switchBtn(ui->download_gdrive_btn, false);
            switchBtn(ui->upload_gdrive_btn, false);
        } else if (isFTPTab) {
            switchBtn(ui->download_ftp_btn, false);
            switchBtn(ui->upload_ftp_btn, false);
        } else {
            switchBtn(ui->download_btn, false);
            switchBtn(ui->upload_btn, false);
            switchBtn(ui->download_gdrive_btn, false);
            switchBtn(ui->upload_gdrive_btn, false);
        }
    }
}

void MainWindow::on_actionCopy_triggered()
{
    // Обработка события копирования
    int currentIndex = ui->storages_tabWidget->currentIndex();

    if (currentIndex == 1) {
        QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
        copyBuffer = findFileById(selectedFileId, desuStorage->getFiles());

        copyBuffer.setOwner(windControl->session->getId());
        if (!cutBuffer.isEmptyFile()) { cutBuffer = File(); }
    } else if (currentIndex == 2) {
        //
    } else if (currentIndex == 4) {
        QMessageBox::warning(this, "", "Copying files using FTP protocol is not supported by default.");
    }
}

void MainWindow::on_actionCut_triggered()
{
    // Обработка события вырезания
    int currentIndex = ui->storages_tabWidget->currentIndex();

    if (currentIndex == 1) {
        QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
        cutBuffer = findFileById(selectedFileId, desuStorage->getFiles());

        if (!copyBuffer.isEmptyFile()) { copyBuffer = File(); }
    } else if (currentIndex == 2) {
        //
    } else if (currentIndex == 4) {
        QString selectedFileId = ui->ftp_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();
        cutBuffer = File::findObjectById(*ftpStorage->getFiles(), selectedFileId, "ftp");
    }
}

void MainWindow::on_actionPaste_triggered()
{
    // Обработка события вставки
    int currentIndex = ui->storages_tabWidget->currentIndex();

    if (currentIndex == 1) {
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
    } else if (currentIndex == 2) {
        //
    } else if (currentIndex == 4) {
        QString filePath = getPath(ui->ftp_files->currentItem()).removeFirst();
        QString filePath_formatted = filePath != "Server Files"? filePath.mid(filePath.indexOf("/")).removeFirst() : ".";

        // Обработка события вставки
        if (!cutBuffer.isEmptyFile()) {
            // Перемещение
            if (filePath_formatted != cutBuffer.getPath())
            {
                ftp_api->moveFile(cutBuffer.getGoogleId(), filePath_formatted);
                cutBuffer = File();
            }
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
    int currentIndex = ui->storages_tabWidget->currentIndex();

    if (currentIndex == 1) {
        QString selectedFileName = ui->desurep_files->currentItem()->text(0);
        QString selectedFileId = ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();

        if (showDeleteConfirmationDialog(selectedFileName)) {
            webApi->deleteFile(*windControl->session, selectedFileId);
        }
    } else if (currentIndex == 2) {
        QString selectedFileName = ui->gdrive_files->currentItem()->text(0);
        QString selectedFileId = ui->gdrive_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();

        if (showDeleteConfirmationDialog(selectedFileName)) {
            google_api->deleteFile(selectedFileId);
        }
    } else if (currentIndex == 4) {
        QString selectedFileName = ui->ftp_files->currentItem()->text(0);
        QString selectedFileId = ui->ftp_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();

        if (showDeleteConfirmationDialog(selectedFileName)) {
            ftp_api->deleteFile(selectedFileId);
        }
    }
}

void MainWindow::on_actionDownload_triggered()
{
    // Обработка события загрузки файла
    QTreeWidget *activeStorage = nullptr;
    if (ui->storages_tabWidget->currentIndex() == 1) {
        activeStorage = ui->desurep_files;
    } else if (ui->storages_tabWidget->currentIndex() == 2) {
        activeStorage = ui->gdrive_files;
    } else if (ui->storages_tabWidget->currentIndex() == 4) {
        activeStorage = ui->ftp_files;
    }

    if (!activeStorage) return;

    QTreeWidgetItem *selectedItem = activeStorage->currentItem();

    if (!selectedItem) return;

    // Получение ссылки на скачивание и имени файла из пользовательских данных
    QString selectedFileId = activeStorage->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();
    QString fileName = selectedItem->text(0) + "." +
            File::getFileExtensionFromMimeType(selectedItem->data(0, Qt::UserRole).toJsonObject()["mime_type"].toString());
    QString savePath = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/Downloads/" + fileName);
    if (savePath.isEmpty()) return;

    // Выполнение скачивания файла
    showFileStatus("Downloading file from Storage:");
    fileOperationInProgress = true;

    if (ui->storages_tabWidget->currentIndex() == 1) {
        webApi->downloadFile(*windControl->session,
                             selectedFileId,
                             savePath,
                             ui->file_status_pb);
    } else if (ui->storages_tabWidget->currentIndex() == 2) {
        qreal fileSize = activeStorage->currentItem()->data(0, Qt::UserRole).toJsonObject()["size"].toDouble();
        google_api->downloadFile(selectedFileId, savePath, fileSize, ui->file_status_pb);
    } else if (ui->storages_tabWidget->currentIndex() == 4) {
        ftp_api->downloadFile(selectedFileId, savePath, ui->file_status_pb);
    }

}

void MainWindow::on_actionUpload_triggered()
{
    int currentIndex = ui->storages_tabWidget->currentIndex();

    // Обработка события выгрузки файла
    if (currentIndex == 1) {
        uploadDialog.setRestrictions(desuStorage->spaceAvailableMB());
        uploadDialog.setStorageType("ufh");
        if (uploadDialog.exec() == QDialog::Accepted)
        {
            showFileStatus("Uploading file to UFH Storage:");
            fileOperationInProgress = true;
            fileUploader = "ufh";
            webApi->uploadFile(*windControl->session,
                               uploadDialog.getUploadData(),
                               ui->file_status_pb);
        }
        else
        {
            fileUploader.clear();
            uploadDialog.clearData();
        }
    } else if (currentIndex == 2) {
        qreal usage = 15 * 1024 * (100 - ui->gdrive_info->value()) / 100;
        uploadDialog.setRestrictions(usage);
        uploadDialog.setStorageType("gdrive");
        if (uploadDialog.exec() == QDialog::Accepted)
        {
            showFileStatus("Uploading file to Google Drive:");
            fileOperationInProgress = true;
            fileUploader = "gdrive";
            google_api->uploadFile(uploadDialog.getUploadData(),
                                   ui->file_status_pb);
        }
        else
        {
            fileUploader.clear();
            uploadDialog.clearData();
        }
    } else if (currentIndex == 4) {
        uploadDialog.setRestrictions(ftpStorage->spaceAvailableMB());
        uploadDialog.setStorageType("ftp");
        if (uploadDialog.exec() == QDialog::Accepted)
        {
            showFileStatus("Uploading file to FTP server:");
            fileOperationInProgress = true;
            fileUploader = "ftp";
            ftp_api->uploadFile(uploadDialog.getUploadData(),
                                   ui->file_status_pb);
        }
        else
        {
            fileUploader.clear();
            uploadDialog.clearData();
        }
    }

}


void MainWindow::on_refresh_files_clicked()
{
    desuStorage->refreshFiles();
}

void MainWindow::on_refresh_files_gdrive_btn_clicked()
{
    if (google_api->isLinked) {
        gdriveStorage->refreshFiles();
    }
}

void MainWindow::on_refresh_files_ftp_btn_clicked()
{
    if (ftp_api->isConnectionSelected()) {
        ftpStorage->refreshFiles();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Please, select FTP-server first."));
    }
}


void MainWindow::on_exit_btn_clicked()
{
    delete windControl;
}

void MainWindow::on_so_btn_clicked()
{
    // Очистим рабочее окно
    ui->desurep_files->clear();
    ui->gdrive_files->clear();
    ui->ftp_files->clear();

    ui->desurep_file_info->clear();
    ui->gdrive_file_info->clear();
    ui->ftp_file_info->clear();

    ui->title_label->setText("Welcome, ");

    ui->desurep_files->headerItem()->setText(0, "UFH Storage");
    ui->gdrive_files->headerItem()->setText(0, "Google Drive");
    ui->ftp_files->headerItem()->setText(0, "FTP Server");


    // Очистим окно профиля
    ui->drep_info->setValue(0);
    ui->gdrive_info->setValue(0);
    ui->odrive_info->setValue(0);
    ui->ftp_info->setValue(0);
    ui->ftp_storage_selector->clear();


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

    // Очистим FTP
    ui->ftp_connection_cb->clear();
    ftp_api->clearConnections();
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
        google_api->clearData();
    }
}

// FTP
void MainWindow::on_ftp_connection_add_btn_clicked()
{
    bool added = addServerDialog.exec();

    if (added) {
        FTPConnection newConnection = addServerDialog.getData();
        webApi->addFtpConn(*windControl->session, *ftp_api, newConnection);
    }

}

void MainWindow::on_ftp_connection_cb_currentIndexChanged(int index)
{
    if (index != -1 && ftp_api->getConnections().toList().size() >= index)
    {
        ftp_api->setCurrentConnection(ftp_api->getConnections().toList()[index]);
        ftpStorage->refreshFiles();
    }
}

void MainWindow::on_ftp_storage_selector_currentIndexChanged(int index)
{
    if (index != -1 && ftp_api->getConnections().toList().size() >= index)
    {
        ui->ftp_info->setValue(ftpStorage->spaceUsage());
    }
}


void MainWindow::on_storages_tabWidget_currentChanged(int index)
{
    copyBuffer = File();
    cutBuffer = File();
}

