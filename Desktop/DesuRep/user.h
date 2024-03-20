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
        int id;
        QString username;
        QString email;
        QString pass;
        int on_active;
        int is_banned;
        int is_actual;
        int role;
    };

    User(const Data &data);
    User();

    ~User();

public:
    int getId() const { return data.id; }
    QString getUsername() const { return data.username; }

    User::Data *getData() { return &data;  }

    QString getEmail() const { return data.email; }

    void setEmail(QString newEmail) { data.email = newEmail; }
    void setPassword(QString passHash) { data.pass = passHash; }

    void setData(User::Data set_data);

private:
    Data data;
};

#endif // USER_H
