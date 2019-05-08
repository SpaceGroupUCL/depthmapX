include(../defaults.pri)
include(dialogs/dialogs.pri)
include(views/views.pri)

QT            += core gui opengl widgets
DEFINES       += _DEPTHMAP
TEMPLATE      = lib
CONFIG        += staticlib c++11
TARGET        = depthmapX
HEADERS       += GraphDoc.h \
                indexWidget.h \
                mainwindow.h \
                mdichild.h \
                treeWindow.h \
    compatibilitydefines.h \
    mainwindowfactory.h \
    version.h \
    settings.h \
    settingsimpl.h

SOURCES       += GraphDoc.cpp \
                indexWidget.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                renderthread.cpp \
                treeWindow.cpp \
    mainwindowfactory.cpp \
    settingsimpl.cpp

RESOURCES     += resource.qrc

OTHER_FILES += \
    Libs/include/generic/lgpl.txt

QMAKE_CXXFLAGS_WARN_ON =

FORMS += \
    UI/TopoMetDlg.ui \
    UI/SegmentAnalysisDlg.ui \
    UI/RenameObjectDlg.ui \
    UI/PushDialog.ui \
    UI/PromptReplace.ui \
    UI/OptionsDlg.ui \
    UI/NewLayerDlg.ui \
    UI/MakeOptionsDlg.ui \
    UI/MakeLayerDlg.ui \
    UI/LicenceDialog.ui \
    UI/LayerChooserDlg.ui \
    UI/IsovistPathDlg.ui \
    UI/InsertColumnDlg.ui \
    UI/GridDialog.ui \
    UI/FindLocDlg.ui \
    UI/FilePropertiesDlg.ui \
    UI/FewestLineOptionsDlg.ui \
    UI/EditConnectionsDlg.ui \
    UI/DepthmapOptionsDlg.ui \
    UI/DepthmapAlert.ui \
    UI/ConvertShapesDlg.ui \
    UI/ColumnPropertiesDlg.ui \
    UI/ColourScaleDlg.ui \
    UI/AxialAnalysisOptionsDlg.ui \
    UI/AttributeSummary.ui \
    UI/AttributeChooserDlg.ui \
    UI/AgentAnalysisDlg.ui \
    UI/AboutDlg.ui \
    UI/licenseagreement.ui

win32: system(make_version_header.bat)
!win32: system(sh ./make_version_header.sh)

win32:LIBS += -lopengl32 -lglu32 -lgdi32
