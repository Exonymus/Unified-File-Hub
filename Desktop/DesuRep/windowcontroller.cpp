#include "windowcontroller.h"


WindowController::WindowController(QWidget *initWindow)
{
    curWindow = initWindow;
    session = new User();
}

WindowController::~WindowController()
{
    for (auto const& window : windows)
    {
        window.second->close();
    }

    delete curWindow;
}

void WindowController::PrepareWindow(QWidget *window)
{
    QPalette palette = window->palette();
    palette.setBrush(QPalette::Window,
                QBrush(QPixmap(":/overlay/background").scaled(
                    window->size(),
                    Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation)));
    window->setPalette(palette);
}

void WindowController::clearSession()
{
    delete session;
    session = new User();
}


void WindowController::WindowSwap(QString toOpen)
{
    if (curWindow)
    {
        curWindow->hide();

        //  Window pos control
        windows[toOpen]->
                move(curWindow->pos().x(), curWindow->pos().y());
    }

    curWindow = windows[toOpen];
    curWindow->show();
}
