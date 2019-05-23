set(depthmapXcli depthmapXcli)

set(depthmapXcli_SRCS
    main.cpp
    commandlineparser.cpp
    runmethods.cpp
    radiusconverter.cpp
    vgaparser.cpp
    linkparser.cpp
    performancewriter.cpp
    modeparserregistry.cpp
    visprepparser.cpp
    axialparser.cpp
    parsingutils.cpp
    agentparser.cpp
    isovistparser.cpp
    exportparser.cpp
    importparser.cpp
    stepdepthparser.cpp
    segmentparser.cpp
    mapconvertparser.cpp)

set(LINK_LIBS salalib genlib mgraph440)

add_executable(${depthmapXcli} ${depthmapXcli_SRCS})
target_link_libraries(${depthmapXcli} ${LINK_LIBS})
