#include "filetreewidget.h"

FileTreeWidget::FileTreeWidget(QTreeWidget *treeWidget, QString storageName, QTextBrowser *textBrowser, QWidget *parent) : QObject{parent}
{
    files = new QList<File>;

    fileInfo = textBrowser;
    name = storageName;
    tree = treeWidget;

//    spaceLimitMB = 0.00001;
    spaceLimitMB = 102;

    QObject::connect(tree, &QTreeWidget::itemSelectionChanged, this, &FileTreeWidget::handleFileSelectionChanged);
}

void FileTreeWidget::updateFiles()
{
    // Очистим файлы
    tree->clear();
    totalSpaceUsedMB = 0;

    // Создадим основные каталоги
    QTreeWidgetItem *publicFilesItem = new QTreeWidgetItem(tree);
    publicFilesItem->setText(0, "Public Files");
    publicFilesItem->setIcon(0, QIcon(":icons/home-folder"));

    QTreeWidgetItem *myFilesItem = new QTreeWidgetItem(tree);
    myFilesItem->setText(0, "My Files");
    myFilesItem->setIcon(0, QIcon(":icons/home-folder"));

    // Итерируемся по найденным файлам
    for (const File &file : *files)
    {
        bool isOwned = file.getUploader() == windControl->session->getUsername();
        if (isOwned) { totalSpaceUsedMB += file.getSizeInMB(); }

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
        setFileInfo(selectedItem);
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

    QString header = "<h3 style='text-align: center;'><b>Properties of '";
    QString body = "<h4><ul>";
    if (isFolder) {
        header += item->text(0) +"' folder</h3></b><br>";
        body += "<li><i>Files Stored:</i> " + QString::number(item->childCount()) + "</li>";
    } else {
        QString publicity = metaData["Public"].toBool()? "public" : "private";
        header += item->text(0) + "." + File::getFileExtensionFromMimeType(metaData["Type"].toString()) +"'</h3></b>";
        for (auto it = metaData.begin(); it != metaData.end(); ++it)
        {
            QString key = it.key();
            if (key == "id" || key == "DownloadUrl") continue;
            QString value = it.value().toString();
            if (key == "Public") {value = publicity; key = "Publicity";}
            if (key == "Size") {
                double size = it.value().toDouble();
                size = size < 0.1? 0 : size;
                QString strSize = QString::number(size, 'f', 2);
                strSize = (strSize == "0.00"? "&lt; 0.1" : strSize);

                value = strSize + " MB";
            }
            body += "<li><i>" + key + ":</i> " + value + "</li>";
        }
    }
    body += "</h4></ul>";
    fileInfo->append(header);
    fileInfo->append(body);
}

void FileTreeWidget::refreshFiles()
{
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

    connect(webApi, &ApiController::filesUpdated, this, [&, this, expandedFolders]() {
        // Обновить список файлов
        updateFiles();

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
    });

    webApi->getUserFiles(windControl->session->getUsername(), *files);
}
