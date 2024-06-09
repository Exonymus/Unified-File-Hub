#include "googledriveapi.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFileInfo>
#include <QDebug>
#include <QProgressBar>
#include <QHttpMultiPart>

GoogleDriveAPI::GoogleDriveAPI(QObject *parent) : QObject(parent)
{
    isLinked = false;
    networkManager = new QNetworkAccessManager(this);
}

GoogleDriveAPI::GoogleDriveAPI(const QString &accessToken, const QString &driveOwner, QObject *parent)
    : QObject(parent), accessToken(accessToken), driveOwner(driveOwner),  networkManager(new QNetworkAccessManager(this)) {}

void GoogleDriveAPI::setData(QString accessToken, QString driveOwner) {
    this->accessToken = accessToken;
    this->driveOwner = driveOwner;
    isLinked = true;
}

void GoogleDriveAPI::clearData() {
    accessToken.clear();
    driveOwner.clear();
    parentPaths.clear();
    pendingQueue.clear();
    pendingRequests.clear();
    GDFiles.clear();
    isLinked = false;
}

void GoogleDriveAPI::getUserFiles()
{
    QUrl url("https://www.googleapis.com/drive/v3/files");
    QUrlQuery query;
    query.addQueryItem("fields", "files(id, name, owners(emailAddress), mimeType, parents, size, createdTime)");
    query.addQueryItem("q", "mimeType != 'application/vnd.google-apps.folder'");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGetUserFilesFinished(reply);
    });
}

void GoogleDriveAPI::onGetUserFilesFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray response = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response);
        QJsonArray files = json.object().value("files").toArray();

        QStringList fileIds;

        QMap<QString, QString> mimeTypeToExtensions = {
            {"application/vnd.google-apps.document", "docx"},
            {"application/vnd.google-apps.spreadsheet", "xlsx"},
            {"application/vnd.google-apps.presentation", "pptx"},
            {"application/vnd.google-apps.drawing", "png"},
            {"application/vnd.google-apps.form", "html"},
            {"application/vnd.google-apps.script", "js"},
            {"application/vnd.google-apps.folder", ""},
        };

        for (const QJsonValue &fileValue : files)
        {
            QJsonObject fileObject = fileValue.toObject();
            QString id = fileObject.value("id").toString();
            QString name = fileObject.value("name").toString();
            QString owner = fileObject.value("owners").toArray().first().toObject().value("emailAddress").toString();
            QString mimeType = fileObject.value("mimeType").toString();
            QString size = fileObject.contains("size") ? fileObject.value("size").toString() : "Unknown";
            QJsonArray parentsArray = fileObject.value("parents").toArray();
            QString parentId = parentsArray.isEmpty() ? "" : parentsArray.first().toString();
            QString uploadDate = QDateTime::fromString(fileObject.value("createdTime").toString(), Qt::ISODate).toString("yyyy-MM-dd hh:mm:ss");
            if (id.isEmpty() || name.isEmpty() || mimeType.isEmpty()) {
                continue;
            }

            QString googleExt = "." + mimeTypeToExtensions.value(mimeType, "");
            QString valid_mimeType = googleExt == "."? File::getContentType(name) : File::getContentType(name + googleExt);
            if (valid_mimeType == "application/octet-stream") { valid_mimeType = mimeType; }

            QJsonObject fileMetadata =
            {
                {"id", id},
                {"name", File::cropExtension(name)},
                {"path", ""},
                {"parent", parentId},
                {"mime_type", valid_mimeType},
                {"is_public", driveOwner != owner},
                {"owner", owner},
                {"upload_date", uploadDate},
                {"size", size.toDouble() / (1024 * 1024)}
            };
            GDFiles.append(File(fileMetadata));

            if (!parentId.isEmpty()) {
                fileIds.append(id);
                pendingRequests.insert(id);
                pendingQueue.enqueue(qMakePair(id, parentId));
            } else {
                parentPaths[id] = "";
            }
        }

        if (!pendingQueue.isEmpty()) {
            QPair<QString, QString> nextRequest = pendingQueue.dequeue();
            fetchParentPath(nextRequest.first, nextRequest.second);
        } else {
            emit listFilesCompleted();
        }
    } else {
        qDebug() << "Error listing files:" << reply->errorString();
    }
    reply->deleteLater();
}

void GoogleDriveAPI::fetchParentPath(const QString &fileId, const QString &parentId)
{
    if (parentPaths.contains(parentId)) {
        pendingRequests.remove(fileId);
        if (!pendingQueue.isEmpty()) {
            QPair<QString, QString> nextRequest = pendingQueue.dequeue();
            fetchParentPath(nextRequest.first, nextRequest.second);
        }
        checkAndEmitListFilesCompleted();
        return;
    }

    QUrl url("https://www.googleapis.com/drive/v3/files/" + parentId);
    QUrlQuery query;
    query.addQueryItem("fields", "id,name,parents");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [fileId, parentId, this]()
    {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument json = QJsonDocument::fromJson(response);
            QJsonObject fileObject = json.object();

            QString folderName = fileObject.value("name").toString();
            parentPaths[parentId] = folderName;

        } else {
            qDebug() << "Error fetching parent path:" << reply->errorString();
        }

        reply->deleteLater();

        pendingRequests.remove(fileId);
        if (!pendingQueue.isEmpty()) {
            QPair<QString, QString> nextRequest = pendingQueue.dequeue();
            fetchParentPath(nextRequest.first, nextRequest.second);
        }
        checkAndEmitListFilesCompleted();
    });
}

QString GoogleDriveAPI::resolvePath(const QString &parentId)
{
    File targetParent = File::findObjectById(GDFiles, parentId);

    if (parentPaths.contains(parentId))
    {
        return parentPaths[parentId];
    }
    else if (!targetParent.isEmptyFile())
    {
        QString path = resolvePath(parentId);
        QString name = targetParent.getName();
        parentPaths[parentId] = path + "/" + name;
        return parentPaths[parentId];
    }
    else
    {
        return ".";
    }
}

void GoogleDriveAPI::checkAndEmitListFilesCompleted()
{
    if (pendingRequests.isEmpty() && pendingQueue.isEmpty()) {
        for (auto it = GDFiles.begin(); it != GDFiles.end(); ++it) {
            QString filePath = resolvePath(it->getGoogleParentId());
            it->setPath(filePath);
            if (it->getPath() == "Мой диск")
            {
                it->setPath(".");
            }
        }
        emit listFilesCompleted();
    }
}


void GoogleDriveAPI::handleNetworkError(const QString &operation, QNetworkReply *reply)
{
    qDebug()<<"|----- API REQUEST ERROR ----->";
    qDebug() << "↳ Requester: " << operation;
    qDebug() << "↳ Error code: " << reply->error();
    qDebug() << "↳ Server response: " << reply->readAll();
}

void GoogleDriveAPI::fetchStorageUsage(QProgressBar *progressBar) {
    QUrl url("https://www.googleapis.com/drive/v3/about?fields=storageQuota");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QNetworkReply *reply = networkManager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
            QJsonObject jsonObject = jsonResponse.object();
            QJsonObject storageQuota = jsonObject["storageQuota"].toObject();

            qint64 limit = storageQuota["limit"].toVariant().toLongLong();
            qint64 usage = storageQuota["usage"].toVariant().toLongLong();

            qreal percentage = (static_cast<double>(usage) / limit) * 100;
            progressBar->setValue(percentage);
        }
        else {
            handleNetworkError("GDrive Usage Fetch", reply);
        }
        reply->deleteLater();
    });
}


void GoogleDriveAPI::downloadFile(const QString &fileId, const QString &destinationPath, const qreal fileSize, QProgressBar *progressBar) {
    QUrl metadataUrl("https://www.googleapis.com/drive/v3/files/" + fileId);
    QNetworkRequest metadataRequest(metadataUrl);
    metadataRequest.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *metadataReply = networkManager->get(metadataRequest);

    connect(metadataReply, &QNetworkReply::finished, this, [=]() {
        if (metadataReply->error() == QNetworkReply::NoError) {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(metadataReply->readAll());
            QJsonObject fileObject = jsonDoc.object();
            QString mimeType = fileObject["mimeType"].toString();

            progressBar->setMaximum(static_cast<int>(fileSize) * 1024 * 1024);

            if (mimeType.startsWith("application/vnd.google-apps.")) {
                // Handle Google Docs Editors files using export
                QString exportMimeType;
                if (mimeType == "application/vnd.google-apps.document") {
                    exportMimeType = "aapplication/vnd.openxmlformats-officedocument.wordprocessingml.document"; // DOCX
                } else if (mimeType == "application/vnd.google-apps.spreadsheet") {
                    exportMimeType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"; // XLSX
                } else if (mimeType == "application/vnd.google-apps.presentation") {
                    exportMimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation"; // PPTX
                } else {
                    emit downloadFailed("Unsupported Google Docs file type for export.");
                    metadataReply->deleteLater();
                    return;
                }

                QUrl exportUrl("https://www.googleapis.com/drive/v3/files/" + fileId + "/export?mimeType=" + QUrl::toPercentEncoding(exportMimeType));
                QNetworkRequest exportRequest(exportUrl);
                exportRequest.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

                QNetworkReply *exportReply = networkManager->get(exportRequest);

                progressBar->setMaximum(static_cast<int>(fileSize) * 1024 * 1024);
                connect(exportReply, &QNetworkReply::downloadProgress, progressBar, [progressBar](qint64 bytesReceived) {
                    progressBar->setValue(static_cast<double>(bytesReceived));
                });

                connect(exportReply, &QNetworkReply::finished, this, [=]() {
                    if (exportReply->error() == QNetworkReply::NoError) {
                        QFile file(destinationPath);
                        if (file.open(QIODevice::WriteOnly)) {
                            file.write(exportReply->readAll());
                            file.close();
                            emit downloadSucceed();
                        } else {
                            emit downloadFailed("Failed to open the file for writing.");
                        }
                    } else {
                        emit downloadFailed("Failed to fetch the file data. Please, check your Internet connection.");
                        handleNetworkError("GDrive Export File", exportReply);
                    }
                    exportReply->deleteLater();
                });
            } else {
                // Handle binary files
                QUrl url("https://www.googleapis.com/drive/v3/files/" + fileId + "?alt=media");
                QNetworkRequest request(url);
                request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

                QNetworkReply *reply = networkManager->get(request);

                connect(reply, &QNetworkReply::downloadProgress, progressBar, [progressBar](qint64 bytesReceived) {
                    progressBar->setValue(static_cast<double>(bytesReceived));
                });

                connect(reply, &QNetworkReply::finished, this, [=]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        QFile file(destinationPath);
                        if (file.open(QIODevice::WriteOnly)) {
                            file.write(reply->readAll());
                            file.close();
                            emit downloadSucceed();
                        } else {
                            emit downloadFailed("Failed to open the file for writing.");
                        }
                    } else {
                        emit downloadFailed("Failed to fetch the file data. Please, check your Internet connection.");
                        handleNetworkError("GDrive Download File", reply);
                    }
                    reply->deleteLater();
                });
            }
        } else {
            emit downloadFailed("Failed to fetch file metadata.");
            handleNetworkError("GDrive File Metadata", metadataReply);
        }
        metadataReply->deleteLater();
    });
}




void GoogleDriveAPI::createFolder(const QString &folderName, const std::function<void(const QString &)> &callback) {
    QUrl url("https://www.googleapis.com/drive/v3/files");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject metadata;
    metadata["name"] = folderName;
    metadata["mimeType"] = "application/vnd.google-apps.folder";

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(metadata).toJson());

    QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
            QJsonObject jsonObject = jsonResponse.object();
            QString folderId = jsonObject["id"].toString();
            callback(folderId);
        } else {
            qWarning() << "Error creating folder:" << reply->errorString();
            callback(QString());
        }
        reply->deleteLater();
    });
}

void GoogleDriveAPI::uploadFile(const QString &filePath, const QString &parentFolderName) {
    auto upload = [=](const QString &parentId) {
        QUrl url("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart");
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/related");

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

        // Metadata part
        QHttpPart metadataPart;
        metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

        QJsonObject metadata;
        metadata["name"] = QFileInfo(filePath).fileName();
        if (!parentId.isEmpty()) {
            QJsonArray parents;
            parents.append(parentId);
            metadata["parents"] = parents;
        }
        metadataPart.setBody(QJsonDocument(metadata).toJson());

        // File content part
        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\""));

        QFile *file = new QFile(filePath);
        if (!file->open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open file for reading:" << filePath;
            delete file;
            delete multiPart;
            return;
        }
        filePart.setBodyDevice(file);
        file->setParent(multiPart);

        multiPart->append(metadataPart);
        multiPart->append(filePart);

        QNetworkReply *reply = networkManager->post(request, multiPart);
        multiPart->setParent(reply);

        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "File uploaded successfully";
            } else {
                qWarning() << "Error in network reply:" << reply->errorString();
            }
            reply->deleteLater();
        });
    };

    createFolder(parentFolderName, upload);
}


void GoogleDriveAPI::editFileMetadata(const QString &fileId, const QString &newName, const QString &newParentId) {
    QUrl url("https://www.googleapis.com/drive/v3/files/" + fileId);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject metadata;
    if (!newName.isEmpty()) {
        metadata["name"] = newName;
    }
    if (!newParentId.isEmpty()) {
        QJsonArray parents;
        parents.append(newParentId);
        metadata["parents"] = parents;
    }

    QNetworkReply *reply = networkManager->sendCustomRequest(request, "PATCH", QJsonDocument(metadata).toJson());

    QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "File metadata updated successfully";
        } else {
            qWarning() << "Error updating file metadata:" << reply->errorString();
        }
        reply->deleteLater();
    });
}
