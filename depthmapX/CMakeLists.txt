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
	GraphDoc.h
	mainwindowfactory.h
	settingsimpl.h
	indexWidget.h
	mdichild.h
	treeWindow.h
	main.cpp
	coreapplication.cpp
        coreapplication.h
        mainwindowhelpers.cpp
        mainwindowmoduleregistry.cpp
        imainwindowmodulefactory.h)

qt5_wrap_ui(UI_HDRS
    UI/TopoMetDlg.ui
    UI/SegmentAnalysisDlg.ui
    UI/RenameObjectDlg.ui
    UI/PushDialog.ui
    UI/PromptReplace.ui
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

qt5_add_resources(DM_RSRC resource.qrc dialogs/settings/settingsdialog.qrc)

if (WIN32)
        add_executable(${depthmapX} WIN32 ${depthmapX_SRCS} ${UI_HDRS} ${DM_RSRC} icons.rc)
        if(MINGW)
            # https://stackoverflow.com/a/18138926
            # MinGW compilation does not bring in the required DLLs to make the executable self-contained
            # The missing DLLs are:
            # - libstdc++-6.dll
            # - libwinpthread-1.dll
            # - libgcc_s_seh-1.dll
            # Additionally, windeployqt does not drag them into the build directory, so it might be
            # necessary to manually find and copy them after compilation
            set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
        endif(MINGW)
endif(WIN32)
if(UNIX AND NOT APPLE)
        add_executable(${depthmapX} ${depthmapX_SRCS} ${UI_HDRS} ${DM_RSRC})
        if (Qt5_POSITION_INDEPENDENT_CODE)
            SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
        endif()
endif()
if(APPLE)
        set(MACOSX_BUNDLE_ICON_FILE depthmapX.icns)
        set(depthmapX_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icons/depthmapX.icns)
        set_source_files_properties(${depthmapX_ICON} PROPERTIES
               MACOSX_PACKAGE_LOCATION "Resources")
        set(graph_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icons/graph.icns)
        set_source_files_properties(${graph_ICON} PROPERTIES
               MACOSX_PACKAGE_LOCATION "Resources")
        add_definitions(-DGL_SILENCE_DEPRECATION)
        add_executable(${depthmapX} MACOSX_BUNDLE ${depthmapX_ICON} ${graph_ICON}
            ${depthmapX_SRCS} ${UI_HDRS} ${DM_RSRC})
        set_target_properties(${depthmapX} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist)
endif(APPLE)

find_package(OpenGL REQUIRED)

set(modules_gui "" CACHE INTERNAL "modules_gui" FORCE)
set(MODULES_GUI TRUE)
set(MODULES_CLI FALSE)
set(MODULES_CLI_TEST FALSE)
set(MODULES_CORE FALSE)
set(MODULES_CORE_TEST FALSE)
add_subdirectory(../modules modules)

target_link_libraries(${depthmapX} salalib genlib mgraph440 Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL
    OpenGL::GL OpenGL::GLU ${modules_gui} ${modules_core})

add_subdirectory(dialogs)
add_subdirectory(views)

