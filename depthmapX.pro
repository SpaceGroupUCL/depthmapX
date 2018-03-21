TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS = \
    mgraph440 \
    genlib \
    salalib \
    settingsdialog \
    depthmapX \
    GuiUnitTest \
    GuiApp \
    depthmapXcli \
    cliTest \
    salaTest \
    genlibTest \
    depthmapXTest
GuiApp.depends = depthmapX genlib mgraph440 salalib settingsdialog
