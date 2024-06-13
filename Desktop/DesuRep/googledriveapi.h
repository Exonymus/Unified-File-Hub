#ifndef GOOGLEDRIVEAPI_H
#define GOOGLEDRIVEAPI_H

#include "file.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QMap>
#include <QStringList>
#include <QSet>
#include "QProgressBar"

class GoogleDriveAPI : public QObject
{
    Q_OBJECT
public:
    GoogleDriveAPI(QObject *parent = nullptr);
    explicit GoogleDriveAPI(const QString &accessToken, const QString &driveOwner, QObject *parent = nullptr);

    void getUserFiles();

    void fetchStorageUsage(QProgressBar *progressBar);
    void downloadFile(const QString &fileId, const QString &destinationPath, const qreal fileSize, QProgressBar *progressBar);
    void createFolder(const QString &folderName, const std::function<void(const QString &)> &callback);
    void uploadFile(const File &uploadFile, QProgressBar *progressBar);
    void deleteFile(const QString &fileId);
    void editFileMetadata(const QString &fileId, const QString &newName, const QString &newParentId);


public:
    QList<File> GDFiles;
    bool isLinked;
    void setData(QString accessToken, QString driveOwner );
    void clearData();

signals:
    void listFilesCompleted();

    void uploadSucceed();
    void uploadFailed();
    void downloadSucceed();
    void downloadFailed(const QString message);

    void filesNeedUpdate();

private slots:
    void onGetUserFilesFinished(QNetworkReply *reply);
    void fetchParentPath(const QString &fileId, const QString &parentId);

private:
    QString resolvePath(const QString &parentId);
    void checkAndEmitListFilesCompleted();
    void handleNetworkError(const QString &operation, QNetworkReply *reply);

    QString accessToken;
    QString driveOwner;
    QNetworkAccessManager *networkManager;
    QMap<QString, QString> parentPaths;
    QSet<QString> pendingRequests;
    QQueue<QPair<QString, QString>> pendingQueue;
};

#endif // GOOGLEDRIVEAPI_H
