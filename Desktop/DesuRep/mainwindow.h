#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include "QTreeWidget"
#include <QRegularExpression>
#include <QMessageBox>
#include <QRegularExpressionValidator>

#include "globals.h"
#include "filetreewidget.h"
#include "fileedit.h"
#include "fileupload.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void showFileStatus(QString status);
    void hideFileStatus();
    File findFileById(QUuid id, const QList<File> *fileList);
    bool showDeleteConfirmationDialog(QString fileName);


private slots:
    void sessionCheck();

    void actionsCheck();

    void on_refresh_files_clicked();

    void on_exit_btn_clicked();

    void on_so_btn_clicked();

    void on_actionCopy_triggered();

    void on_actionCut_triggered();

    void on_actionPaste_triggered();

    void on_actionDelete_triggered();

    void on_actionDownload_triggered();

    void on_actionUpload_triggered();

    void on_actionEdit_triggered();

    void on_change_email_btn_clicked();

    void on_change_pass_btn_clicked();

    void updateStorageUsage();

    void on_change_sq_btn_clicked();

private:
    QTimer *actionsTimer;
    QTimer *sessionTimer;
    Ui::MainWindow *ui;
    FileEditDialog editDialog;
    FileUploadDialog uploadDialog;

    File copyBuffer;
    File cutBuffer;

    //Storages
    FileTreeWidget *desuStorage;

    QJsonObject changedUserMetadata;

    bool fileOperationInProgress;

    bool showChangeEmailDialog();
    bool showChangePasswordDialog();
    bool showSecretQuestionRecoveryDialog();
};
#endif // MAINWINDOW_H
