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
    QString getUserToken(const QString username, const QString password);
    QList<QString> getLocallySavedUserSessions();
    void saveUserSessionLocal(User::Data *data, const QString &token);
    void updateUserPasswordLocal(const QString username, const QString password);

private:
    QSqlDatabase local;
};

#endif // DATABASE_H
