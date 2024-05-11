#include "database.h"
#include "QtSql/qsqlquery.h"


Database::Database()
{
    local = QSqlDatabase::addDatabase("QSQLITE", "local");
    local.setDatabaseName(PROJECT_PATH "resources/local.db");

    if (!local.open())
    {
        qDebug() << "Failed to connect to the database: " << local.lastError().text();
    } else {
        qDebug() << "Connected to the local database";
    }

    // Set the database connection parameters
    remote = QSqlDatabase::addDatabase("QMYSQL", "remote");
    remote.setHostName("127.0.0.1");
    remote.setPort(3307);
    remote.setDatabaseName("db");
    remote.setUserName("user");
    remote.setPassword("password");

    if (!remote.open()) {
        qDebug() << "Failed to connect to the database: " << remote.lastError().text();
    } else {
        qDebug() << "Connected to the remote database";
    }

}

Database::~Database()
{
    if (local.isOpen()){ local.close(); }
    if (remote.isOpen()){ remote.close(); }
}


// Auth Methods

User *Database::SignInUser(QString username, QString pass, QString mode)
{
    QString query = "SELECT * FROM users WHERE username = :username";

    QSqlDatabase *src = (mode == "offline") ? &local : &remote;

    QSqlQuery findUser(*src);
    findUser.prepare(query);
    findUser.bindValue(":username", username);
    findUser.exec();

    if (findUser.next())
    {
        QString storedPassHash = findUser.value("password").toString();

        if (!Enhasher::checkPassword(pass, storedPassHash)) {
            QMessageBox::critical(0, "", "⚠ Sign in failed!\n\nUser credentials invalid! "
                                         "Please check your input.");
            return nullptr;
        }

        int id = findUser.value("id").toInt();

        QString data_query = "SELECT * FROM users WHERE id = :id";
        QSqlQuery user_data(*src);
        user_data.prepare(data_query);
        user_data.bindValue(":id", id);
        user_data.exec();

        if (user_data.next())
        {
            QString email = user_data.value("email").toString();
            int on_active = user_data.value("on_active").toInt();
            int is_banned = user_data.value("is_banned").toInt();
            int is_actual = user_data.value("is_actual").toInt();
            int role = user_data.value("role").toInt();

            User::Data data =
            {
                id,
                username,
                email,
                storedPassHash,
                on_active,
                is_banned,
                is_actual,
                role
            };

            if (data.is_actual != 1) {
                QMessageBox::critical(0, "", "⚠ Sign in failed!\n\nUser credentials expired! "
                                             "Please try signing in online.");
                return nullptr;
            }

            // Update local
            SaveUserLocal(&data, mode);

            if (data.is_actual != 1) {
                QMessageBox::critical(0, "", "⚠ Sign in failed!\n\nUser credentials expired! "
                                             "Please try signing in online.");
                return nullptr;
            }

            return new User(data);
        }
    }

    QMessageBox::critical(0, "", "⚠ Sign in failed!\n\nUser credentials invalid! "
                                  "Please check your input.");

    return nullptr;
}

User *Database::SignUpUser(QString username, QString email, QString pass)
{
    QString checkQuery = "SELECT * FROM users WHERE username = :username";
    QSqlQuery checkUser(remote);
    checkUser.prepare(checkQuery);
    checkUser.bindValue(":username", username);
    checkUser.exec();

    if (checkUser.next())
    {
        // The username already exists
        QMessageBox::critical(0, "", "⚠ Sign up failed!\n\nUsername already exists. Please choose a different username.");
        return nullptr;
    }

    QString passHash = Enhasher::hashPassword(pass);
    QString query = "INSERT INTO users (username, email, password, on_active, is_banned, is_actual, role) "
                    "VALUES (:username, :email, :password, 1, 0, 1, 3)";

    remote.transaction();

    QSqlQuery addUser(remote);
    addUser.prepare(query);
    addUser.bindValue(":username", username);
    addUser.bindValue(":email", email);
    addUser.bindValue(":password", passHash);

    bool success = addUser.exec();

    if (success)
    {
        // Retrieve the last inserted id
        int id = addUser.lastInsertId().toInt();

        QString data_query = "SELECT * FROM users WHERE id = :id";
        QSqlQuery user_data(remote);
        user_data.prepare(data_query);
        user_data.bindValue(":id", id);
        user_data.exec();

        if (user_data.next())
        {
            User::Data data =
            {
                id,
                user_data.value("username").toString(),
                user_data.value("email").toString(),
                user_data.value("password").toString(),
                user_data.value("on_active").toInt(),
                user_data.value("is_banned").toInt(),
                user_data.value("is_actual").toInt(),
                user_data.value("role").toInt()
            };

            // Update local
            SaveUserLocal(&data, "online");

            remote.commit(); // Commit the transaction

            return new User(data);
        }
    }

    remote.rollback(); // Rollback the transaction
    QMessageBox::critical(0, "", "⚠ Sign up failed!\n\nError creating a new account occurred! "
                                  "Please check your input.");

    return nullptr;
}

void Database::SaveUserLocal(User::Data *data, QString mode) {
    QString data_query = "SELECT * FROM users WHERE id = :id";
    QSqlDatabase &src = (mode == "online") ? local : remote;

    QSqlQuery user_data(src);
    user_data.prepare(data_query);
    user_data.bindValue(":id", data->id);

    user_data.exec();

    if (user_data.next())
    {
        // User saved, data differs
        if  (user_data.value("username").toString() != data->username ||
             user_data.value("email").toString() != data->email ||
             user_data.value("password").toString() != data->pass ||
             user_data.value("on_active").toInt() != data->on_active ||
             user_data.value("is_banned").toInt() != data->is_banned ||
             user_data.value("is_actual").toInt() != data->is_actual ||
             user_data.value("role").toInt() != data->role) {

            if (mode == "online") {
                QString update_query = "UPDATE users "
                                       "SET username = :username, email = :email, password = :password, "
                                       "on_active = 1, is_banned = 0, is_actual = 1, role = 3 "
                                       "WHERE id = :id";

                QSqlQuery updateSavedUser(local);
                updateSavedUser.prepare(update_query);
                updateSavedUser.bindValue(":username", data->username);
                updateSavedUser.bindValue(":email", data->email);
                updateSavedUser.bindValue(":password", data->pass);
                updateSavedUser.bindValue(":id", data->id);

                updateSavedUser.exec();
            } else if (mode == "offline") {
                data->is_actual = 0;
                QString update_query = "UPDATE users "
                                       "SET is_actual = :is_actual "
                                       "WHERE id = :id";
                QSqlQuery updateSavedUser(local);
                updateSavedUser.prepare(update_query);
                updateSavedUser.bindValue(":is_actual", data->is_actual);
                updateSavedUser.bindValue(":id", data->id);
                updateSavedUser.exec();
            }
            return;

        } else {
            // User saved, data the same
            return;
        }

    } else {
        // No user saved, adding one to local
        if (mode == "online") {
            QString query = "INSERT INTO users (id, username, email, password, on_active, is_banned, is_actual, role) "
                            "VALUES (:id, :username, :email, :password, 1, 0, 1, 3)";

            QSqlQuery addUser(local);
            addUser.prepare(query);
            addUser.bindValue(":id", data->id);
            addUser.bindValue(":username", data->username);
            addUser.bindValue(":email", data->email);
            addUser.bindValue(":password", data->pass);

            addUser.exec();
        } else if (mode == "offline") {
            data->is_actual = 0;
            QString update_query = "UPDATE users "
                                   "SET is_actual = :is_actual "
                                   "WHERE id = :id";
            QSqlQuery updateSavedUser(local);
            updateSavedUser.prepare(update_query);
            updateSavedUser.bindValue(":is_actual", data->is_actual);
            updateSavedUser.bindValue(":id", data->id);
            updateSavedUser.exec();
        }

        return;
    }
}

QList<QString> Database::GetLocallySavedUsers()
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

// Files Methods
QList<File> *Database::getFiles(QString uploader)
{
    QString data_query = "SELECT * FROM files WHERE uploader_name = :uploader OR is_public = 1";

    QSqlQuery files_data(remote);
    files_data.prepare(data_query);
    files_data.bindValue(":uploader", uploader);
    files_data.exec();

    QList<File> *files = new QList<File>;

    while (files_data.next())
    {
        QJsonObject data =
        {
            {"id", files_data.value("id").toInt()},
            {"Name", files_data.value("publication_name").toString()},
            {"Type", files_data.value("doc_type").toString()},
            {"Path", files_data.value("folder_path").toString()},
            {"Description", files_data.value("description").toString()},
            {"Author", files_data.value("author_name").toString()},
            {"Uploader", files_data.value("uploader_name").toString()},
            {"UploadDate", files_data.value("upload_date").toString()},
            {"Theme", files_data.value("theme").toString()},
            {"DownloadUrl", "http://desurep.lol/download_file/?id=" + files_data.value("download_id").toString()},
            {"Public", files_data.value("is_public").toBool()}
        };


        File newFile = File(data, files_data.value("data").toByteArray());

        files->append(newFile);
    }

    return files;
}

void Database::updateFile(QJsonObject editedData)
{
    QSqlQuery update_file(remote);
    update_file.prepare("UPDATE files SET publication_name = :publication_name, "
                  "folder_path = :folder_path, description = :description, "
                  "author_name = :author_name,  theme = :theme, is_public = :is_public WHERE id = :id");

    // Bind the values to the query parameters
    update_file.bindValue(":id", editedData["id"].toInt());
    update_file.bindValue(":folder_path", editedData["Path"].toString());
    update_file.bindValue(":publication_name", editedData["Name"].toString());
    update_file.bindValue(":description", editedData["Description"].toString());
    update_file.bindValue(":author_name", editedData["Author"].toString());
    update_file.bindValue(":theme", editedData["Theme"].toString());
    update_file.bindValue(":is_public", editedData["Public"].toInt());

    update_file.exec();
}

bool Database::uploadFile(File file_to_upload)
{
    QJsonObject data = file_to_upload.getMetaData();
    QString insert_query = "INSERT INTO files (filename, download_id, author_name,"
                           " publication_name, folder_path, theme, publication_date, description,"
                           " upload_date, uploader_name, doc_type, is_public, data) "
                           "VALUES (:filename, :download_id, :author_name, :publication_name, "
                           ":folder_path, :theme, :publication_date, :description, :upload_date, "
                           ":uploader_name, :doc_type, :is_public, :data)";

    QSqlQuery add_file(remote);
    add_file.prepare(insert_query);

    add_file.bindValue(":filename", QUuid::createUuid().toString().mid(1, 36) + data["Name"].toString());
    add_file.bindValue(":download_id", QUuid::createUuid().toString().mid(1, 36));
    add_file.bindValue(":author_name", data["Author"].toString());
    add_file.bindValue(":folder_path", data["Path"].toString().isEmpty()? "." : data["Path"].toString());
    add_file.bindValue(":publication_name", data["Name"].toString());
    add_file.bindValue(":theme", data["Theme"].toString());
    add_file.bindValue(":publication_date", QDate::currentDate().toString("yyyy-MM-dd"));
    add_file.bindValue(":description", data["Description"].toString());
    add_file.bindValue(":upload_date", QDate::currentDate().toString("yyyy-MM-dd"));
    add_file.bindValue(":uploader_name", data["Uploader"].toString());
    add_file.bindValue(":doc_type", data["Type"].toString());
    add_file.bindValue(":is_public", data["Public"].toInt());
    add_file.bindValue(":data", file_to_upload.getBlob());

    return add_file.exec();
}

bool Database::deleteFile(int fileId)
{
    QString remove_query = "DELETE FROM files WHERE id = :id";
    QSqlQuery remove_file(remote);
    remove_file.prepare(remove_query);
    remove_file.bindValue(":id", fileId);

    return remove_file.exec();
}
