#include "globals.h"

//// ALL THE GLOBAL DEFINITIONS

// Window-Controller init
WindowController *windControl= new WindowController();

Database *database = new Database();

ApiController *webApi = new ApiController();


QStringList secret_questions =
{
    "What is your favorite color?",
    "What was the name of your first pet?",
    "What is your mother’s maiden name?",
    "What is the name of the city where you were born?",
    "What was your first car?"
};

void switchBtn(QPushButton *btn, bool state)
{
    if (state == false)
    {
        btn->setStyleSheet("color: red;");
        btn->setDisabled(true);
    }
    else
    {
        btn->setStyleSheet("color: black;");
        btn->setEnabled(true);
    }
}

void dropShadow(QLabel *label) {
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(label);
    effect->setOffset(0, 0);
    effect->setBlurRadius(12);
    effect->setColor(Qt::black);
    label->setGraphicsEffect(effect);
}
