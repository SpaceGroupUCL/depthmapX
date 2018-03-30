TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS = \
    mgraph440 \
    genlib \
    importlib \
    salalib \
    settingsdialog \
    depthmapX \
    GuiUnitTest \
    GuiApp \
    depthmapXcli \
    cliTest \
    salaTest \
    genlibTest \
    depthmapXTest \
    importlibTest
GuiApp.depends = depthmapX genlib mgraph440 importlib salalib settingsdialog
