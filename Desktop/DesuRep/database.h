#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlField>
#include <QSqlRecord>
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QSqlError>
#include <QFileInfo>
#include <QUuid>

#include "user.h"
#include "enhasher.h"

class Database
{
public:
    Database();
    ~Database();

public:
    // Databases
    QList<int> getDBStatus() const { return {local.isOpen(), remote.isOpen()}; }

    // User Methods
    User *SignInUser(QString login, QString pass, QString mode);
    User *SignUpUser(QString username, QString email, QString pass);
    QList<QString> GetLocallySavedUsers();
    void SaveUserLocal(User::Data *data, QString mode);

    // Files Methods
    QList<File> *getFiles(QString uploader);
    void updateFile(QJsonObject editedData);
    bool uploadFile(File file_to_upload);
    bool deleteFile(int fileId);

private:
    QSqlDatabase local;
    QSqlDatabase remote;
};

#endif // DATABASE_H
