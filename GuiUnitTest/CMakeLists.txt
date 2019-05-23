set(GuiUnitTest GuiUnitTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

set(guiUnitTest_SRCS
    main.cpp 
    testviewhelpers.cpp 
    testsettings.cpp 
    ../depthmapX/settingsimpl.cpp 
    ../depthmapX/views/viewhelpers.cpp)

include_directories("../ThirdParty/Catch" "../ThirdParty/FakeIt" "../depthmapX")

set(LINK_LIBS salalib genlib mgraph440 Qt5::Core)

add_executable(${GuiUnitTest} ${guiUnitTest_SRCS})
target_link_libraries(${GuiUnitTest} ${LINK_LIBS})

