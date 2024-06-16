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

QString File::cropExtension(const QString &fileName) {
    QFileInfo fileInfo(fileName);
    return fileInfo.completeBaseName();
}

File File::findObjectById(const QList<File> &list, const QString &id, QString handler) {
    auto it = std::find_if(list.begin(), list.end(), [&id, &handler](const File &file) {
        return handler == "gdrive"? file.getGoogleId() == id : file.getId().toString() == id;
    });
    if (it != list.end()) {
        return *it;
    } else {
        return File();
    }
}
