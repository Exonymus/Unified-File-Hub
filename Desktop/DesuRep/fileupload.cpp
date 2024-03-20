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

    // Author
    authorLineEdit = new QLineEdit(this);
    authorLineEdit->setMaximumWidth(260);
    authorLineEdit->setMinimumWidth(260);
    formLayout->addRow("Author:", authorLineEdit);

    // Theme
    themeLineEdit = new QLineEdit(this);
    themeLineEdit->setMaximumWidth(260);
    themeLineEdit->setMinimumWidth(260);
    formLayout->addRow("Theme:", themeLineEdit);

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
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select File"), QDir::homePath());
    QFile file(filePath);

    if (!filePath.isEmpty() && file.open(QIODevice::ReadOnly))
    {
        fByteArray = file.readAll();

        file.close();
        fType = File::getContentType(filePath);

        if ((fByteArray.size() / (1024 * 1024)) > spaceAvailableMB) {
            QMessageBox::critical(this, "", "⚠ File size error!\n\nThe file has exceeded your space limit! "
                                         "Please select another file to upload.");
            fByteArray.clear();
            fType = "";
        }

        return;
    }
}

void FileUploadDialog::clearData() {
    fileNameLineEdit->clear();
    filePathLineEdit->clear();
    descriptionTextEdit->clear();
    authorLineEdit->clear();
    themeLineEdit->clear();
    isPublicCheckBox->setChecked(false);
    fType = "";
    fByteArray.clear();

}

void FileUploadDialog::fieldsCheck()
{
    if (fileNameLineEdit->text().isEmpty() ||
        descriptionTextEdit->toPlainText().isEmpty() ||
        authorLineEdit->text().isEmpty() ||
        themeLineEdit->text().isEmpty() ||
        fByteArray.isEmpty() || fType == "") {
        switchBtn(fileUploadBtn, false);
    } else {
        switchBtn(fileUploadBtn, true);
    }

}

File FileUploadDialog::getUploadData() const
{

    QJsonObject uploadData;

    uploadData["Name"] = fileNameLineEdit->text();
    uploadData["Type"] = fType;
    uploadData["Uploader"] = windControl->session->getUsername();
    uploadData["Path"] = filePathLineEdit->text();
    uploadData["Description"] = descriptionTextEdit->toPlainText();
    uploadData["Author"] = authorLineEdit->text();
    uploadData["Theme"] = themeLineEdit->text();
    uploadData["Public"] = int(isPublicCheckBox->isChecked());

    File to_upload = File(uploadData, fByteArray);

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
