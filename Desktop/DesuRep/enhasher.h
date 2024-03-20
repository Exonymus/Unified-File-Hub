#ifndef ENHASHER_H
#define ENHASHER_H

#include <QString>
#include <QCryptographicHash>

class Enhasher
{
public:
    static QString hashPassword(const QString& password);
    static bool checkPassword(const QString &password, const QString &hashedPassword);
};

#endif // ENHASHER_H
