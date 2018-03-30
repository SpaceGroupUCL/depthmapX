include(../defaults.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../ThirdParty/Catch

SOURCES += \
    main.cpp \
    testentityparsing.cpp \
    dxfptest.cpp

HEADERS +=

win32:Release:LIBS += -L../importlib/release -L../genlib/release -L../salalib/release
win32:Debug:LIBS += -L../importlib/debug -L../genlib/debug -L../salalib/debug
!win32:LIBS += -L../importlib -L../genlib -L../salalib

LIBS += -limportlib -lgenlib
