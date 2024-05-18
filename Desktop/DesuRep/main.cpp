#include "globals.h"
#include "windows.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    windControl->windows["a"] = new AuthForm();
    windControl->updates["a"] = true;

    // Check Server state
    webApi->checkApiAvailability([](bool isAvailable) {
        if (isAvailable) {
            qDebug() << "API is available";
        } else {
            QMessageBox::critical(0,"","  ⚠ Connection error\n\n" "App Server is out of range!");
            exit(-1);
        }
    });

    windControl->windows["m"] = new MainWindow();
    windControl->updates["m"] = false;

    windControl->WindowSwap("a");

    return a.exec();
}
