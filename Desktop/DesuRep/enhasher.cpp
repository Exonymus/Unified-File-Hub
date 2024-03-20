#include "enhasher.h"
#include <QDebug>
#include "bcrypt.h"

QString Enhasher::hashPassword(const QString &password)
{
    return QString::fromStdString((bcrypt::generateHash(password.toStdString())));
}

bool Enhasher::checkPassword(const QString &password, const QString &hashedPassword)
{
    return bcrypt::validatePassword(password.toStdString(), hashedPassword.toStdString());
}
