#ifndef USER_H
#define USER_H

#include "file.h"
#include <QString>
#include <QList>

class User
{
public:
    struct Data
    {
        QUuid id;
        QString username;
        QString email;
        int secret_num;
        QString secret_answer;
        QString password;
        int is_banned;
    };

    struct Token
    {
        QString value;
        bool expired = false;
    };


    User(const Data &userData);
    User();

    ~User();

public:
    QUuid getId() const { return data.id; }
    QString getUsername() const { return data.username; }
    QString getEmail() const { return data.email; }
    int getSecretNum() const { return data.secret_num; }
    QString getSecretAnswer() const { return data.secret_answer; }

    void setEmail(QString newEmail) { data.email = newEmail; }
    void setPassword(QString passHash) { data.password = passHash; }


    QString getToken() const { return token.value; }
    bool tokenExpired() const { return token.expired; }

    void setTokenValue(QString newToken) { token.value = newToken; }
    void setTokenExpired() { token.expired = true; }


    User::Data *getData() { return &data;  }
    void setData(User::Data set_data);

private:
    Data data;
    Token token;
};

#endif // USER_H
