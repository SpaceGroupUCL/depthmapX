include(../defaults.pri)

QT         += core gui opengl widgets
DEFINES    += _DEPTHMAP
TEMPLATE   = app
TARGET     = depthmapX
ICON       = icons/depthmapX.icns
SOURCES    = main.cpp \
    coreapplication.cpp

win32:RC_ICONS += icons/depthmapX.ico

win32:Release:LIBS += -L../depthmapX/release -L../genlib/release -L../mgraph440/release  -L../salalib/release -L../settingsdialog/release
win32:Debug:LIBS += -L../depthmapX/debug -L../genlib/debug -L../mgraph440/debug -L../salalib/debug -L../settingsdialog/debug
!win32:LIBS += -L../depthmapX -L../genlib -L../mgraph440 -L../salalib -L../settingsdialog

LIBS += -ldepthmapX -lsalalib -lmgraph440 -lgenlib -lsettingsdialog

!win32:!macx:LIBS += -L/usr/lib/i386-linux-gnu/

!win32:!macx:LIBS += -lGL -lGLU


win32:LIBS += -lOpenGl32 -lglu32 -lgdi32

HEADERS += \
    coreapplication.h

mac {
    QMAKE_INFO_PLIST = resources/Info.plist
    BUNDLE_RESOURCES.files = icons/graph.icns
    BUNDLE_RESOURCES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += BUNDLE_RESOURCES
}

FORMS += \
    ../depthmapX/UI/ColourScaleDlg.ui

