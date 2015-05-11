set(SRCS
    grammar.c
    grammar.h
    libconfig.c
    libconfig.h
    parsectx.h
    scanctx.c
    scanctx.h
    scanner.c
    scanner.h
    strbuf.c
    strbuf.h
    wincompat.h
    )

# Should be added at the top level CMakeList.txt because project "app" also
# needs these definitions.
#add_definitions(-DLIBCONFIG_STATIC -DYY_NO_UNISTD_H -DYY_USE_CONST)

add_library(libconfig ${SRCS})

if(MSVC)
    set_target_properties(libconfig PROPERTIES COMPILE_FLAGS "/wd4290")
endif()
