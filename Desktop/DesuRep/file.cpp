#include "file.h"

File::File(const QJsonObject& data)
{
    metaData = data;
    is_empty = false;
}

File::File() {
    is_empty = true;
}

File::~File() { }

qreal File::getSizeInMB() const
{
    return metaData["size"].toDouble();
}


QString File::getFileExtensionFromMimeType(const QString& mimeTypeName)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeTypeName);

    // Get the preferred file extension for the MIME type
    QString fileExtension = mimeType.preferredSuffix();

    return fileExtension;
}

QString File::getContentType(const QString& filePath)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(filePath);
    QString contentType = mimeType.name();

    return contentType;
}
