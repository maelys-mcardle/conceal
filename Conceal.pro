#-------------------------------------------------
#
# Project created by QtCreator 2013-01-20T17:41:59
#
#-------------------------------------------------

QT       += core gui declarative

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Conceal
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
	passworddialog.cpp \
	cryptothread.cpp \
    encryptdecryptdialog.cpp

HEADERS  += mainwindow.h \
	passworddialog.h \
    cryptothread.h \
    encryptdecryptdialog.h

FORMS    += mainwindow.ui \
    passworddialog.ui \
    encryptdecryptdialog.ui

RESOURCES += \
    resource.qrc

OTHER_FILES += \
    droparea.qml

LIBS += \
	-lmcrypt
