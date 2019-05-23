set(mgraph440 mgraph440)
set(mgraph440_SRCS
    pafcolor.cpp
    attr.cpp
    ngraph.cpp
    datalayer.cpp
    pointmap.cpp
    attributes.cpp
    connector.cpp
    mgraph.cpp
    axialmap.cpp
    shapemap.cpp
    pixelbase.cpp
    spacepix.cpp
    point.cpp
    stringutils.cpp
    p2dpoly.cpp
    salaprogram.cpp
    pafmath.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wnone")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W0 /EHsc /wd4800")
endif() 

add_compile_definitions(MGRAPH440_LIBRARY)

add_library(${mgraph440} ${mgraph440_SRCS})
