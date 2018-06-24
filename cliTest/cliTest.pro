include(../defaults.pri)
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../ThirdParty/Catch ../ThirdParty/FakeIt

SOURCES += main.cpp \
    ../depthmapXcli/commandlineparser.cpp \
    testcommandlineparser.cpp \
    testradiusconverter.cpp \
    ../depthmapXcli/radiusconverter.cpp \
    testsimpletimer.cpp \
    testvgaparser.cpp \
    ../depthmapXcli/vgaparser.cpp \
    testlinkparser.cpp \
    ../depthmapXcli/linkparser.cpp \
    testagentparser.cpp \
    ../depthmapXcli/agentparser.cpp \
    testargumentholder.cpp \
    ../depthmapXcli/performancewriter.cpp \
    testperformancewriter.cpp \
    testselfcleaningfile.cpp \
    ../depthmapXcli/runmethods.cpp \
    ../depthmapXcli/modeparserregistry.cpp \
    testvisprepparser.cpp \
    ../depthmapXcli/visprepparser.cpp \
    testaxialparser.cpp \
    ../depthmapXcli/axialparser.cpp \
    testparsingutils.cpp \
    ../depthmapXcli/parsingutils.cpp \
    testisovistparser.cpp \
    ../depthmapXcli/isovistparser.cpp \
    testexportparser.cpp \
    ../depthmapXcli/exportparser.cpp \
    ../depthmapXcli/importparser.cpp \
    testimportparser.cpp \
    ../depthmapXcli/stepdepthparser.cpp \
    teststepdepthparser.cpp \
    ../depthmapXcli/segmentparser.cpp \
    testsegmentparser.cpp


HEADERS += \
    ../depthmapXcli/commandlineparser.h \
    ../depthmapXcli/radiusconverter.h \
    ../depthmapXcli/simpletimer.h \
    ../depthmapXcli/vgaparser.h \
    ../depthmapXcli/linkparser.h \
    ../depthmapXcli/agentparser.h \
    ../depthmapXcli/permformancewriter.h \
    argumentholder.h \
    selfcleaningfile.h

win32:Release:LIBS += -L../genlib/release -L../mgraph440/release -L../salalib/release
win32:Debug:LIBS += -L../genlib/debug -L../mgraph440/debug -L../salalib/debug
!win32:LIBS += -L../genlib -L../mgraph440 -L../salalib

LIBS += -lsalalib -lmgraph440 -lgenlib
