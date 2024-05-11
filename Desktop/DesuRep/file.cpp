#include "file.h"

File::File(const QJsonObject& data, QByteArray blob)
{
    metaData = data;
    rawFileData = blob;
    is_empty = false;
}

File::File() {
    is_empty = true;
}

File::~File() { }

qreal File::getBlobSizeInMB() const
{
    const qint64 bytes = rawFileData.size();
    const qreal megabytes = static_cast<qreal>(bytes) / (1024 * 1024);
    return megabytes;
}

qreal File::getSizeInMB() const
{
    return metaData["Size"].toDouble();
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
