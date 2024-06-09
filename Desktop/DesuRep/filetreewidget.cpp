#include "filetreewidget.h"

FileTreeWidget::FileTreeWidget(QTreeWidget *treeWidget,
                               QString storageName,
                               QTextBrowser *textBrowser,
                               QString handlerSystem,
                               QWidget *parent) : QObject{parent}
{
    files = new QList<File>;
    handler = handlerSystem;

    fileInfo = textBrowser;
    name = storageName;
    tree = treeWidget;

//    spaceLimitMB = 0.00001;
    spaceLimitMB = 1024;

    QObject::connect(tree, &QTreeWidget::itemSelectionChanged, this, &FileTreeWidget::handleFileSelectionChanged);
}

void FileTreeWidget::updateFiles()
{
    // Очистим файлы
    tree->clear();
    totalSpaceUsedMB = 0;

    // Создадим основные каталоги
    QTreeWidgetItem *publicFilesItem = new QTreeWidgetItem(tree);
    if (handler == "gdrive" || handler == "odrive") {
        publicFilesItem->setText(0, "Available Files");
    } else {
        publicFilesItem->setText(0, "Public Files");
    }
    publicFilesItem->setIcon(0, QIcon(":icons/home-folder"));

    QTreeWidgetItem *myFilesItem = new QTreeWidgetItem(tree);
    myFilesItem->setText(0, "My Files");
    myFilesItem->setIcon(0, QIcon(":icons/home-folder"));

    // Итерируемся по найденным файлам
    for (const File &file : *files)
    {
        bool isOwned = handler == "ufh"? file.getOwnerId() == windControl->session->getId() : !file.isPublic();
        if (isOwned && handler == "ufh") { totalSpaceUsedMB += file.getSizeInMB(); }

        // Файл не лежит в корне
        if (file.getPath() == ".") {
            QTreeWidgetItem *parentItem = nullptr;

            if (isOwned)
            {
                parentItem = myFilesItem;
            }
            else if (file.isPublic())
            {
                parentItem = publicFilesItem;
            }

            QTreeWidgetItem *fileItem = new QTreeWidgetItem(parentItem);
            fileItem->setText(0, file.getName());
            fileItem->setData(0, Qt::UserRole, file.getMetaData());
            fileItem->setIcon(0, QIcon(":icons/file"));
        } else {
            QStringList pathSegments = file.getPath().split("/", Qt::SkipEmptyParts);
            QTreeWidgetItem *parentItem = nullptr;

            for (const QString &segment : pathSegments)
            {
                // Находим / создаем каталог файла
                if (!parentItem)
                {
                    // Поиск каталога сверху
                    QTreeWidgetItem *parentItemToSearch = isOwned ? myFilesItem : publicFilesItem;

                    int childCount = parentItemToSearch->childCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        QTreeWidgetItem *item = parentItemToSearch->child(i);
                        if (item->text(0) == segment)
                        {
                            parentItem = item;
                            break;
                        }
                    }

                    if (!parentItem)
                    {
                        parentItem = isOwned ? new QTreeWidgetItem(myFilesItem) : new QTreeWidgetItem(publicFilesItem);
                        parentItem->setText(0, segment);
                        parentItem->setIcon(0, QIcon(":icons/folder"));
                    }
                }
                else
                {
                    // Поиск каталога внизу
                    QTreeWidgetItem *childItem = nullptr;
                    for (int i = 0; i < parentItem->childCount(); ++i)
                    {
                        if (parentItem->child(i)->text(0) == segment)
                        {
                            childItem = parentItem->child(i);
                            break;
                        }
                    }

                    if (!childItem)
                    {
                        // Если подпапки, нет создаем ее
                        childItem = new QTreeWidgetItem(parentItem);
                        childItem->setText(0, segment);
                        childItem->setIcon(0, QIcon(":icons/folder"));
                    }

                    parentItem = childItem;
                }
            }


            QTreeWidgetItem *fileItem = new QTreeWidgetItem(parentItem);
            fileItem->setText(0, file.getName());
            fileItem->setData(0, Qt::UserRole, file.getMetaData());
            fileItem->setIcon(0, QIcon(":icons/file"));
        }
    }
}

void FileTreeWidget::handleFileSelectionChanged()
{
    QTreeWidgetItem *selectedItem = tree->currentItem();

    if (selectedItem) {
        // Получим путь элемента
        QString filePath = getPath(selectedItem);

        // Установим визуально путь элемента
        tree->headerItem()->setText(0, name + filePath);

        // Установим информацию элемента
        handler == "ufh"? setFileInfo(selectedItem) : setFileInfoCompact(selectedItem);
    }
}

QString FileTreeWidget::getPath(QTreeWidgetItem *item)
{
    QStringList pathSegments;
    while (item) {
        pathSegments.prepend(item->text(0));
        item = item->parent();
    }
    return "/" + pathSegments.join("/");
}

void FileTreeWidget::setFileInfo(QTreeWidgetItem *item)
{
    fileInfo->clear();

    bool isFolder = item->data(0, Qt::UserRole).isNull();
    QJsonObject metaData = item->data(0, Qt::UserRole).toJsonObject();

    QString header;
    QString body = "<h4><ul>";

    if (isFolder)
    {
        header = QString("<h3 style='text-align: center;'><b>Properties of '%1' folder</b></h3><br>").arg(item->text(0));
        body += QString("<li><i>Files Stored:</i> %1</li>").arg(item->childCount());
    }
    else
    {
        QString fileName = item->text(0);
        QString fileExtension = File::getFileExtensionFromMimeType(metaData["mime_type"].toString());
        QString publicity = metaData["is_public"].toBool() ? "public" : "private";

        header = QString("<h3 style='text-align: center;'><b>Properties of '%1.%2'</b></h3>").arg(fileName, fileExtension);

        QHash<QString, QString> displayKeys = {
            {"mime_type", "Type"},
            {"path", "Path"},
            {"description", "Description"},
            {"upload_date", "Uploaded on"},
            {"category", "Category"},
            {"owner", "Owner"},
            {"tag", "Tag"},
            {"is_public", "Publicity"},
            {"size", "Size"}
        };

        for (auto it = metaData.constBegin(); it != metaData.constEnd(); ++it) {
            QString key = it.key();
            if (key == "id" || key == "name" || key == "owner_id") continue;

            QString displayKey = displayKeys.value(key, key);
            QString value = it.value().toString();

            if (key == "is_public") {
                value = publicity;
            } else if (key == "size") {
                double size = it.value().toDouble();
                size = size < 0.1 ? 0 : size;
                QString strSize = QString::number(size, 'f', 2);
                value = (strSize == "0.00" ? "&lt; 0.1" : strSize) + " MB";
            }

            body += QString("<li><i>%1:</i> %2</li>").arg(displayKey, value);
        }
    }

    body += "</ul></h4>";

    fileInfo->append(header);
    fileInfo->append(body);
}


void FileTreeWidget::setFileInfoCompact(QTreeWidgetItem *item)
{
    fileInfo->clear();

    bool isFolder = item->data(0, Qt::UserRole).isNull();
    QJsonObject metaData = item->data(0, Qt::UserRole).toJsonObject();

    QString header;
    QString body = "<h4><ul>";

    if (isFolder)
    {
        header = QString("<h3 style='text-align: center;'><b>Properties of '%1' folder</b></h3><br>").arg(item->text(0));
        body += QString("<li><i>Files Stored:</i> %1</li>").arg(item->childCount());
    }
    else
    {
        QString fileName = item->text(0);
        QString fileExtension = File::getFileExtensionFromMimeType(metaData["mime_type"].toString());

        header = QString("<h3 style='text-align: center;'><b>Properties of '%1.%2'</b></h3>").arg(fileName, fileExtension);

        QHash<QString, QString> displayKeys = {
            {"mime_type", "Type"},
            {"path", "Path"},
            {"upload_date", "Uploaded on"},
            {"owner", "Owner"},
            {"size", "Size"}
        };

        for (auto it = metaData.constBegin(); it != metaData.constEnd(); ++it) {
            QString key = it.key();
            if (key == "id" || key == "name" || key == "is_public"
                    || key == "parent" || key == "gdrive_mime_type") continue;

            QString displayKey = displayKeys.value(key, key);
            QString value = it.value().toString();

            if (key == "size") {
                double size = it.value().toDouble();
                size = size < 0.1 ? 0 : size;
                QString strSize = QString::number(size, 'f', 2);
                value = (strSize == "0.00" ? "&lt; 0.1" : strSize) + " MB";
            }

            body += QString("<li><i>%1:</i> %2</li>").arg(displayKey, value);
        }
    }

    body += "</ul></h4>";

    fileInfo->append(header);
    fileInfo->append(body);
}

void FileTreeWidget::refreshFiles() {
    // Сохранение состояния открытых папок
    QSet<QString> expandedFolders;
    QTreeWidgetItemIterator it(tree);
    while (*it) {
        if ((*it)->isExpanded()) {
            QString path = getPath(*it);
            expandedFolders.insert(path);
        }
        ++it;
    }

    tree->clearSelection();
    tree->headerItem()->setText(0, name);

    tree->clear();
    fileInfo->clear();

    auto updateFilesLambda = [&, this, expandedFolders]() {
        // Обновить список файлов
        updateFiles();

        if (handler != "ufh") {
            files = &google_api->GDFiles;
        }

        // Восстановление состояния открытых папок
        for (const QString &path : expandedFolders) {
            QTreeWidgetItemIterator it(tree);
            while (*it) {
                if (getPath(*it) == path) {
                    (*it)->setExpanded(true);
                    break;
                }
                ++it;
            }
        }

        emit spaceUsageUpdate();
    };

    if (handler == "ufh") {
        connect(webApi, &ApiController::filesUpdated, this, updateFilesLambda);
        webApi->getUserFiles(*windControl->session, *files);
    } else if (handler == "gdrive") {
        connect(google_api, &GoogleDriveAPI::listFilesCompleted, this, updateFilesLambda);
        google_api->getUserFiles();
    }
}
