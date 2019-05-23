project(depthmapX)
cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 11)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /EHsc")
    execute_process(COMMAND make_version_header.bat WORKING_DIRECTORY depthmapX)
endif()

# policy for target sources - we don't expect any old CMakes
cmake_policy(SET CMP0076 NEW)

include_directories(".")

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE APP_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE APP_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# generate version_defs.h
include_directories(${CMAKE_BINARY_DIR})
configure_file("${CMAKE_SOURCE_DIR}/version_defs.h.in" "${CMAKE_BINARY_DIR}/version_defs.h" @ONLY)

string(TIMESTAMP APP_DATE %Y-%m-%d)

add_subdirectory(genlib)
add_subdirectory(genlibTest)
add_subdirectory(salalib)
add_subdirectory(mgraph440)
add_subdirectory(mgraph440Test)
add_subdirectory(salaTest)
add_subdirectory(depthmapXcli)
add_subdirectory(cliTest)
add_subdirectory(depthmapXTest)
add_subdirectory(depthmapX)
add_subdirectory(GuiUnitTest)
