include(../defaults.pri)
QT            += core gui opengl widgets
DEFINES       += _DEPTHMAP
TEMPLATE      = lib
CONFIG        += staticlib
TARGET        = depthmapX
HEADERS       = depthmapView.h \
                GraphDoc.h \
                indexWidget.h \
                mainwindow.h \
                mdichild.h \
                treeWindow.h \
                3DView.h \
                PlotView.h \
                tableView.h \
    TopoMetDlg.h \
    SegmentAnalysisDlg.h \
    RenameObjectDlg.h \
    PushDialog.h \
    PromptReplace.h \
    OptionsDlg.h \
    NewLayerDlg.h \
    MakeOptionsDlg.h \
    MakeLayerDlg.h \
    LicenceDialog.h \
    LayerChooserDlg.h \
    IsovistPathDlg.h \
    InsertColumnDlg.h \
    GridDialog.h \
    FindLocDlg.h \
    FilePropertiesDlg.h \
    FewestLineOptionsDlg.h \
    EditConnectionsDlg.h \
    ConvertShapesDlg.h \
    ColumnPropertiesDlg.h \
    ColourScaleDlg.h \
    AxialAnalysisOptionsDlg.h \
    AttributeSummary.h \
    AttributeChooserDlg.h \
    AgentAnalysisDlg.h \
    AboutDlg.h \
    licenseagreement.h \
    compatibilitydefines.h \
    mainwindowfactory.h \
    viewhelpers.h \
    version.h \
    settings.h \
    settingsimpl.h \
    gllinesuniform.h \
    glview.h \
    gllines.h \
    glrastertexture.h \
    glpolygons.h \
    glutriangulator.h \
    gltrianglesuniform.h \
    glpointmap.h \
    glshapegraph.h \
    glshapemap.h \
    gldynamicrect.h \
    gldynamicline.h \
    mapview.h

SOURCES       = depthmapView.cpp \
                GraphDoc.cpp \
                indexWidget.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                renderthread.cpp \
                treeWindow.cpp \
                3DView.cpp \
                PlotView.cpp \
                tableView.cpp \
    TopoMetDlg.cpp \
    SegmentAnalysisDlg.cpp \
    RenameObjectDlg.cpp \
    PushDialog.cpp \
    PromptReplace.cpp \
    OptionsDlg.cpp \
    NewLayerDlg.cpp \
    MakeOptionsDlg.cpp \
    MakeLayerDlg.cpp \
    LicenceDialog.cpp \
    LayerChooserDlg.cpp \
    IsovistPathDlg.cpp \
    InsertColumnDlg.cpp \
    GridDialog.cpp \
    FindLocDlg.cpp \
    FilePropertiesDlg.cpp \
    FewestLineOptionsDlg.cpp \
    EditConnectionsDlg.cpp \
    ConvertShapesDlg.cpp \
    ColumnPropertiesDlg.cpp \
    ColourScaleDlg.cpp \
    AxialAnalysisOptionsDlg.cpp \
    AttributeSummary.cpp \
    AttributeChooserDlg.cpp \
    AgentAnalysisDlg.cpp \
    AboutDlg.cpp \
    licenseagreement.cpp \
    mainwindowfactory.cpp \
    viewhelpers.cpp \
    settingsimpl.cpp \
    gllinesuniform.cpp \
    glview.cpp \
    gllines.cpp \
    glrastertexture.cpp \
    glpolygons.cpp \
    glutriangulator.cpp \
    gltrianglesuniform.cpp \
    glpointmap.cpp \
    glshapegraph.cpp \
    glshapemap.cpp \
    gldynamicrect.cpp \
    gldynamicline.cpp \
    mapview.cpp

RESOURCES     = resource.qrc

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

