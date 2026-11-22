set(GuiUnitTest GuiUnitTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
set(guiUnitTest_SRCS
    main.cpp
    testmainwindowhelpers.cpp
    testrenderthread.cpp
    testviewhelpers.cpp
    testsettings.cpp)

include_directories("../ThirdParty/Catch" "../ThirdParty/FakeIt" "../depthmapX")

add_executable(${GuiUnitTest} ${guiUnitTest_SRCS})
target_link_libraries(${GuiUnitTest} depthmapXGui)

