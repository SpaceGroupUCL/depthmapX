#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T16:17:22
#
#-------------------------------------------------
include(../defaults.pri)

QT       -= qt
CONFIG   -= qt app_bundle

TARGET = genlib
TEMPLATE = lib
CONFIG        += staticlib c++11

DEFINES += GENLIB_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dxfp.cpp \
    p2dpoly.cpp \
    pafmath.cpp \
    xmlparse.cpp \
    stringutils.cpp \
    bsptree.cpp

HEADERS += \
    comm.h \
    dxfp.h \
    p2dpoly.h \
    pafmath.h \
    paftl.h \
    paftl_old.h \
    xmlparse.h \
    exceptions.h \
    legacyconverters.h \
    stringutils.h \
    vectorhelpers.h \
    containerutils.h \
    linreg.h \
    bsptree.h
