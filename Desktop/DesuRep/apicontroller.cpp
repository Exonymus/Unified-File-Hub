#include "apicontroller.h"

ApiController::ApiController(QObject *parent) : QObject(parent)
{
    BASE_URL = "http://127.0.0.1:8888/";

    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiController::onRequestFinished);
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
    qDebug()<<"|----- API REQUEST ERROR ----->";
    qDebug() << "↳ Requester: " << operation;
    qDebug() << "↳ Error code: " << reply->error();
    qDebug() << "↳ Server response: " << reply->readAll();
}

void ApiController::onRequestFinished(QNetworkReply *reply)
{
    // Обработка других запросов, если необходимо
    Q_UNUSED(reply);
}

void addTextPart(QHttpMultiPart* multiPart, const QString& name, const QString& value) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + name + "\"");
    part.setBody(value.toUtf8());
    multiPart->append(part);
}


// Authenticating User and receiving token
void ApiController::authenticate(const QString &username, const QString &password, User &session)
{
    QUrl apiUrl(BASE_URL + "users/signin");
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Prepare form data
    QByteArray postData;
    postData.append("username=" + username.toUtf8());
    postData.append("&password=" + password.toUtf8());

    QNetworkReply *reply = networkManager->post(request, postData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, username, &session](){
        onAuthenticateFinished(reply, session);
    });
}

void ApiController::onAuthenticateFinished(QNetworkReply *reply, User &session)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid())
    {
        if (statusCode.toInt() == 200)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (jsonDoc.isObject())
            {
                QJsonObject jsonObject = jsonDoc.object();

                if (jsonObject.contains("access_token") && !jsonObject.value("access_token").toString().isEmpty())
                {
                    session.setTokenValue(jsonObject.value("access_token").toString());
                    QUrl apiUrl(BASE_URL + "users/get_info");

                    QNetworkRequest request(apiUrl);
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                    request.setRawHeader(QByteArray("Authorization"),
                                         QString("bearer %1").arg(session.getToken()).toUtf8());


                    QNetworkReply *userInfoReply = networkManager->get(request);

                    connect(userInfoReply, &QNetworkReply::finished, this, [this, userInfoReply, &session](){
                        onGetUserInfoFinished(userInfoReply, session);
                    });
                    reply->deleteLater();
                    return;
                }
            }
            emit authFailed("Unknown error occured!");
            handleNetworkError("Auth User", reply);
        }
        else if (statusCode.toInt() == 403)
        {
            emit authFailed("This account is banned!");
        }
        else if (statusCode.toInt() == 401 || statusCode.toInt() == 404)
        {
            emit authFailed("User credentials invalid!");
        }
        else
        {
            emit authFailed("Unknown error occured!");
            handleNetworkError("Auth User", reply);
        }
    }

    reply->deleteLater();
}

void ApiController::onGetUserInfoFinished(QNetworkReply *reply, User &session)
{
    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (reply->error() != QNetworkReply::NoError || !jsonDoc.isObject()) {
        emit authFailed("Unknown error occurred!");
        handleNetworkError("Get User Info", reply);
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObject = jsonDoc.object();
    if (!jsonObject.contains("id") || jsonObject.value("id").toString().isEmpty()) {
        emit authFailed("Unknown error occurred!");
        handleNetworkError("Get User Info", reply);
        reply->deleteLater();
        return;
    }

    session.setData(createUserDataObject(jsonObject));
    emit authSucceed();

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
        QUuid(userObject["id"].toString()),
        userObject["username"].toString(),
        userObject["email"].toString(),
        userObject["secret_num"].toInt(),
        userObject["secret_answer"].toString(),
        "passwordHash",
        int(userObject["is_banned"].toBool()),
    };

    return data;
}


// Editing User metadata in UFH Storage
void ApiController::editUser(User &session, const QString &email)
{
    QUrl apiUrl(BASE_URL + "users/edit");
    QUrlQuery query;

    query.addQueryItem("email", email);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onEditUserFinished(reply);
    });
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
    }

    reply->deleteLater();
}


// Getting User files in UFH Storage
void ApiController::getUserFiles(User &session, QList<File> &files)
{
    files.clear();

    QUrl apiUrl(BASE_URL + "files/get_user_files");
    QNetworkRequest request(apiUrl);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, &files]() {
        onGetUserFilesFinished(reply, files);
    });
}

void ApiController::onGetUserFilesFinished(QNetworkReply *reply, QList<File> &files)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid())
    {
        if (statusCode.toInt() == 200)
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
        }
    }
    else
    {
        handleNetworkError("Get User Files", reply);
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

            if (fileObject.contains("id") && !fileObject.value("id").toString().isEmpty())
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
        {"id", fileObject["id"].toString()},
        {"name", fileObject["name"].toString()},
        {"path", fileObject["path"].toString()},
        {"mime_type", fileObject["mime_type"].toString()},
        {"description", fileObject["description"].toString()},
        {"author", fileObject["author"].toString()},
        {"theme", fileObject["theme"].toString()},
        {"is_public", fileObject["is_public"].toBool()},
        {"owner_id", fileObject["owner_id"].toString()},
        {"upload_date", fileObject["upload_date"].toString().replace("T"," ")},
        {"size", fileObject["size"].toDouble() / (1024 * 1024)}
    };

    return File(data);
}


// Uploading User file to UFH Storage
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
    //file->setData(uploadFile.getBlob());
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


// Downloading User file from UFH Storage
void ApiController::downloadFile(User &session, const QString &file_id, const QString &savePath)
{
    QUrl apiUrl(BASE_URL + "files/download_file/" + file_id);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath](){
        onDownloadFileFinished(reply, savePath);
    });
}

void ApiController::onDownloadFileFinished(QNetworkReply *reply, const QString &savePath)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        if (!savePath.isEmpty())
        {
            QFile file(savePath);

            if (file.open(QIODevice::WriteOnly))
            {
                file.write(reply->readAll());
                file.close();
                emit downloadSucceed();
            }
            else
            {
                emit downloadFailed("Failed to open the file for writing.");
            }
        }
    }
    else
    {
        emit downloadFailed("Failed to fetch the file data. Please, check up your Internet connection.");
        handleNetworkError("Download File", reply);
    }

    reply->deleteLater();
}


// Copy User file in UFH Storage
void ApiController::copyFile(User &session, const QString &fileId, const QString &file_path)
{
    QUrl apiUrl(BASE_URL + "files/copy_file");

    QUrlQuery query;

    query.addQueryItem("file_id", fileId);
    query.addQueryItem("file_path", file_path);

    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        onCopyFileFinished(reply);
    });
}

void ApiController::onCopyFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Copy File", reply);
    reply->deleteLater();
}


// Delete User file in UFH Storage
void ApiController::deleteFile(User &session, const QString &fileId)
{
    qDebug() << fileId;
    QUrl apiUrl(BASE_URL + "files/delete_file/" + fileId);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onDeleteFileFinished(reply);
    });
}

void ApiController::onDeleteFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Delete File", reply);
    reply->deleteLater();
}


// Update User file in UFH Storage
void ApiController::updateFile(User &session, const QJsonObject &metaData)
{
    QUrl apiUrl(BASE_URL + "files/update_file/" + metaData["id"].toString());
    QUrlQuery query;

    query.addQueryItem("name", metaData["name"].toString());
    query.addQueryItem("path", metaData["path"].toString());
    query.addQueryItem("description", metaData["description"].toString());
    query.addQueryItem("author", metaData["author"].toString());
    query.addQueryItem("theme", metaData["theme"].toString());
    query.addQueryItem("is_public", QString::number(metaData["is_public"].toInt()));
    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUpdateFileFinished(reply);
    });
}

void ApiController::onUpdateFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Update File", reply);
    reply->deleteLater();
}
