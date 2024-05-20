#include "database.h"
#include "QtSql/qsqlquery.h"


Database::Database()
{
    local = QSqlDatabase::addDatabase("QSQLITE", "local");
    local.setDatabaseName(PROJECT_PATH "resources/local.db");

    if (!local.open())
    {
        qDebug() << "Failed to connect to the database: " << local.lastError().text();
    }
    else
    {
        qDebug() << "Connected to the local database";
    }
}

Database::~Database()
{
    if (local.isOpen()) { local.close(); }
}


// Auth Methods
QString Database::getUserToken(const QString username, const QString password)
{
    QString query = "SELECT * FROM users WHERE username = :username";

    QSqlQuery findUser(local);

    findUser.prepare(query);
    findUser.bindValue(":username", username);
    findUser.exec();

    if (findUser.next())
    {
        QString storedPassHash = findUser.value("hashed_password").toString();

        if (!Enhasher::checkPassword(password, storedPassHash))
        {
            return "ERROR_Provided password is incorrect! Please check your input.";
        }

        return findUser.value("session_token").toString();
    }

    return "ERROR_Bad session.";
}

void Database::saveUserSessionLocal(User::Data *data, const QString &token)
{
    QString data_query = "SELECT * FROM users WHERE username = :username";
    QSqlDatabase &src = local;

    QSqlQuery user_data(src);
    user_data.prepare(data_query);
    user_data.bindValue(":username", data->username);

    user_data.exec();

    if (user_data.next())
    {
        QString update_query = "UPDATE users "
                               "SET hashed_password = :hashed_password, "
                               "session_token = :session_token "
                               "WHERE username = :username";

        QSqlQuery updateSavedUser(local);
        updateSavedUser.prepare(update_query);
        updateSavedUser.bindValue(":hashed_password", data->password);
        updateSavedUser.bindValue(":session_token", token);
        updateSavedUser.bindValue(":username", data->username);
        updateSavedUser.exec();
    }
    else
    {
        QString query = "INSERT INTO users (username, hashed_password, session_token) "
                        "VALUES (:username, :hashed_password, :session_token)";

        QSqlQuery addUser(local);
        addUser.prepare(query);
        addUser.bindValue(":username", data->username);
        addUser.bindValue(":hashed_password", data->password);
        addUser.bindValue(":session_token", token);

        addUser.exec();

        return;
    }
}

void Database::updateUserPasswordLocal(const QString username, const QString password)
{
    QString data_query = "SELECT * FROM users WHERE username = :username";
    QSqlDatabase &src = local;

    QSqlQuery user_data(src);
    user_data.prepare(data_query);
    user_data.bindValue(":username", username);

    user_data.exec();

    if (user_data.next())
    {
        QString update_query = "UPDATE users "
                               "SET hashed_password = :hashed_password, "
                               "WHERE username = :username";

        QSqlQuery updateSavedUser(local);
        updateSavedUser.prepare(update_query);
        updateSavedUser.bindValue(":hashed_password", password);
        updateSavedUser.exec();
    }
    else
    {
        qDebug() << "User not found in LocalDB error.";
    }

}

QList<QString> Database::getLocallySavedUserSessions()
{
    QString query = "SELECT username FROM users ";

    QSqlQueryModel getLocalUsers;
    getLocalUsers.setQuery(query, local);
    while (getLocalUsers.canFetchMore())
        getLocalUsers.fetchMore();

    QList<QString> usersList;

    if (getLocalUsers.rowCount() >= 1)
    {
        for (int i = 0; i < getLocalUsers.rowCount(); i++)
        {
            usersList.append(getLocalUsers.record(i).field(0).value().toString());
        }
    }

    return usersList;
}
