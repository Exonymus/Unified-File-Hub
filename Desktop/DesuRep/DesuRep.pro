QT      += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addftpserver.cpp \
    apicontroller.cpp \
    auth.cpp \
    database.cpp \
    enhasher.cpp \
    file.cpp \
    fileedit.cpp \
    filetreewidget.cpp \
    fileupload.cpp \
    ftpcontroller.cpp \
    globals.cpp \
    googledriveauth.cpp \
    googledriveapi.cpp \
    main.cpp \
    mainwindow.cpp \
    user.cpp \
    windowcontroller.cpp

ICON = resources/icons/icon_app.png

HEADERS += \
    addftpserver.h \
    apicontroller.h \
    auth.h \
    bcrypt.h \
    database.h \
    enhasher.h \
    ftpcontroller.h \
    googledriveauth.h \
    googledriveapi.h \
    file.h \
    fileedit.h \
    filetreewidget.h \
    fileupload.h \
    globals.h \
    mainwindow.h \
    user.h \
    windowcontroller.h \
    windows.h

FORMS += \
    auth.ui \
    mainwindow.ui

DEFINES += PROJECT_PATH=\"\\\"$${_PRO_FILE_PWD_}/\\\"\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES += \
    resources/local.db

macx: LIBS += -L$$PWD/libraries/ -lbcrypt
LIBS += -lcurl

INCLUDEPATH += $$PWD/libraries
DEPENDPATH += $$PWD/libraries

TARGET = "UFH Storage"
macx: PRE_TARGETDEPS += $$PWD/libraries/libbcrypt.a

QMAKE_MACOSX_DEPLOYMENT_TARGET = 14
