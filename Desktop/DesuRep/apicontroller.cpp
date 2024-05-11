#include "apicontroller.h"

ApiController::ApiController(QObject *parent) : QObject(parent)
{
    BASE_URL = "http://127.0.0.1:8888/";

    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiController::onRequestFinished);
}

void ApiController::onCopyFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Copy File", reply);
}

void ApiController::onDeleteFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Delete File", reply);
}

void ApiController::onUpdateFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Update File", reply);
}

void ApiController::onGetUserFilesFinished(QNetworkReply *reply, QList<File> &files)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject())
        {
            QJsonObject jsonObject = jsonDoc.object();

            if (jsonObject.contains("data"))
            {
                processFileData(jsonObject.value("data").toObject(), files);
            }
        }

        emit filesUpdated();
    }
    else
    {
        handleNetworkError("Get User Files", reply);
        return;
    }

    reply->deleteLater();
}

void ApiController::processFileData(const QJsonObject& dataObject, QList<File> &files)
{
    for (auto it = dataObject.begin(); it != dataObject.end(); ++it)
    {
        if (it.value().isObject())
        {
            QJsonObject fileObject = it.value().toObject();

            if (fileObject.contains("id") && fileObject.contains("publication_name"))
            {
                File newFile = createFileObject(fileObject);
                files.append(newFile);
            }
        }
    }
}

File ApiController::createFileObject(const QJsonObject& fileObject)
{
    QJsonObject data =
    {
        {"id", fileObject["id"].toInt()},
        {"Name", fileObject["publication_name"].toString()},
        {"Type", fileObject["doc_type"].toString()},
        {"Path", fileObject["folder_path"].toString()},
        {"Description", fileObject["description"].toString()},
        {"Author", fileObject["author_name"].toString()},
        {"Uploader", fileObject["uploader_name"].toString()},
        {"UploadDate", fileObject["upload_date"].toString().replace("T"," ")},
        {"Theme", fileObject["theme"].toString()},
        {"DownloadUrl", "http://desurep.lol/download_file/?id=" + fileObject["download_id"].toString()},
        {"Public", fileObject["is_public"].toBool()},
        {"Size", fileObject["file_size"].toDouble() / (1024 * 1024)}
    };

    return File(data, QByteArray());
}

void ApiController::onAuthenticateFinished(QNetworkReply *reply, const QString &username, User &session)
{
    if (reply->error() == QNetworkReply::NoError)
    {

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject())
        {
            QJsonObject jsonObject = jsonDoc.object();

            if (jsonObject.contains("is_auth"))
            {
                if (jsonObject.value("is_auth").toBool() == true) {
                    QUrl apiUrl(BASE_URL + "users/get_user_info");
                    QUrlQuery query;

                    query.addQueryItem("username", username);

                    apiUrl.setQuery(query);

                    QNetworkRequest request(apiUrl);
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                    QNetworkReply *reply = networkManager->post(request, QByteArray());

                    connect(reply, &QNetworkReply::finished, this, [this, reply, &session](){
                        onGetUserInfoFinished(reply, session);
                    });
                } else {
                    emit authFailed("User credentials invalid!");
                };
            }
        }
    }
    else
    {
        emit authFailed("Unknown error occured!");
        handleNetworkError("Auth User", reply);
        return;
    }

    reply->deleteLater();
}

void ApiController::onGetUserInfoFinished(QNetworkReply *reply, User &session)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject())
        {
            QJsonObject jsonObject = jsonDoc.object();

            if (jsonObject.contains("data"))
            {
                processUserData(jsonObject.value("data").toObject(), session);
            }
        }

        emit authSucceed();
    }
    else
    {
        emit authFailed("Unknown error occured!");
        handleNetworkError("Get User Info", reply);
        return;
    }

    reply->deleteLater();
}

void ApiController::processUserData(const QJsonObject& dataObject, User &session)
{
    for (auto it = dataObject.begin(); it != dataObject.end(); ++it)
    {
        if (it.value().isObject())
        {
            QJsonObject userObject = it.value().toObject();

            if (userObject.contains("id") && userObject.contains("username"))
            {
                session.setData(createUserDataObject(userObject));
            }
        }
    }
}

User::Data ApiController::createUserDataObject(const QJsonObject& userObject)
{
    User::Data data =
    {
        userObject["id"].toInt(),
        userObject["username"].toString(),
        userObject["email"].toString(),
        "passwordHash",
        int(userObject["on_active"].toBool()),
        int(userObject["is_banned"].toBool()),
        int(userObject["is_actual"].toBool()),
        userObject["role"].toInt()
    };

    return data;
}

void ApiController::onRequestFinished(QNetworkReply *reply)
{
    // Обработка других запросов, если необходимо
    Q_UNUSED(reply);
}

void ApiController::onEditUserFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        emit userEditSucceed();
    }
    else
    {
        emit userEditFailed();
        handleNetworkError("Edit User", reply);
        return;
    }

    reply->deleteLater();
}


void ApiController::handleApiResponse(const QString &operation, QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        emit refreshDesuFiles();
    }
    else
    {
        handleNetworkError(operation, reply);
    }

    reply->deleteLater();
}

void ApiController::handleNetworkError(const QString &operation, QNetworkReply *reply)
{
    qDebug() << operation << " File Error Code: " << reply->error();
    qDebug() << operation << " File Error String: " << reply->errorString();
    qDebug() << "Server Response: " << reply->readAll();
}

void ApiController::authenticate(const QString &username, const QString &password, User &session)
{
    QUrl apiUrl(BASE_URL + "users/auth_user");
    QUrlQuery query;

    query.addQueryItem("username", username);
    query.addQueryItem("password", password);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, username, &session](){
        onAuthenticateFinished(reply, username, session);
    });
}

void ApiController::copyFile(const QString &username, const QString &fileId, const QString &folderPath)
{
    QUrl apiUrl(BASE_URL + "files/copy_file");
    QUrlQuery query;

    query.addQueryItem("file_id", fileId);
    query.addQueryItem("folder_path", folderPath);
    query.addQueryItem("username", username);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        onCopyFileFinished(reply);
    });
}

void ApiController::deleteFile(const QString &fileId)
{
    QUrl apiUrl(BASE_URL + "files/delete_file");
    QUrlQuery query;

    query.addQueryItem("file_id", fileId);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onDeleteFileFinished(reply);
    });
}


void ApiController::updateFile(const QJsonObject &metaData)
{
    QUrl apiUrl(BASE_URL + "files/update_file");
    QUrlQuery query;

    query.addQueryItem("file_id", QString::number(metaData["id"].toInt()));
    query.addQueryItem("author_name", metaData["Author"].toString());
    query.addQueryItem("publication_name", metaData["Name"].toString());
    query.addQueryItem("theme", metaData["Theme"].toString());
    query.addQueryItem("publication_date", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addQueryItem("description", metaData["Description"].toString());
    query.addQueryItem("folder_path", metaData["Path"].toString());
    query.addQueryItem("is_public", QString::number(metaData["Public"].toInt()));
    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUpdateFileFinished(reply);
    });
}

void addTextPart(QHttpMultiPart* multiPart, const QString& name, const QString& value) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + name + "\"");
    part.setBody(value.toUtf8());
    multiPart->append(part);
}

//void ApiController::uploadFile(const File &uploadFile)
//{
//    QJsonObject metaData = uploadFile.getMetaData();

//    QNetworkAccessManager manager;
//    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

//    QHttpPart filePart;
//    QString fullFileName = metaData["Name"].toString() + '.' + File::getFileExtensionFromMimeType(metaData["Type"].toString());
//    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
//                       QVariant("form-data; name=\"input_data\"; filename=\"" + fullFileName + "\""));
//    QBuffer *file = new QBuffer();
//    file->setData(uploadFile.getBlob());
//    file->open(QIODevice::ReadOnly);
//    filePart.setBodyDevice(file);
//    file->setParent(multiPart);
//    multiPart->append(filePart);

//    QUrl url(BASE_URL + "files/upload_file");
//    QUrlQuery query;
//    query.addQueryItem("download_id", QUuid::createUuid().toString().mid(1, 36));
//    query.addQueryItem("filename", QUuid::createUuid().toString().mid(1, 36) + metaData["Name"].toString());
//    query.addQueryItem("filename_full", fullFileName);
//    query.addQueryItem("author_name", metaData["Author"].toString());
//    query.addQueryItem("publication_name", metaData["Name"].toString());
//    query.addQueryItem("theme", metaData["Theme"].toString());
//    query.addQueryItem("publication_date", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
//    query.addQueryItem("description", metaData["Description"].toString());
//    query.addQueryItem("upload_date", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
//    query.addQueryItem("uploader_name", metaData["Uploader"].toString());
//    query.addQueryItem("doc_type", metaData["Type"].toString());
//    query.addQueryItem("is_public", QString::number(metaData["Public"].toInt()));
//    query.addQueryItem("folder_path", metaData["Path"].toString());
//    url.setQuery(query);

//    QNetworkRequest request(url);

//    QNetworkReply *reply = manager.post(request, multiPart);
//    multiPart->setParent(reply);

//    QEventLoop loop;
//    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
//    loop.exec();

//    if (reply->error() == QNetworkReply::NoError) {
//        emit uploadSucceed();
//    } else {
//        emit uploadFailed();
//        handleNetworkError("Upload File", reply);
//        return;
//    }

//    reply->deleteLater();
//}

void ApiController::uploadFile(const File &uploadFile)
{
    QJsonObject dataToUpload = uploadFile.getMetaData();
    QJsonObject metaData;
    QNetworkAccessManager manager;
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    QString fullFileName = dataToUpload["Name"].toString() + '.' + File::getFileExtensionFromMimeType(dataToUpload["Type"].toString());
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"input_data\"; filename=\"" + fullFileName + "\""));
    qDebug() << fullFileName;
    QBuffer *file = new QBuffer();
    file->setData(uploadFile.getBlob());
    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    metaData["download_id"] = QUuid::createUuid().toString().mid(1, 36);
    metaData["filename"] = QUuid::createUuid().toString().mid(1, 36) + dataToUpload["Name"].toString();
    metaData["filename_full"] = fullFileName;
    metaData["author_name"] = dataToUpload["Author"].toString();
    metaData["publication_name"] = dataToUpload["Name"].toString();
    metaData["theme"] = dataToUpload["Theme"].toString();
    metaData["publication_date"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    metaData["uploader_name"] = dataToUpload["Uploader"].toString();
    metaData["description"] = dataToUpload["Description"].toString();
    metaData["upload_date"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    metaData["doc_type"] = dataToUpload["Type"].toString();
    metaData["is_public"] = QString::number(dataToUpload["Public"].toInt());
    metaData["folder_path"] = dataToUpload["Path"].toString();

    QJsonDocument jsonDocument(metaData);
    QByteArray metaDataJson = jsonDocument.toJson();

    QHttpPart metaDataPart;
    metaDataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    metaDataPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"metadata\""));
    metaDataPart.setBody(metaDataJson);
    multiPart->append(metaDataPart);

    QUrl url(BASE_URL + "files/upload_file");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        emit uploadSucceed();
    } else {
        emit uploadFailed();
        handleNetworkError("Upload File", reply);
    }

    reply->deleteLater();
}


void ApiController::getUserFiles(const QString &username, QList<File> &files)
{
    files.clear();

    QUrl apiUrl(BASE_URL + "files/get_user_files_metadata");
    QUrlQuery query;

    query.addQueryItem("username", username);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, &files]() {
        onGetUserFilesFinished(reply, files);
    });
}

void ApiController::editUser(const QString &id, const QString &email)
{
    QUrl apiUrl(BASE_URL + "users/edit_user");
    QUrlQuery query;

    query.addQueryItem("user_id", id);
    query.addQueryItem("email", email);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onEditUserFinished(reply);
    });
}
