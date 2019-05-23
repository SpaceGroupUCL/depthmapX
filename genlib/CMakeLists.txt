set(genlib genlib)
set(genlib_SRCS
    bsptree.cpp  
    p2dpoly.cpp  
    pafmath.cpp  
    stringutils.cpp  
    xmlparse.cpp)

add_compile_definitions(GENLIB_LIBRARY)

add_library(${genlib} STATIC ${genlib_SRCS})
