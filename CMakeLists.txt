cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum macOS deployment version")
project(depthmapX)
set(CMAKE_CXX_STANDARD 17)

option(DEPTHMAPX_ENABLE_IPO "Enable interprocedural optimisation for Release builds" OFF)

if (DEPTHMAPX_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT DEPTHMAPX_IPO_SUPPORTED OUTPUT DEPTHMAPX_IPO_ERROR)
    if (DEPTHMAPX_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "IPO/LTO is not supported by this toolchain: ${DEPTHMAPX_IPO_ERROR}")
    endif()
endif()

if (MSVC)
    # /MP improves build time. /Oi, /Gy and linker folding improve Release runtime/size.
    add_compile_options(/MP "$<$<CONFIG:Release>:/Oi>" "$<$<CONFIG:Release>:/Gy>")
    add_link_options("$<$<CONFIG:Release>:/OPT:REF>" "$<$<CONFIG:Release>:/OPT:ICF>")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /EHsc")
endif()

# policy for target sources - we don't expect any old CMakes
cmake_policy(SET CMP0076 NEW)

include_directories(".")

# Generate version metadata without relying on the deprecated Windows WMIC tool.
set(APP_BRANCH "unknown")
set(APP_COMMIT "unknown")
find_package(Git QUIET)
find_package(Threads REQUIRED)
if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE APP_BRANCH_RESULT
        OUTPUT_VARIABLE APP_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (NOT APP_BRANCH_RESULT EQUAL 0 OR APP_BRANCH STREQUAL "")
        set(APP_BRANCH "unknown")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -1 --format=%h
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE APP_COMMIT_RESULT
        OUTPUT_VARIABLE APP_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (NOT APP_COMMIT_RESULT EQUAL 0 OR APP_COMMIT STREQUAL "")
        set(APP_COMMIT "unknown")
    endif()
endif()

string(TIMESTAMP APP_DATE "%Y-%m-%d")
include_directories("${CMAKE_BINARY_DIR}")
configure_file("${CMAKE_SOURCE_DIR}/version_defs.h.in" "${CMAKE_BINARY_DIR}/version_defs.h" @ONLY)

set(modules_core "" CACHE INTERNAL "modules_core" FORCE)
set(MODULES_GUI FALSE)
set(MODULES_CLI FALSE)
set(MODULES_CLI_TEST FALSE)
set(MODULES_CORE TRUE)
set(MODULES_CORE_TEST FALSE)

add_subdirectory(genlib)
add_subdirectory(genlibTest)
add_subdirectory(salalib)
add_subdirectory(modules) # only the core modules are loaded here
add_subdirectory(mgraph440)
add_subdirectory(mgraph440Test)
add_subdirectory(salaTest)
add_subdirectory(depthmapXcli)
add_subdirectory(cliTest)
add_subdirectory(depthmapXTest)
add_subdirectory(depthmapX)
add_subdirectory(GuiUnitTest)
add_subdirectory(moduleTest)

