#ifndef FILE_H
#define FILE_H

#include "QJsonObject"
#include <QString>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUuid>
#include <QFileInfo>

class File
{
public:
    File();
    File(const QJsonObject& metaData);
    ~File();

public:
    QUuid getId() const { return QUuid(metaData["id"].toString()); }
    QString getGoogleId() const { return metaData["id"].toString(); }
    QString getGoogleParentId() const { return metaData["parent"].toString(); }
    QString getName() const { return metaData["name"].toString(); }
    QString getPath() const { return metaData["path"].toString(); }
    QString getType() const { return metaData["mime_type"].toString(); }
    QString getDescription() const { return metaData["description"].toString(); }
    QString getCategory() const { return metaData["category"].toString(); }
    QString getTag() const { return metaData["tag"].toString(); }
    QString getOwner() const { return metaData["owner"].toString(); }
    QUuid getOwnerId() const { return QUuid(metaData["owner_id"].toString()); }

    bool isPublic() const { return metaData["is_public"].toBool(); }
    bool isEmptyFile() const { return is_empty; }

    void setPath(QString path) { metaData["path"] = path; }
    void setOwner(QUuid owner_id) { metaData["owner_id"] = owner_id.toString(); }

    QJsonObject getMetaData() const { return metaData; }

    qreal getSizeInMB() const;
    qreal getBlobSizeInMB() const;

    static QString getFileExtensionFromMimeType(const QString& mimeTypeName);
    static QString getContentType(const QString& filePath);
    static QString cropExtension(const QString &fileName);
    static File findObjectById(const QList<File> &list, const QString &id);

private:
    QJsonObject metaData;
    bool is_empty;
};

#endif // FILE_H
