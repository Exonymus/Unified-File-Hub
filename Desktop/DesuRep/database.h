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
    // User Methods
    QString getUserToken(QString username, QString password);

    User *SignInUser(QString login, QString pass, QString mode);
    User *SignUpUser(QString username, QString email, QString pass);
    QList<QString> getLocallySavedUserSessions();
    void saveUserSessionLocal(User::Data *data, const QString &token);

private:
    QSqlDatabase local;
};

#endif // DATABASE_H
