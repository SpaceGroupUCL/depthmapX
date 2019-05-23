set(cliTest cliTest)
set(cliTest_SRCS 
    main.cpp
    ../depthmapXcli/commandlineparser.cpp
    testcommandlineparser.cpp
    testradiusconverter.cpp
    ../depthmapXcli/radiusconverter.cpp
    testsimpletimer.cpp
    testvgaparser.cpp
    ../depthmapXcli/vgaparser.cpp
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
    ../depthmapXcli/stepdepthparser.cpp
    teststepdepthparser.cpp
    ../depthmapXcli/segmentparser.cpp
    testsegmentparser.cpp
    ../depthmapXcli/mapconvertparser.cpp
    testmapconvertparser.cpp)


include_directories("../ThirdParty/Catch" "../ThirdParty/FakeIt")

set(LINK_LIBS salalib genlib mgraph440)

add_executable(${cliTest} ${cliTest_SRCS})
target_link_libraries(${cliTest} ${LINK_LIBS})

