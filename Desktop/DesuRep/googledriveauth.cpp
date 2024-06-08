#include "googledriveauth.h"
#include <QFile>
#include <QUrl>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>

GoogleDriveAuth::GoogleDriveAuth(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
    loadClientCredentials();
    redirectUri = "http://localhost:8080";
}

void GoogleDriveAuth::loadClientCredentials()
{
    QFile file(":/credentials/google");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObj = jsonDoc.object().value("installed").toObject();
        clientId = jsonObj.value("client_id").toString();
        clientSecret = jsonObj.value("client_secret").toString();
    }
    else
    {
        qDebug() << "Failed to open credentials file.";
    }
}

void GoogleDriveAuth::authenticate()
{
    QString authUrl = QString("https://accounts.google.com/o/oauth2/auth?"
                              "response_type=code"
                              "&client_id=%1"
                              "&redirect_uri=%2"
                              "&scope=%3"
                              "&access_type=offline"
                              "&prompt=consent"
                              "&include_granted_scopes=true")
                      .arg(clientId, redirectUri,
                           "https://www.googleapis.com/auth/drive "
                           "https://www.googleapis.com/auth/userinfo.email "
                           "https://www.googleapis.com/auth/userinfo.profile");

    QDesktopServices::openUrl(QUrl(authUrl));
    startLocalServer();
}

void GoogleDriveAuth::clear()
{
    accessToken = "";
    isLinked = false;
    userEmail = "";
    userAvatarData = 0;
}

void GoogleDriveAuth::startLocalServer()
{
    LocalServer *server = new LocalServer(this);
    connect(server, &LocalServer::authorizationCodeReceived, this, &GoogleDriveAuth::onAuthorizationCodeReceived);
    server->listen(QHostAddress::Any, 8080);
}

void GoogleDriveAuth::onAuthorizationCodeReceived(const QString &code)
{
    QUrl tokenUrl("https://oauth2.googleapis.com/token");
    QUrlQuery query;
    query.addQueryItem("code", code);
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    query.addQueryItem("redirect_uri", redirectUri);
    query.addQueryItem("grant_type", "authorization_code");

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = networkManager->post(request, query.query().toUtf8());
    connect(reply, &QNetworkReply::finished, this, &GoogleDriveAuth::onAccessTokenReply);
}

void GoogleDriveAuth::onAccessTokenReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument json = QJsonDocument::fromJson(response);
        accessToken = json.object().value("access_token").toString();
        refreshToken = json.object().value("refresh_token").toString();

        // Проверка разрешения на доступ к диску.
        checkDriveAccess();
    }
    else
    {
        qDebug() << "Error:" << reply->errorString();
        emit authorizationError("Failed to get access token.");
    }
    reply->deleteLater();
}

void GoogleDriveAuth::fetchUserInfo()
{
    QUrl userInfoUrl("https://www.googleapis.com/oauth2/v3/userinfo");
    QNetworkRequest request(userInfoUrl);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray response = reply->readAll();
            QJsonDocument json = QJsonDocument::fromJson(response);
            QJsonObject userInfo = json.object();
            QString email = userInfo.value("email").toString();
            QString avatarUrl = userInfo.value("picture").toString();

            // Получение аватарки пользователя
            QNetworkReply *avatarReply = networkManager->get(QNetworkRequest(QUrl(avatarUrl)));
            connect(avatarReply, &QNetworkReply::finished, this, [this, avatarReply, email]()
            {
                if (avatarReply->error() == QNetworkReply::NoError)
                {
                    userEmail = email;
                    userAvatarData = avatarReply->readAll();
                    emit userInfoReceived();

                }
                else
                {
                    qDebug() << "Error fetching avatar image:" << avatarReply->errorString();
                    emit authorizationError("Failed to fetch avatar image.");
                }
                avatarReply->deleteLater();
            });

        } else {
            qDebug() << "Error fetching user info:" << reply->errorString();
            emit authorizationError("Failed to fetch user info.");
        }
        reply->deleteLater();
    });
}

void GoogleDriveAuth::checkDriveAccess()
{
    QUrl driveUrl("https://www.googleapis.com/drive/v3/files?pageSize=1");
    QNetworkRequest request(driveUrl);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            emit accessTokenReceived();

            // Получение данных пользователя.
            fetchUserInfo();
        }
        else
        {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
            {
                qDebug() << "Access token expired, attempting to refresh.";
                refreshAccessToken();
            }
            else
            {
                qDebug() << "Error checking drive access:" << reply->errorString();
                emit authorizationError("Drive access not granted.");
            }

        }
        reply->deleteLater();
    });
}

void GoogleDriveAuth::refreshAccessToken()
{
    QUrl tokenUrl("https://oauth2.googleapis.com/token");
    QUrlQuery params;
    params.addQueryItem("client_id", clientId);
    params.addQueryItem("client_secret", clientSecret);
    params.addQueryItem("refresh_token", refreshToken);
    params.addQueryItem("grant_type", "refresh_token");

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = networkManager->post(request, params.query().toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll());
            QJsonObject jsonObject = jsonResponse.object();
            accessToken = jsonObject["access_token"].toString();

            // Retry drive access check after refreshing the token
            checkDriveAccess();
        }
        else
        {
            qDebug() << "Error refreshing access token:" << reply->errorString();
            emit authorizationError("Failed to refresh access token.");
        }
        reply->deleteLater();
    });
}

LocalServer::LocalServer(QObject *parent)
    : QTcpServer(parent) {}

void LocalServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *clientConnection = new QTcpSocket(this);
    connect(clientConnection, &QTcpSocket::readyRead, this, &LocalServer::readClient);
    clientConnection->setSocketDescriptor(socketDescriptor);
}

void LocalServer::readClient()
{
    QTcpSocket *clientConnection = qobject_cast<QTcpSocket*>(sender());
    if (clientConnection)
    {
        QByteArray request = clientConnection->readAll();
        const static QRegularExpression codeRx("code=([^&]+)");
        QRegularExpressionMatch match = codeRx.match(request);
        if (match.hasMatch())
        {
            QString code = match.captured(1);
            emit authorizationCodeReceived(code);
            clientConnection->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                                    "<html><body><h1>Authorization successful. You can close this window.</h1></body></html>");
        }
        QTimer::singleShot(1000, clientConnection, &QTcpSocket::disconnectFromHost);
    }
}

