#ifndef WINDOWCONTROLLER_H
#define WINDOWCONTROLLER_H

#include <QObject>
#include <QWidget>

#include "user.h"

class WindowController : public QObject
{
    Q_OBJECT

public:
    WindowController(QWidget *initWindow = nullptr);
    ~WindowController();

public:
    QWidget *curWindow;
    User *session;

    std::map<QString, QWidget*> windows;
    std::map<QString, bool> updates;

public:
    void PrepareWindow(QWidget *window);
    void WindowSwap(QString toOpen);
    void clearSession();
};

#endif // WINDOWCONTROLLER_H
