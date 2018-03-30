#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T16:19:46
#
#-------------------------------------------------
include(../defaults.pri)


QT       -= qt
CONFIG   -= qt
CONFIG   -= app_bundle
DEFINES       += _DEPTHMAP
TARGET = salalib
TEMPLATE = lib
CONFIG        += staticlib c++11

DEFINES += SALALIB_LIBRARY

SOURCES += \
    attributes.cpp \
    axialmap.cpp \
    connector.cpp \
    datalayer.cpp \
    isovist.cpp \
    MapInfoData.cpp \
    mgraph.cpp \
    nagent.cpp \
    ngraph.cpp \
    pointdata.cpp \
    salaprogram.cpp \
    shapemap.cpp \
    spacepix.cpp \
    sparksieve2.cpp \
    topomet.cpp \
    linkutils.cpp \
    gridproperties.cpp \
    geometrygenerators.cpp \
    point.cpp \
    pafcolor.cpp

HEADERS += \
    attributes.h \
    axialmap.h \
    connector.h \
    datalayer.h \
    fileproperties.h \
    isovist.h \
    MapInfoData.h \
    mgraph.h \
    nagent.h \
    ngraph.h \
    pointdata.h \
    salaprogram.h \
    shapemap.h \
    spacepix.h \
    sparksieve2.h \
    topomet.h \
    linkutils.h \
    gridproperties.h \
    isovistdef.h \
    mgraph_consts.h \
    geometrygenerators.h \
    point.h \
    pixelref.h \
    displayparams.h \
    pafcolor.h \
    options.h

DISTFILES += \
    salascript-tests.txt
