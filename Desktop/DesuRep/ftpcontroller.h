#ifndef FTPCONTROLLER_H
#define FTPCONTROLLER_H

#include "file.h"
#include <QObject>
#include <curl/curl.h>
#include <QUrl>
#include <QList>

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

    static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata);
    QList<File> parseFileList(const QByteArray &data, const QString &path);
    void processDirectory(const QString &path);
};

#endif
