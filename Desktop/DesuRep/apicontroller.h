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
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QProgressBar>
#include <QBuffer>
#include <QHttpMultiPart>

#include "user.h"
#include "file.h"


class ApiController : public QObject
{
    Q_OBJECT

public:
    ApiController(QObject *parent = nullptr);

    // Метод для проверки состояния апи
    void checkApiAvailability(std::function<void(bool)> callback);

    // Мето для регистрации пользователя
    void signUp(User &session, const QJsonObject &user_metadata);

    // Метод для авторизации пользователя
    void authenticate(User &session, const QString &username, const QString &password);

    // Метод для получения метаданных пользователя
    void getUserInfo(User &session);

    // Метод для копирования файла
    void copyFile(User &session, const QString &fileId, const QString &file_path);

    // Метод для удаления файла
    void deleteFile(User &session, const QString &fileId);

    // Метод для обновления файла
    void updateFile(User &session, const QJsonObject &metaData);

    // Метод для выгрузки файла
    void uploadFile(User &session, const File &uploadFile, QProgressBar *progressBar);

    // Метод для загрузки файла
    void downloadFile(User &session, const QString &file_id, const QString &savePath, QProgressBar *progressBar);

    // Метод для получения файлов пользователя
    void getUserFiles(User &session, QList<File> &files);

    // Метод для получения файлов пользователя
    void editUser(User &session, const QJsonObject &changedUserMetadata, const QString change_mode);

    // Метод для восстановления пароля пользователя
    void recoverUser(User &session, const QJsonObject &recoveryUserMetadata);

private slots:
    void onCopyFileFinished(QNetworkReply *reply);

    void onDeleteFileFinished(QNetworkReply *reply);

    void onUploadFileFinished(QNetworkReply *reply);

    void onDownloadFileFinished(QNetworkReply *reply, const QString &savePath);

    void onUpdateFileFinished(QNetworkReply *reply);

    void onSignUpFinished(QNetworkReply *reply, User &session, const QJsonObject &user_metadata);

    void onGetUserFilesFinished(QNetworkReply *reply, QList<File> &files);

    void onAuthenticateFinished(QNetworkReply *reply, User &session);

    void onGetUserInfoFinished(QNetworkReply *reply, User &session);

    void onRequestFinished(QNetworkReply *reply);

    void onEditUserFinished(QNetworkReply *reply, QString change_mode);

    void onRecoverUserFinished(QNetworkReply *reply);

private:
    void handleApiResponse(const QString &operation, QNetworkReply *reply);
    void handleNetworkError(const QString &operation, QNetworkReply *reply);

    void processFileData(const QJsonObject& dataObject, QList<File> &files);

    File createFileObject(const QJsonObject& fileObject);
    User::Data createUserDataObject(const QJsonObject &userObject);

    QString BASE_URL;

    QNetworkAccessManager *networkManager;

signals:
    void sessionExpired();

    void authSucceed();
    void authFailed(const QString message);

    void userEditSucceed(const QString message);
    void userEditFailed();

    void userRecoverSucceed();
    void userRecoverFailed(const QString message);

    void refreshDesuFiles();

    void uploadSucceed();
    void uploadFailed();

    void downloadSucceed();
    void downloadFailed(const QString message);

    void filesUpdated();
};




#endif // APICONTROLLER_H
