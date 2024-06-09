#include "addftpserver.h"
#include "QtCore/qjsonobject.h"

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

}

void AddFtpServerDialog::clearData()
{
    serverNameLineEdit->clear();
    serverFTPUserLineEdit->clear();
    serverFTPPasswordLineEdit->clear();
    serverFTPIPLineEdit->clear();
}


QJsonObject AddFtpServerDialog::getData() const
{
    return {{"name", serverNameLineEdit->text()},
            {"user", serverFTPUserLineEdit->text()},
            {"password", serverFTPPasswordLineEdit->text()},
            {"ip", serverFTPIPLineEdit->text()}};
}


void AddFtpServerDialog::cancel()
{
    reject();
}

void AddFtpServerDialog::addServer()
{
    accept();
}
