set(salaTest salaTest)

set(salaTest_SRCS
    main.cpp
    testentityparsing.cpp
    testpointmap.cpp
    testlinkutils.cpp
    testgridproperties.cpp
    testisovistdef.cpp
    testmgraph.cpp
    testshapegraphs.cpp
    teststructsizes.cpp
    testsparksieve.cpp
    testattributetable.cpp
    testattributetableindex.cpp
    testlayermanager.cpp
    testattributetablehelpers.cpp
    testattributetableview.cpp
    testshapemaps.cpp
    testgeometrygenerators.cpp
    testmapinfodata.cpp
    testsalaprogram.cpp
    testdxfp.cpp)

include_directories("../ThirdParty/Catch" "../ThirdParty/FakeIt")

set(LINK_LIBS salalib genlib mgraph440)

add_executable(${salaTest} ${salaTest_SRCS})
target_link_libraries(${salaTest} ${LINK_LIBS})
