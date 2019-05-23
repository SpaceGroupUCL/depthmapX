set(mgraph440Test mgraph440Test)
set(mgraph440Test_SRCS
    main.cpp
    testcontainers.cpp
    testconverters.cpp)

set(LINK_LIBS
    salalib
    mgraph440
    genlib)

include_directories("../ThirdParty/Catch")

add_executable(${mgraph440Test} ${mgraph440Test_SRCS})
target_link_libraries(${mgraph440Test} ${LINK_LIBS})
