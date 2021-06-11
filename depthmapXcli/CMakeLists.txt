set(depthmapXcli depthmapXcli)

set(depthmapXcli_SRCS
    main.cpp
    printcommunicator.cpp
    commandlineparser.cpp
    runmethods.cpp
    radiusconverter.cpp
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
    segmentparser.cpp
    mapconvertparser.cpp)

set(LINK_LIBS salalib genlib mgraph440)

set(modules_cli "" CACHE INTERNAL "modules_cli" FORCE)
set(MODULES_GUI FALSE)
set(MODULES_CLI TRUE)
set(MODULES_CLI_TEST FALSE)
set(MODULES_CORE FALSE)
set(MODULES_CORE_TEST FALSE)
add_subdirectory(../modules modules)

add_executable(${depthmapXcli} ${depthmapXcli_SRCS})
target_link_libraries(${depthmapXcli} ${LINK_LIBS} ${modules_cli} ${modules_core})

