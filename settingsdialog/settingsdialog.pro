include(../defaults.pri)

QT       += widgets
DEFINES += _DEPTHMAP

TARGET = settingsdialog
TEMPLATE = lib

CONFIG += staticlib c++11

DEFINES += SETTINGSDIALOG_LIBRARY

SOURCES += \
    generalpage.cpp \
    interfacepage.cpp \
    settingsdialog.cpp

HEADERS += \
    generalpage.h \
    interfacepage.h \
    settingspage.h \
    settingsdialog.h

RESOURCES += \
    settingsdialog.qrc
