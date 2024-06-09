#ifndef GOOGLEDRIVEAUTH_H
#define GOOGLEDRIVEAUTH_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QBuffer>

class GoogleDriveAuth : public QObject
{
    Q_OBJECT

public:
    explicit GoogleDriveAuth(QObject *parent = nullptr);
    void authenticate();

    QString accessToken;
    QString refreshToken;

    QString userEmail;
    QByteArray userAvatarData;
    bool isLinked;

    void clear();

signals:
    void accessTokenReceived();
    void userInfoReceived();
    void authorizationError(const QString &error);

public slots:
    void checkDriveAccess();
    void refreshAccessToken();

private slots:
    void onAuthorizationCodeReceived(const QString &code);
    void onAccessTokenReply();
    void fetchUserInfo();

private:
    QString clientId;
    QString clientSecret;
    QString redirectUri;

    QNetworkAccessManager *networkManager;

    void startLocalServer();
    void loadClientCredentials();
};

class LocalServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit LocalServer(QObject *parent = nullptr);

signals:
    void authorizationCodeReceived(const QString &code);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void readClient();
};

#endif // GOOGLEDRIVEAUTH_H
