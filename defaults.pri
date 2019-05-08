INCLUDEPATH += $$PWD/depthmapX $$PWD

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui

win32: QMAKE_CXXFLAGS_WARN_ON -= -W3
msvc: QMAKE_CXXFLAGS_WARN_ON += -W4

linux: QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual
mac: QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual
