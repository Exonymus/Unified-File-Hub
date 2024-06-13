#ifndef ADDFTPSERVER_H
#define ADDFTPSERVER_H

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include "QCheckBox"
#include <QVBoxLayout>
#include "QJsonObject"

#include "globals.h"

class AddFtpServerDialog : public QDialog
{
    Q_OBJECT

public:
    AddFtpServerDialog(QWidget *parent = nullptr);
    FTPConnection getData() const;
    void clearData();

private slots:
    void cancel();
    void addServer();

private:
    QLineEdit *serverNameLineEdit;
    QLineEdit *serverFTPUserLineEdit;
    QLineEdit *serverFTPPasswordLineEdit;
    QLineEdit *serverFTPIPLineEdit;
};
class AddFTPServer
{
public:
    AddFTPServer();
};

#endif // ADDFTPSERVER_H
