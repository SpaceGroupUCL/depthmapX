TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS = \
    mgraph440 \
    genlib \
    salalib \
    depthmapX \
    GuiUnitTest \
    depthmapXcli \
    cliTest \
    salaTest \
    genlibTest \
    depthmapXTest
GuiApp.depends = depthmapX genlib mgraph440 salalib
