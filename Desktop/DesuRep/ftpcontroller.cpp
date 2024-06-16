#include "ftpcontroller.h"
#include <QDebug>
#include <QMimeDatabase>
#include <QDateTime>
#include <QFileInfo>
#include <QUuid>
#include <QJsonObject>

FTPController::FTPController(QObject *parent)
    : QObject(parent)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

FTPController::~FTPController()
{
    curl_global_cleanup();
}

QString FTPController::getServerFilePath(const File &file)
{
    return QString("/%1/%2.%3").arg(file.getPath(),
                                    file.getName(),
                                    File::getFileExtensionFromMimeType(file.getType()));
}

void FTPController::testConnection(const FTPConnection &connection)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        QString url = "ftp://" + connection.host + "/";
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD,
                         (connection.username + ":" + connection.password).toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            emit connectionTested(true);
        } else {
            qDebug() << "[FTP Server]: Test Connection failed:" << curl_easy_strerror(res);
            emit connectionTested(false);
        }

        curl_easy_cleanup(curl);
    }
}

void FTPController::addConnection(const FTPConnection &connection)
{
    connections.append(connection);
}

void FTPController::removeConnection(const QString &host)
{
    for (int i = 0; i < connections.size(); ++i) {
        if (connections.at(i).host == host) {
            connections.removeAt(i);
            break;
        }
    }
}

QList<FTPConnection> FTPController::getConnections() const
{
    return connections;
}

void FTPController::setCurrentConnection(const FTPConnection connection)
{
    currentConnection = connection;
}

void FTPController::setConnections(QList<FTPConnection> connList)
{
    connections = connList;
    currentConnection.id = "";
}

void FTPController::clearConnections()
{
    connections.clear();
    currentConnection.id = "";
}

void FTPController::getUserFiles()
{
    currentPath = "/";
    directoriesToProcess.clear();
    ftpFiles.clear();
    processDirectory(currentPath);
}

void FTPController::processDirectory(const QString &path)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        QString url = "ftp://" + currentConnection.host + path;

        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD,
                         (currentConnection.username + ":" + currentConnection.password).toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FTPController::writeCallback);

        QByteArray response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            QList<File> files = parseFileList(response, path);

            for (const File &file : files) {
                if (file.getType() == "inode/directory") {
                    directoriesToProcess.append(file.getPath());
                } else {
                    ftpFiles.append(file);
                }
            }

            if (!directoriesToProcess.isEmpty()) {
                QString nextDir = directoriesToProcess.takeFirst();
                processDirectory(nextDir);
            } else {
                emit filesListedSuccess();
            }
        } else {
            emit filesListedFailed();
            qDebug() << "Error listing files:" << curl_easy_strerror(res);
        }

        curl_easy_cleanup(curl);
    }
}

QList<File> FTPController::parseFileList(const QByteArray &data, const QString &path)
{
    QList<File> files;
    QMimeDatabase mimeDatabase;
    QList<QByteArray> lines = data.split('\n');

    for (const QByteArray &line : lines) {
        if (line.isEmpty()) continue;
        QList<QByteArray> parts = line.split(' ');
        if (parts.size() < 9) continue;

        QString fileName = parts.last();
        QString filePath = path.isEmpty() ? "." : path;

        int iterator = parts.size() - 2;
        QString year = parts.at(iterator);
        if (year.contains(":")) {
            year = QString::number(QDateTime::currentDateTime().date().year());
        } else {
            iterator -= 1;
        }

        QString month = parts.at(iterator - 2);
        QString day = parts.at(iterator - 1);
        QString time = parts.at(iterator);

        QString fileSize = parts.at(iterator - 3);
        QString dateStr = QString("%1 %2 %3 %4").arg(year,
                                                     month.isEmpty()? "Jan": month,
                                                     day.isEmpty()? "01" : day,
                                                     time.isEmpty()? "00:00" : time);

        QDateTime dateTime = QDateTime::fromString(dateStr, "yyyy MMM dd hh:mm");
        QString uploadDate = "";
        if (dateTime.isValid()) {
            dateTime.setDate(QDate(dateTime.date().year(), dateTime.date().month(), dateTime.date().day()));
            uploadDate = dateTime.toString("yyyy-MM-dd hh:mm:ss");
        }

        QFileInfo localFileInfo(fileName);
        QString mimeType = mimeDatabase.mimeTypeForFile(localFileInfo).name();
        if (mimeType == "application/octet-stream" && fileSize == "0") {
            mimeType = "inode/directory";
            filePath += fileName + "/";
        } else {
            filePath.removeFirst().removeLast();
            if (filePath.isEmpty()) { filePath = "."; }
        }

        QJsonObject fileMetadata = {
            {"id", QUuid::createUuid().toString()},
            {"name", File::cropExtension(fileName)},
            {"path", filePath},
            {"mime_type", mimeType},
            {"is_public", false},
            {"upload_date", uploadDate},
            {"size", fileSize.toDouble() / (1024 * 1024)}
        };

        files.append(File(fileMetadata));
    }
    return files;
}


int FTPController::progressCallback(void *progressBar, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    QProgressBar *bar = static_cast<QProgressBar*>(progressBar);
    if (bar) {
        if (dltotal > 0) {
            bar->setMaximum(static_cast<int>(dltotal));
            bar->setValue(static_cast<int>(dlnow));
        } else if (ultotal > 0) {
            bar->setMaximum(static_cast<int>(ultotal));
            bar->setValue(static_cast<int>(ulnow));
        }
    }
    return 0;
}

size_t FTPController::readCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    QFile *file = static_cast<QFile*>(userdata);
    return file->read(static_cast<char*>(ptr), size * nmemb);
}

size_t FTPController::writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    QByteArray *response = static_cast<QByteArray*>(userdata);
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(ptr), totalSize);
    return totalSize;
}

size_t FTPController::writeFileCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    QFile *file = static_cast<QFile*>(userdata);
    if (!file || !file->isOpen()) {
        return 0;
    }
    return file->write(static_cast<char*>(ptr), size * nmemb);
}


void FTPController::downloadFile(const QString &fileId, const QString &localFilePath, QProgressBar *progressBar)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        File toDownload = File::findObjectById(ftpFiles, fileId, "ftp");

        QFile file(localFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadFailed("Failed to open local file for writing");
            curl_easy_cleanup(curl);
            return;
        }

        QString url = "ftp://" + currentConnection.host + getServerFilePath(toDownload);;

        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (currentConnection.username + ":" + currentConnection.password).toUtf8().constData());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FTPController::writeFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

        if (progressBar) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, FTPController::progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progressBar);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            emit downloadSucceed();
        } else {
            emit downloadFailed(curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        file.close();
    } else {
        emit downloadFailed("[FTP Server]: Failed to initialize CURL");
    }
}

void FTPController::uploadFile(const File &uploadFile, QProgressBar *progressBar)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        QJsonObject metaData = uploadFile.getMetaData();
        QFile file(metaData["BLOB_path"].toString());

        if (!file.open(QIODevice::ReadOnly)) {
            emit uploadFailed();
            return;
        }

        QString remoteFilePath = getServerFilePath(uploadFile);
        QString url = "ftp://" + currentConnection.host + remoteFilePath;

        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (currentConnection.username + ":" + currentConnection.password).toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, FTPController::readCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &file);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(file.size()));
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY);

        if (progressBar) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, FTPController::progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progressBar);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        // Enable passive mode
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            emit uploadSucceed();
        } else {
            emit uploadFailed();
            qDebug() << "[FTP Server]: Upload failed:" << curl_easy_strerror(res);
        }

        curl_easy_cleanup(curl);
        file.close();
    }
}

void FTPController::deleteFile(const QString &fileId)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        File toDelete = File::findObjectById(ftpFiles, fileId, "ftp");

        QString url = "ftp://" + currentConnection.host + "/";

        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (currentConnection.username + ":" + currentConnection.password).toUtf8().constData());

        QString deleCommand = "DELE " + getServerFilePath(toDelete);
        curl_slist *commands = nullptr;
        commands = curl_slist_append(commands, deleCommand.toUtf8().constData());

        curl_easy_setopt(curl, CURLOPT_QUOTE, commands);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            emit filesNeedUpdate();
        } else {
            qDebug() << "[FTP Server]: Delete error "<< curl_easy_strerror(res);
        }

        curl_slist_free_all(commands);
        curl_easy_cleanup(curl);
    } else {
        qDebug() << "[FTP Server]: Failed to initialize CURL";
    }
}

void FTPController::moveFile(const QString &fileId, const QString &destinationPath)
{
    CURL *curl = curl_easy_init();
    if (curl) {
        File toMove = File::findObjectById(ftpFiles, fileId, "ftp");

        QString sourcePath = getServerFilePath(toMove);
        QString destinationFilePath = QString("/%1/%2.%3")
                                        .arg(destinationPath,
                                             toMove.getName(),
                                             File::getFileExtensionFromMimeType(toMove.getType()));

        QString rnfrCommand = "RNFR " + sourcePath;
        QString rntoCommand = "RNTO " + destinationFilePath;
        qDebug() << rnfrCommand;
        qDebug() << rntoCommand;

        curl_slist *commands = nullptr;
        commands = curl_slist_append(commands, rnfrCommand.toUtf8().constData());
        commands = curl_slist_append(commands, rntoCommand.toUtf8().constData());

        QString url = "ftp://" + currentConnection.host;
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (currentConnection.username + ":" + currentConnection.password).toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_QUOTE, commands);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            emit filesNeedUpdate();
        } else {
            qDebug() << "[FTP Server]: Move error:" << curl_easy_strerror(res);
        }

        curl_slist_free_all(commands);
        curl_easy_cleanup(curl);
    } else {
        qDebug() << "[FTP Server]: Failed to initialize libcurl.";
    }
}



