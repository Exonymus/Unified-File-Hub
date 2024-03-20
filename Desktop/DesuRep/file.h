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
    File(const QJsonObject& metaData, QByteArray blob = 0);
    ~File();

public:
    int getId() const { return metaData["id"].toInt(); }

    QString getName() const { return metaData["Name"].toString(); }
    QString getType() const { return metaData["Type"].toString(); }

    QString getUploader() const { return metaData["Uploader"].toString();; }
    void setUploader(QString uploader) { metaData["Uploader"] = uploader; }

    QString getPath() const { return metaData["Path"].toString(); }
    void setPath(QString path) { metaData["Path"] = path; }

    QByteArray getBlob() const { return rawFileData; }

    QJsonObject getMetaData() const { return metaData; }


    bool isPublic() const { return metaData["Public"].toBool(); }
    bool isEmptyFile() const { return is_empty; }

    qreal getSizeInMB() const;
    qreal getBlobSizeInMB() const;

    static QString getFileExtensionFromMimeType(const QString& mimeTypeName);
    static QString getContentType(const QString& filePath);

private:
    QJsonObject metaData;
    QByteArray rawFileData;
    bool is_empty;
};

#endif // FILE_H
