set(salalib salalib)

set(salalib_SRCS
    axialmap.cpp
    connector.cpp
    isovist.cpp
    mgraph.cpp
    ngraph.cpp
    pointdata.cpp
    salaprogram.cpp
    shapemap.cpp
    spacepix.cpp
    sparksieve2.cpp
    entityparsing.cpp
    linkutils.cpp
    gridproperties.cpp
    attributetable.cpp
    layermanagerimpl.cpp
    attributetableview.cpp
    geometrygenerators.cpp
    point.cpp
    pafcolor.cpp
    spacepixfile.cpp
    alllinemap.cpp
    axialminimiser.cpp
    axialpolygons.cpp
    tidylines.cpp
    mapconverter.cpp
    importutils.cpp
    attributetableindex.cpp
    tracemap.cpp)

add_compile_definitions(_DEPTHMAP SALALIB_LIBRARY)

add_library(${salalib} STATIC ${salalib_SRCS} ${vgamodules_SRCS} ${axialmodules_SRCS} ${segmmodules_SRCS} ${parsers_SRCS})

add_subdirectory(vgamodules)
add_subdirectory(axialmodules)
add_subdirectory(segmmodules)
add_subdirectory(parsers)
add_subdirectory(agents)



