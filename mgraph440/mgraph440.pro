include(../defaults.pri)


QT       -= qt
QT -= gui
CONFIG   -= qt


CONFIG += staticlib c++11 console
CONFIG -= app_bundle
TARGET = mgraph440
TEMPLATE = lib

DEFINES += MGRAPH440_LIBRARY

SOURCES += \
    pafcolor.cpp \
    attr.cpp \
    ngraph.cpp \
    datalayer.cpp \
    pointmap.cpp \
    attributes.cpp \
    connector.cpp \
    mgraph.cpp \
    axialmap.cpp \
    shapemap.cpp \
    pixelbase.cpp \
    spacepix.cpp \
    point.cpp \
    stringutils.cpp \
    p2dpoly.cpp \
    salaprogram.cpp \
    pafmath.cpp

HEADERS += \
    mgraph.h \
    mgraph_consts.h \
    spacepix.h \
    pointmap.h \
    paftl.h \
    p2dpoly.h \
    bspnode.h \
    ngraph.h \
    pixelref.h \
    shapemap.h \
    point.h \
    pafcolor.h \
    displayparams.h \
    options.h \
    fileproperties.h \
    attr.h \
    pixelbase.h \
    datalayer.h \
    attributes.h \
    containerutils.h \
    exceptions.h \
    axialmap.h \
    connector.h \
    stringutils.h \
    mapinfodata.h \
    legacyconverters.h \
    salaprogram.h \
    pafmath.h \
    comm.h
