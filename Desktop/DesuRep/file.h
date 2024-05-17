#ifndef FILE_H
#define FILE_H

#include "QJsonObject"
#include <QString>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>


class File
{
public:
    File();
    File(const QJsonObject& metaData);
    ~File();

public:
    QUuid getId() const { return QUuid(metaData["id"].toString()); }
    QString getName() const { return metaData["name"].toString(); }
    QString getPath() const { return metaData["path"].toString(); }
    QString getType() const { return metaData["mime_type"].toString(); }
    QString getDescription() const { return metaData["description"].toString(); }
    QString getAuthor() const { return metaData["author"].toString(); }
    QString getTheme() const { return metaData["theme"].toString(); }
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

private:
    QJsonObject metaData;
    bool is_empty;
};

#endif // FILE_H
