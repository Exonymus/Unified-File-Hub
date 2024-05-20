#include "apicontroller.h"

ApiController::ApiController(QObject *parent) : QObject(parent)
{
    BASE_URL = "http://127.0.0.1:8888/";

    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiController::onRequestFinished);
}

void ApiController::checkApiAvailability(std::function<void(bool)> callback)
{
    QUrl apiUrl(BASE_URL);
    QNetworkRequest request(apiUrl);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [ reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            // API доступно
            callback(true);
        } else {
            // API недоступно
            callback(false);
        }
        reply->deleteLater();
    });
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
    if (reply->error() == QNetworkReply::AuthenticationRequiredError)
    {
        emit sessionExpired();
    }

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


// Creating new User account
void ApiController::signUp(User &session, const QJsonObject &user_metadata)
{
    QJsonDocument jsonDoc(user_metadata);
    QByteArray metaDataBytes = jsonDoc.toJson(QJsonDocument::Compact);

    QUrl apiUrl(BASE_URL + "users/signup");

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, metaDataBytes);

    connect(reply, &QNetworkReply::finished, this, [ reply, &session, this, user_metadata](){
        onSignUpFinished(reply, session, user_metadata);
    });
}

void ApiController::onSignUpFinished(QNetworkReply *reply, User &session, const QJsonObject &user_metadata)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        authenticate(session, user_metadata.value("username").toString(), user_metadata.value("password").toString());
    }
    else
    {
        handleNetworkError("Sign Up", reply);
    }

    reply->deleteLater();
}


// Authenticating User and receiving token
void ApiController::authenticate(User &session, const QString &username, const QString &password)
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
                    getUserInfo(session);
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


// Get User Info
void ApiController::getUserInfo(User &session)
{
    QUrl apiUrl(BASE_URL + "users/get_info");

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());


    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, &session](){
        onGetUserInfoFinished(reply, session);
    });

    return;
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
        QDateTime::fromString(userObject["reg_date"].toString(), Qt::ISODate)
    };

    return data;
}


// Editing User metadata in UFH Storage
void ApiController::editUser(User &session, const QJsonObject &changedUserMetadata, const QString change_mode)
{
    QUrl apiUrl(BASE_URL + "users/edit_" + change_mode);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QJsonDocument jsonDoc(changedUserMetadata);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, change_mode]() {
        onEditUserFinished(reply, change_mode);
    });
}

void ApiController::onEditUserFinished(QNetworkReply *reply, QString change_mode)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        emit userEditSucceed(change_mode);
    }
    else
    {
        emit userEditFailed();
        handleNetworkError("Edit User", reply);
    }

    reply->deleteLater();
}


// Recivering User password in UFH Storage
void ApiController::recoverUser(User &session, const QJsonObject &recoveryUserMetadata)
{
    QUrl apiUrl(BASE_URL + "users/recover");

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QJsonDocument jsonDoc(recoveryUserMetadata);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onRecoverUserFinished(reply);
    });
}

void ApiController::onRecoverUserFinished(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid())
    {
        if (statusCode.toInt() == 200)
        {
            emit userRecoverSucceed();
        }
        else if (statusCode.toInt() == 404)
        {
            emit userRecoverFailed("Account does not exist.");
        }
        else if (statusCode.toInt() == 406)
        {
            emit userRecoverFailed("User credentials invalid!");
        }
        else
        {
            emit userRecoverFailed("Unknown error occured!");
            handleNetworkError("Recover User", reply);
        }
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
        {"category", fileObject["category"].toString()},
        {"tag", fileObject["tag"].toString()},
        {"is_public", fileObject["is_public"].toBool()},
        {"owner", fileObject["owner"].toString()},
        {"owner_id", fileObject["owner_id"].toString()},
        {"upload_date", fileObject["upload_date"].toString().replace("T"," ")},
        {"size", fileObject["size"].toDouble() / (1024 * 1024)}
    };

    return File(data);
}


// Uploading User file to UFH Storage
void ApiController::uploadFile(User &session, const File &uploadFile, QProgressBar *progressBar)
{
    QJsonObject metaData = uploadFile.getMetaData();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Adding file part
    QHttpPart filePart;
    QString fullFileName = metaData["name"].toString() + '.' + File::getFileExtensionFromMimeType(metaData["mime_type"].toString());
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"input_data\"; filename=\"" + fullFileName + "\""));

    QByteArray fileData = QByteArray::fromBase64(metaData["BLOB"].toString().toUtf8());
    QBuffer *fileBuffer = new QBuffer();
    fileBuffer->setData(fileData);
    fileBuffer->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(fileBuffer);
    fileBuffer->setParent(multiPart);
    multiPart->append(filePart);

    // Adding metadata part
    QHttpPart metaDataPart;
    metaDataPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"metadata\""));
    QJsonDocument jsonDoc(metaData);
    QByteArray metaDataBytes = jsonDoc.toJson(QJsonDocument::Compact);
    metaDataPart.setBody(metaDataBytes);
    multiPart->append(metaDataPart);

    QUrl apiUrl(BASE_URL + "files/upload_file");

    QNetworkRequest request(apiUrl);
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, [=](qint64 bytesSent, qint64 bytesTotal)
    {
        progressBar->setMaximum(static_cast<int>(bytesTotal));
        progressBar->setValue(static_cast<int>(bytesSent));
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUploadFileFinished(reply);
    });
}

void ApiController::onUploadFileFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        emit uploadSucceed();
    } else
    {
        emit uploadFailed();
        handleNetworkError("Upload File", reply);
    }

    reply->deleteLater();
}

// Downloading User file from UFH Storage
void ApiController::downloadFile(User &session, const QString &file_id, const QString &savePath, QProgressBar *progressBar)
{
    QUrl apiUrl(BASE_URL + "files/download_file/" + file_id);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal)
    {
        progressBar->setMaximum(static_cast<int>(bytesTotal));
        progressBar->setValue(static_cast<int>(bytesReceived));
    });

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

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("Authorization"),
                         QString("bearer %1").arg(session.getToken()).toUtf8());

    QJsonDocument jsonDoc(metaData);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUpdateFileFinished(reply);
    });
}

void ApiController::onUpdateFileFinished(QNetworkReply *reply)
{
    handleApiResponse("Update File", reply);
    reply->deleteLater();
}
