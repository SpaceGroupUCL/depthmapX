include(../defaults.pri)
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../ThirdParty/Catch

SOURCES += convertertest.cpp \
    main.cpp \
    dxfptest.cpp \
    teststringutils.cpp \
    testbspnode.cpp

HEADERS +=

win32:Release:LIBS += -L../genlib/release
win32:Debug:LIBS += -L../genlib/debug
!win32:LIBS += -L../genlib -L../salalib

LIBS += -lgenlib
