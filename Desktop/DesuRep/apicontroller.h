#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QUuid>
#include <QFile>
#include <QBuffer>
#include <QHttpMultiPart>

#include "user.h"
#include "file.h"


class ApiController : public QObject
{
    Q_OBJECT

public:
    ApiController(QObject *parent = nullptr);

    // Метод для авторизации пользователя
    void authenticate(const QString &username, const QString &password, User &session);

    // Метод для копирования файла
    void copyFile(const QString &username, const QString &fileId, const QString &folderPath);

    // Метод для удаления файла
    void deleteFile(const QString &fileId);

    // Метод для обновления файла
    void updateFile(const QJsonObject &metaData);

    // Метод для загрузки файла
    void uploadFile(const File &uploadFile);

    // Метод для получения файлов пользователя
    void getUserFiles(const QString &username, QList<File> &files);

    // Метод для получения файлов пользователя
    void editUser(const QString &id, const QString &email);

private slots:
    void onCopyFileFinished(QNetworkReply *reply);

    void onDeleteFileFinished(QNetworkReply *reply);

    void onUpdateFileFinished(QNetworkReply *reply);

    void onGetUserFilesFinished(QNetworkReply *reply, QList<File> &files);

    void onAuthenticateFinished(QNetworkReply *reply, const QString &username, User &session);

    void onGetUserInfoFinished(QNetworkReply *reply, User &session);

    void onRequestFinished(QNetworkReply *reply);

    void onEditUserFinished(QNetworkReply *reply);

private:
    void handleApiResponse(const QString &operation, QNetworkReply *reply);
    void handleNetworkError(const QString &operation, QNetworkReply *reply);

    void processFileData(const QJsonObject& dataObject, QList<File> &files);
    void processUserData(const QJsonObject &dataObject, User &session);

    File createFileObject(const QJsonObject& fileObject);
    User::Data createUserDataObject(const QJsonObject &userObject);

    QString BASE_URL;

    QNetworkAccessManager *networkManager;

signals:
    void authSucceed();
    void authFailed(const QString message);

    void userEditSucceed();
    void userEditFailed();

    void refreshDesuFiles();

    void uploadSucceed();
    void uploadFailed();

    void filesUpdated();
};




#endif // APICONTROLLER_H
