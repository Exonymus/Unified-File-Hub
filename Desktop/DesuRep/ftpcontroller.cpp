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
            qDebug() << "Connection failed:" << curl_easy_strerror(res);
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
                //qDebug() << "Files found: " << ftpFiles.size();
                emit filesListedSuccess();
            }
        } else {
            emit filesListedFailed();
            qDebug() << "Error listing files:" << curl_easy_strerror(res);
        }

        curl_easy_cleanup(curl);
    }
}

size_t FTPController::writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    QByteArray *response = static_cast<QByteArray*>(userdata);
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(ptr), totalSize);
    return totalSize;
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
