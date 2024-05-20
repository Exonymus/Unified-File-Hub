#ifndef FILEEDIT_H
#define FILEEDIT_H


#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include "QCheckBox"
#include <QVBoxLayout>

#include "file.h"

class FileEditDialog : public QDialog
{
    Q_OBJECT

public:
    FileEditDialog(QWidget *parent = nullptr);

    void setFileProperties(QJsonObject data);

   QJsonObject getEditedData() const;

private slots:
    void cancelChanges();

    void saveChanges();

private:
    QLineEdit *fileNameLineEdit;
    QLineEdit *filePathLineEdit;
    QTextEdit *descriptionTextEdit;
    QLineEdit *categoryLineEdit;
    QLineEdit *tagLineEdit;
    QCheckBox *isPublicCheckBox;
};

#endif // FILEEDIT_H
