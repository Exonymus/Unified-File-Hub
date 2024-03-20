#ifndef FILETREEWIDGET_H
#define FILETREEWIDGET_H

#include <QTreeWidget>
#include <QTextBrowser>
#include <QJsonDocument>

#include "globals.h"
#include "file.h"

class FileTreeWidget : public QObject
{
    Q_OBJECT

public:
    explicit FileTreeWidget(QTreeWidget *treeWidget, QString storageName, QTextBrowser *textBrowser, QWidget *parent = nullptr);

    void refreshFiles();

    QList<File> *getFiles() const { return files; }

    qreal spaceAvailableMB() const { return spaceLimitMB - totalSpaceUsedMB; }
    qreal spaceUsage() const { return totalSpaceUsedMB / spaceLimitMB * 100; }
private:
    void updateFiles();
    QString getPath(QTreeWidgetItem *item);
    void setFileInfo(QTreeWidgetItem *item);

    QTextBrowser *fileInfo;
    QList<File> *files;
    QString name;
    QTreeWidget *tree;

    qreal totalSpaceUsedMB;
    qreal spaceLimitMB;

private slots:
    void handleFileSelectionChanged();

signals:
    void spaceUsageUpdate();
};

#endif // FILETREEWIDGET_H
