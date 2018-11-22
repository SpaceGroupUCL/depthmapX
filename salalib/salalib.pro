#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T16:19:46
#
#-------------------------------------------------
include(../defaults.pri)
include(vgamodules/vgamodules.pri)
include(axialmodules/axialmodules.pri)
include(segmmodules/segmmodules.pri)
include(parsers/parsers.pri)

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
    mgraph.cpp \
    nagent.cpp \
    ngraph.cpp \
    pointdata.cpp \
    salaprogram.cpp \
    shapemap.cpp \
    spacepix.cpp \
    sparksieve2.cpp \
    entityparsing.cpp \
    linkutils.cpp \
    gridproperties.cpp \
    attributetable.cpp \
    layermanagerimpl.cpp \
    attributetableview.cpp \
    geometrygenerators.cpp \
    point.cpp \
    pafcolor.cpp \
    spacepixfile.cpp \
    alllinemap.cpp \
    axialminimiser.cpp \
    axialpolygons.cpp \
    tidylines.cpp \
    mapconverter.cpp \
    importutils.cpp \
    attributetableindex.cpp

HEADERS += \
    attributes.h \
    axialmap.h \
    connector.h \
    fileproperties.h \
    isovist.h \
    mgraph.h \
    nagent.h \
    ngraph.h \
    pointdata.h \
    salaprogram.h \
    shapemap.h \
    spacepix.h \
    sparksieve2.h \
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
    point.h \
    pixelref.h \
    displayparams.h \
    pafcolor.h \
    options.h \
    spacepixfile.h \
    alllinemap.h \
    axialminimiser.h \
    tolerances.h \
    axialpolygons.h \
    tidylines.h \
    mapconverter.h \
    ivga.h \
    iaxial.h \
    isegment.h \
    importutils.h \
    importtypedefs.h

DISTFILES += \
    salascript-tests.txt
