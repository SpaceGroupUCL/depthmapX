set(cliTest cliTest)
set(cliTest_SRCS 
    main.cpp
    ../depthmapXcli/printcommunicator.cpp
    ../depthmapXcli/commandlineparser.cpp
    testcommandlineparser.cpp
    testradiusconverter.cpp
    ../depthmapXcli/radiusconverter.cpp
    testsimpletimer.cpp
    testlinkparser.cpp
    ../depthmapXcli/linkparser.cpp
    testagentparser.cpp
    ../depthmapXcli/agentparser.cpp
    testargumentholder.cpp
    ../depthmapXcli/performancewriter.cpp
    testperformancewriter.cpp
    testselfcleaningfile.cpp
    ../depthmapXcli/runmethods.cpp
    ../depthmapXcli/modeparserregistry.cpp
    testvisprepparser.cpp
    ../depthmapXcli/visprepparser.cpp
    testaxialparser.cpp
    ../depthmapXcli/axialparser.cpp
    testparsingutils.cpp
    ../depthmapXcli/parsingutils.cpp
    testisovistparser.cpp
    ../depthmapXcli/isovistparser.cpp
    testexportparser.cpp
    ../depthmapXcli/exportparser.cpp
    ../depthmapXcli/importparser.cpp
    testimportparser.cpp
    ../depthmapXcli/segmentparser.cpp
    testsegmentparser.cpp
    ../depthmapXcli/mapconvertparser.cpp
    testmapconvertparser.cpp)


include_directories("../ThirdParty/Catch" "../ThirdParty/FakeIt")

set(LINK_LIBS salalib genlib mgraph440)

set(modules_cliTest "" CACHE INTERNAL "modules_cliTest" FORCE)
set(MODULES_GUI FALSE)
set(MODULES_CLI FALSE)
set(MODULES_CLI_TEST TRUE)
set(MODULES_CORE FALSE)
set(MODULES_CORE_TEST FALSE)
add_subdirectory(../modules modules)

add_executable(${cliTest} ${cliTest_SRCS})
target_link_libraries(${cliTest} ${LINK_LIBS} ${modules_cli} ${modules_cliTest} ${modules_core})

