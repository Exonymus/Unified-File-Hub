#include "fileupload.h"


FileUploadDialog::FileUploadDialog(QWidget *parent) : QDialog(parent)
{
    QFormLayout *formLayout = new QFormLayout(this);

    uploadTimer = new QTimer();
    connect(uploadTimer, SIGNAL(timeout()), this, SLOT(fieldsCheck()));
    uploadTimer->start(100);
    fType = "";

    // File Name
    fileNameLineEdit = new QLineEdit(this);
    fileNameLineEdit->setMaximumWidth(260);
    fileNameLineEdit->setMinimumWidth(260);
    formLayout->addRow("File Name:", fileNameLineEdit);

    // File Path
    filePathLineEdit = new QLineEdit(this);
    filePathLineEdit->setMaximumWidth(260);
    filePathLineEdit->setMinimumWidth(260);
    formLayout->addRow("File Path:", filePathLineEdit);

    // Description
    descriptionTextEdit = new QTextEdit(this);
    descriptionTextEdit->setMaximumWidth(260);
    descriptionTextEdit->setMinimumWidth(260);
    formLayout->addRow("Description:", descriptionTextEdit);

    // Category
    categoryLineEdit = new QLineEdit(this);
    categoryLineEdit->setMaximumWidth(260);
    categoryLineEdit->setMinimumWidth(260);
    formLayout->addRow("Category:", categoryLineEdit);

    // Tag
    tagLineEdit = new QLineEdit(this);
    tagLineEdit->setMaximumWidth(260);
    tagLineEdit->setMinimumWidth(260);
    formLayout->addRow("Tag:", tagLineEdit);

    // Is Public
    isPublicCheckBox = new QCheckBox("Public", this);
    formLayout->addRow("", isPublicCheckBox);

    // File Select Button
    QPushButton *fileSelectBtn = new QPushButton("Select File", this);
    connect(fileSelectBtn, &QPushButton::clicked, this, &FileUploadDialog::selectFile);
    formLayout->addRow(fileSelectBtn);

    // File Upload Button
    fileUploadBtn = new QPushButton("Upload File", this);
    connect(fileUploadBtn, &QPushButton::clicked, this, &FileUploadDialog::saveUpload);
    formLayout->addRow(fileUploadBtn);

    // Update Button
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &FileUploadDialog::cancelUpload);
    formLayout->addRow(cancelBtn);

    // Set up the dialog window
    setWindowTitle(working_mode == "edit"? "Edit File": "Upload File");
    setMinimumWidth(400);
    setMaximumWidth(400);
    setMinimumHeight(500);
    setMaximumHeight(500);
}

void FileUploadDialog::setRestrictions(qreal avSpace)
{
    spaceAvailableMB = avSpace;
}

void FileUploadDialog::selectFile()
{
    fPath = QFileDialog::getOpenFileName(this, tr("Select File"), QDir::homePath());
    QFile file(fPath);

    if (!fPath.isEmpty() && file.open(QIODevice::ReadOnly))
    {
        QByteArray fByteArray = file.readAll();

        file.close();
        fType = File::getContentType(fPath);

        if ((fByteArray.size() / (1024 * 1024)) > spaceAvailableMB) {
            QMessageBox::critical(this, "", "⚠ File size error!\n\nThe file has exceeded your space limit! "
                                         "Please select another file to upload.");
            fByteArray.clear();
            fType = "";
            fPath = "";
        }

        return;
    }
}

void FileUploadDialog::clearData()
{
    fileNameLineEdit->clear();
    filePathLineEdit->clear();
    descriptionTextEdit->clear();
    categoryLineEdit->clear();
    tagLineEdit->clear();
    isPublicCheckBox->setChecked(false);
    fType = "";
    fPath = "";
}

void FileUploadDialog::fieldsCheck()
{
    if (fileNameLineEdit->text().isEmpty() ||
        descriptionTextEdit->toPlainText().isEmpty() ||
        categoryLineEdit->text().isEmpty() ||
        tagLineEdit->text().isEmpty() ||
        fType == "" || fPath == "") {
        switchBtn(fileUploadBtn, false);
    } else {
        switchBtn(fileUploadBtn, true);
    }

}

File FileUploadDialog::getUploadData() const
{

    QJsonObject uploadData;

    uploadData["name"] = fileNameLineEdit->text();
    uploadData["mime_type"] = fType;
    uploadData["owner_id"] = windControl->session->getUsername();
    uploadData["path"] = filePathLineEdit->text().isEmpty()? "." : filePathLineEdit->text();
    uploadData["description"] = descriptionTextEdit->toPlainText();
    uploadData["category"] = categoryLineEdit->text();
    uploadData["tag"] = tagLineEdit->text();
    uploadData["is_public"] = int(isPublicCheckBox->isChecked());
    uploadData["BLOB_path"] = fPath;

    File to_upload = File(uploadData);

    return to_upload;
}

void FileUploadDialog::cancelUpload()
{
    reject();
}

void FileUploadDialog::saveUpload()
{
    accept();
}
