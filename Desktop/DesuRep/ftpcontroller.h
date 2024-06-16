#ifndef FTPCONTROLLER_H
#define FTPCONTROLLER_H

#include "file.h"
#include <QObject>
#include <curl/curl.h>
#include <QUrl>
#include <QList>
#include <QProgressBar>

struct FTPConnection {
    QString id;
    QString name;
    QString host;
    QString username;
    QString password;
};

class FTPController : public QObject
{
    Q_OBJECT
public:
    explicit FTPController(QObject *parent = nullptr);
    ~FTPController();

    void testConnection(const FTPConnection &connection);

    void addConnection(const FTPConnection &connection);
    void removeConnection(const QString &host);
    void clearConnections();

    QList<FTPConnection> getConnections() const;
    void setConnections(QList<FTPConnection> connList);
    void setCurrentConnection(const FTPConnection connection);
    bool isConnectionSelected() const { return !currentConnection.id.isEmpty(); }

    void getUserFiles();

    void downloadFile(const QString &fileId, const QString &localFilePath, QProgressBar *progressBar);
    void uploadFile(const File &uploadFile, QProgressBar *progressBar);
    void deleteFile(const QString &fileId);
    void moveFile(const QString &fileId, const QString &destinationPath);

    QList<File> ftpFiles;
    FTPConnection currentConnection;

signals:
    void connectionTested(bool success);

    void filesListedSuccess();
    void filesListedFailed();

    void uploadSucceed();
    void uploadFailed();

    void downloadSucceed();
    void downloadFailed(const QString &message);

    void filesNeedUpdate();

private:
    QList<FTPConnection> connections;

    QString currentPath;
    QStringList directoriesToProcess;

    static QString getServerFilePath(const File &file);

    static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata);
    static int progressCallback(void *progressBar, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static size_t readCallback(void *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t writeFileCallback(void *ptr, size_t size, size_t nmemb, void *userdata);

    QList<File> parseFileList(const QByteArray &data, const QString &path);
    void processDirectory(const QString &path);
};

#endif
