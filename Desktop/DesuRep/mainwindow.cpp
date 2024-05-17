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

    // Сигналы изменения почты
    connect(webApi, &ApiController::userEditSucceed, this, [this]() {
        windControl->session->setEmail(changedEmail);
        changedEmail.clear();
    });
    connect(webApi, &ApiController::userEditFailed, this, [this]() {
        QMessageBox::warning(this, tr("Error"), tr("Failed to change profile info."));
        changedEmail.clear();
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
}

void MainWindow::on_actionCopy_triggered() {
    // Обработка события копирования
    QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
    copyBuffer = findFileById(selectedFileId, desuStorage->getFiles());

    copyBuffer.setOwner(windControl->session->getId());
    if (!cutBuffer.isEmptyFile()) { cutBuffer = File(); }
}

void MainWindow::on_actionCut_triggered() {
    // Обработка события вырезания
    QUuid selectedFileId = QUuid(ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString());
    cutBuffer = findFileById(selectedFileId, desuStorage->getFiles());

    if (!copyBuffer.isEmptyFile()) { copyBuffer = File(); }
}

void MainWindow::on_actionPaste_triggered() {
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


File MainWindow::findFileById(QUuid id, const QList<File> *fileList) {
    for (const File &file : *fileList) {
        if (file.getId() == id) {
            return file;
        }
    }

    return File();
}

void MainWindow::on_actionEdit_triggered() {
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

bool MainWindow::showDeleteConfirmationDialog(QString fileName) {
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

bool MainWindow::showChangeEmailDialog() {
    QDialog changeEmailDialog(this);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&changeEmailDialog);
    dialogLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLabel *label = new QLabel("Current Email: " +  windControl->session->getEmail(), &changeEmailDialog);
    dialogLayout->addWidget(label);

    QLineEdit *newEmailLineEdit = new QLineEdit(&changeEmailDialog);
    newEmailLineEdit->setPlaceholderText("email@example.com");
    dialogLayout->addWidget(newEmailLineEdit);

    QPushButton *confirmButton = new QPushButton("Change Email", &changeEmailDialog);
    dialogLayout->addWidget(confirmButton);
    connect(confirmButton, &QPushButton::clicked, &changeEmailDialog, [this, &newEmailLineEdit, &changeEmailDialog]() {
        changedEmail = newEmailLineEdit->text();
        changeEmailDialog.accept();
    });

    QPushButton *cancelButton = new QPushButton("Cancel", &changeEmailDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &changeEmailDialog, &QDialog::reject);

    return changeEmailDialog.exec() == QDialog::Accepted;
}


bool MainWindow::showChangePasswordDialog() {
    QDialog changePasswordDialog(this);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&changePasswordDialog);
    dialogLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLineEdit *currentPasswordLineEdit = new QLineEdit(&changePasswordDialog);
    currentPasswordLineEdit->setPlaceholderText("Enter current password");
    currentPasswordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(currentPasswordLineEdit);

    QLineEdit *newPasswordLineEdit = new QLineEdit(&changePasswordDialog);
    newPasswordLineEdit->setPlaceholderText("Enter new password");
    newPasswordLineEdit->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(newPasswordLineEdit);

    QPushButton *confirmButton = new QPushButton("Change Password", &changePasswordDialog);
    dialogLayout->addWidget(confirmButton);
    connect(confirmButton, &QPushButton::clicked, &changePasswordDialog, &QDialog::accept);

    QPushButton *cancelButton = new QPushButton("Cancel", &changePasswordDialog);
    dialogLayout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, &changePasswordDialog, &QDialog::reject);

    return changePasswordDialog.exec() == QDialog::Accepted;
}

void MainWindow::on_actionDelete_triggered() {
    QString selectedFileId = ui->desurep_files->currentItem()->data(0, Qt::UserRole).toJsonObject()["id"].toString();

    // Обработка события удаления файла
    if (showDeleteConfirmationDialog(ui->desurep_files->currentItem()->text(0))) {
        webApi->deleteFile(*windControl->session, selectedFileId);
    }
}

void MainWindow::on_actionDownload_triggered() {
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
}


void MainWindow::on_edit_profile_btn_clicked()
{
    if (showChangeEmailDialog()) {
        if (!changedEmail.isEmpty() && changedEmail != windControl->session->getEmail()) {
            webApi->editUser(*windControl->session, changedEmail);
        } else {
            QMessageBox::critical(this, "", "⚠ Bad Input!\n\nNew email is invalid! "
                                         "Please check your input.");
        }
    }

}


void MainWindow::on_change_pass_btn_clicked()
{

}

