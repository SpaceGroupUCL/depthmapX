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
    isovist.cpp \
    MapInfoData.cpp \
    mgraph.cpp \
    nagent.cpp \
    ngraph.cpp \
    ntfp.cpp \
    pointdata.cpp \
    salaprogram.cpp \
    shapemap.cpp \
    spacepix.cpp \
    sparksieve2.cpp \
    tigerp.cpp \
    topomet.cpp \
    entityparsing.cpp \
    linkutils.cpp \
    gridproperties.cpp \
    attributetable.cpp \
    layermanagerimpl.cpp \
    attributetableview.cpp \
    geometrygenerators.cpp \
    importutils.cpp \
    point.cpp \
    pafcolor.cpp

HEADERS += \
    attributes.h \
    axialmap.h \
    connector.h \
    fileproperties.h \
    isovist.h \
    MapInfoData.h \
    mgraph.h \
    nagent.h \
    ngraph.h \
    ntfp.h \
    pointdata.h \
    salaprogram.h \
    shapemap.h \
    spacepix.h \
    sparksieve2.h \
    tigerp.h \
    topomet.h \
    entityparsing.h \
    linkutils.h \
    gridproperties.h \
    isovistdef.h \
    mgraph_consts.h \
    attributetable.h \
    attributetableindex.h \
    layermanager.h \
    layermanagerimpl.h \
    attributetablehelpers.h \
    attributetableview.h \
    geometrygenerators.h \
    importutils.h \
    importtypedefs.h \
    point.h \
    pixelref.h \
    displayparams.h \
    pafcolor.h \
    options.h

DISTFILES += \
    salascript-tests.txt
