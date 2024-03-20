#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtGlobal>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

#include "windowcontroller.h"
#include "database.h"
#include "apicontroller.h"

// Window-Controller
extern WindowController *windControl;

extern Database *database;

extern ApiController *webApi;

extern bool isOfflineMode;

extern void switchBtn(QPushButton *btn, bool state);
extern void dropShadow(QLabel *label);

#endif // GLOBALS_H
