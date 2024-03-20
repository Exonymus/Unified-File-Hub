#include "fileedit.h"


FileEditDialog::FileEditDialog(QWidget *parent) : QDialog(parent)
{
    QFormLayout *formLayout = new QFormLayout(this);

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

    // File Selection Button
    QPushButton *saveChangesBtn = new QPushButton("Save Changes", this);
    connect(saveChangesBtn, &QPushButton::clicked, this, &FileEditDialog::saveChanges);
    formLayout->addRow(saveChangesBtn);

    // Update Button
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &FileEditDialog::cancelChanges);
    formLayout->addRow(cancelBtn);

    // Set up the dialog window
    setWindowTitle("Edit File");
    setMinimumWidth(400);
    setMaximumWidth(400);
    setMinimumHeight(500);
    setMaximumHeight(500);
}

void FileEditDialog::setFileProperties(QJsonObject data)
{
    fileNameLineEdit->setText(data["Name"].toString());
    filePathLineEdit->setText(data["Path"].toString());
    descriptionTextEdit->setText(data["Description"].toString());
    authorLineEdit->setText(data["Author"].toString());
    themeLineEdit->setText(data["Theme"].toString());
    isPublicCheckBox->setChecked(data["Public"].toBool());
}

QJsonObject FileEditDialog::getEditedData() const
{
    QJsonObject editedData;

    editedData["Name"] = fileNameLineEdit->text();
    editedData["Path"] = filePathLineEdit->text();
    editedData["Description"] = descriptionTextEdit->toPlainText();
    editedData["Author"] = authorLineEdit->text();
    editedData["Theme"] = themeLineEdit->text();
    editedData["Public"] = int(isPublicCheckBox->isChecked());

    return editedData;
}

void FileEditDialog::cancelChanges()
{
    reject();
}

void FileEditDialog::saveChanges()
{
    accept();
}
