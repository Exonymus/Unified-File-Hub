#include "addftpserver.h"

AddFtpServerDialog::AddFtpServerDialog(QWidget *parent) : QDialog(parent)
{
    QFormLayout *formLayout = new QFormLayout(this);

    // Server Name
    serverNameLineEdit = new QLineEdit(this);
    serverNameLineEdit->setMaximumWidth(260);
    serverNameLineEdit->setMinimumWidth(260);
    formLayout->addRow("Name:", serverNameLineEdit);

    // Server username
    serverFTPUserLineEdit = new QLineEdit(this);
    serverFTPUserLineEdit->setMaximumWidth(260);
    serverFTPUserLineEdit->setMinimumWidth(260);
    formLayout->addRow("FTP User:", serverFTPUserLineEdit);

    // Server password
    serverFTPPasswordLineEdit = new QLineEdit(this);
    serverFTPPasswordLineEdit->setEchoMode(QLineEdit::Password);
    serverFTPPasswordLineEdit->setMaximumWidth(260);
    serverFTPPasswordLineEdit->setMinimumWidth(260);
    formLayout->addRow("FTP Password:", serverFTPPasswordLineEdit);

    // Server IP
    serverFTPIPLineEdit = new QLineEdit(this);
    serverFTPIPLineEdit->setMaximumWidth(260);
    serverFTPIPLineEdit->setMinimumWidth(260);
    formLayout->addRow("FTP IP:", serverFTPIPLineEdit);

    // Add Server
    QPushButton *addServerBtn = new QPushButton("Add", this);
    connect(addServerBtn, &QPushButton::clicked, this, &AddFtpServerDialog::addServer);
    formLayout->addRow(addServerBtn);

    // Cancel
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &AddFtpServerDialog::cancel);
    formLayout->addRow(cancelBtn);

    // Set up the dialog window
    setWindowTitle("Add FTP Server");
    setMinimumWidth(400);
    setMaximumWidth(400);
    setMinimumHeight(220);
    setMaximumHeight(220);

    connect(ftp_api, &FTPController::connectionTested, this, [this](bool success) {
        if (success)
        {
            accept();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to access the server.\n"
                                                       "Please check up provided info."));
        }
    });
}

void AddFtpServerDialog::clearData()
{
    serverNameLineEdit->clear();
    serverFTPUserLineEdit->clear();
    serverFTPPasswordLineEdit->clear();
    serverFTPIPLineEdit->clear();
}


FTPConnection AddFtpServerDialog::getData() const
{
    return {QUuid::createUuid().toString(),
            serverNameLineEdit->text(),
            serverFTPIPLineEdit->text(),
            serverFTPUserLineEdit->text(),
            serverFTPPasswordLineEdit->text()};
}


void AddFtpServerDialog::cancel()
{
    clearData();
    reject();
}

void AddFtpServerDialog::addServer()
{
    ftp_api->testConnection(FTPConnection(getData()));
}
