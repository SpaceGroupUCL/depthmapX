set(depthmapX depthmapX)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)

add_compile_definitions(_DEPTHMAP)

set(depthmapX_SRCS
    GraphDoc.cpp
    indexWidget.cpp
    mainwindow.cpp
    mdichild.cpp
    renderthread.cpp
    treeWindow.cpp
    mainwindowfactory.cpp
    settingsimpl.cpp
	compatibilitydefines.h
	mainwindow.h
	settings.h
	version.h
	GraphDoc.h
	mainwindowfactory.h
	settingsimpl.h
	indexWidget.h
	mdichild.h
	treeWindow.h
	main.cpp
	coreapplication.cpp
	coreapplication.h)

qt5_wrap_ui(UI_HDRS
    UI/TopoMetDlg.ui
    UI/SegmentAnalysisDlg.ui
    UI/RenameObjectDlg.ui
    UI/PushDialog.ui
    UI/PromptReplace.ui
    UI/OptionsDlg.ui
    UI/NewLayerDlg.ui
    UI/MakeOptionsDlg.ui
    UI/MakeLayerDlg.ui
    UI/LicenceDialog.ui
    UI/LayerChooserDlg.ui
    UI/IsovistPathDlg.ui
    UI/InsertColumnDlg.ui
    UI/GridDialog.ui
    UI/FindLocDlg.ui
    UI/FilePropertiesDlg.ui
    UI/FewestLineOptionsDlg.ui
    UI/EditConnectionsDlg.ui
    UI/DepthmapOptionsDlg.ui
    UI/DepthmapAlert.ui
    UI/ConvertShapesDlg.ui
    UI/ColumnPropertiesDlg.ui
    UI/ColourScaleDlg.ui
    UI/AxialAnalysisOptionsDlg.ui
    UI/AttributeSummary.ui
    UI/AttributeChooserDlg.ui
    UI/AgentAnalysisDlg.ui
    UI/AboutDlg.ui
    UI/licenseagreement.ui)

qt5_wrap_cpp(UI_SRCS
	${UI_HDRS})	

qt5_add_resources(DM_RSRC resource.qrc dialogs/settings/settingsdialog.qrc)

if (MSVC)
        add_executable(${depthmapX} ${depthmapX_SRCS} ${UI_HDRS} ${UI_SRCS} ${DM_RSRC} icons.rc)
endif(MSVC)
if(UNIX)
        add_executable(${depthmapX} ${depthmapX_SRCS} ${UI_HDRS} ${UI_SRCS} ${DM_RSRC})
endif(UNIX)

find_package(OpenGL REQUIRED)

target_link_libraries(${depthmapX} salalib genlib mgraph440 Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL
    OpenGL::GL OpenGL::GLU)

add_subdirectory(dialogs)
add_subdirectory(views)
