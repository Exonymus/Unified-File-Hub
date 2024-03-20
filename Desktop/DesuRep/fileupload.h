#ifndef FILEUPLOAD_H
#define FILEUPLOAD_H


#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include "QCheckBox"
#include <QVBoxLayout>
#include <QTimer>

#include "file.h"
#include "globals.h"

class FileUploadDialog : public QDialog
{
    Q_OBJECT

public:
    FileUploadDialog(QWidget *parent = nullptr);

    void setRestrictions(qreal avSpace);

    File getUploadData() const;

    void clearData();

private slots:
    void cancelUpload();

    void saveUpload();

    void selectFile();

    void fieldsCheck();

private:
    QTimer * uploadTimer;
    QString working_mode;
    QLineEdit *fileNameLineEdit;
    QLineEdit *filePathLineEdit;
    QTextEdit *descriptionTextEdit;
    QLineEdit *authorLineEdit;
    QLineEdit *themeLineEdit;
    QCheckBox *isPublicCheckBox;
    QPushButton *fileUploadBtn;
    QByteArray fByteArray;
    QString fType;
    qreal spaceAvailableMB;
};


#endif // FILEUPLOAD_H
