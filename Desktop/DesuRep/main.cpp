#include "globals.h"
#include "windows.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    windControl->windows["a"] = new AuthForm();
    windControl->updates["a"] = true;

    // Check Databases
    QList dbStatus = database->getDBStatus();
    if (dbStatus[0] == 0) {
        QMessageBox::critical(0,"","  ⚠ Local DB error\n\n" "Data base is out of range!");
        exit(-1);
    } else if (dbStatus[1] == 0) {
        QMessageBox::critical(0,"","  ⚠ Remote DB error\n\n" "Data base is out of range!");
        exit(-1);
    }

    windControl->windows["m"] = new MainWindow();
    windControl->updates["m"] = false;

    windControl->WindowSwap("a");

    return a.exec();
}
