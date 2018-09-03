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

add_library(libconfig ${SRCS})

if(MSVC)
    set_target_properties(libconfig PROPERTIES COMPILE_FLAGS "/wd4290")
endif()
