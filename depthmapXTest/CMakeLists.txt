set(depthmapXTest depthmapXTest)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)

include_directories(${QT})

add_compile_definitions(_DEPTHMAP)

set(depthmapXTest_SRCS
    main.cpp
    testgllines.cpp
    testgllinesuniform.cpp
    testglrastertexture.cpp
    ../depthmapX/views/glview/gllines.h
    ../depthmapX/views/glview/gllines.cpp
	../depthmapX/views/glview/gllinesuniform.h
	../depthmapX/views/glview/gllinesuniform.cpp
	../depthmapX/views/glview/glrastertexture.h
	../depthmapX/views/glview/glrastertexture.cpp)


include_directories("../ThirdParty/Catch")

set(LINK_LIBS salalib genlib mgraph440)

add_executable(${depthmapXTest} ${depthmapXTest_SRCS})

find_package(OpenGL REQUIRED)

target_link_libraries(${depthmapXTest} Qt5::OpenGL salalib genlib OpenGL::GL OpenGL::GLU)

