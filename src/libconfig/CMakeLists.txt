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

add_definitions(-DLIBCONFIG_EXPORTS -DYY_NO_UNISTD_H -DYY_USE_CONST)

add_library(libconfig SHARED ${SRCS})

if(MSVC)
    set_target_properties(libconfig PROPERTIES COMPILE_FLAGS "/wd4290")
endif()

install(TARGETS libconfig DESTINATION .)

