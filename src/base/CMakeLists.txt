set(SRCS
    math_util.h
    string_util.cc
    string_util.h
    )

add_library(base ${SRCS})

COMMON_TARGET_SETTING(base SRCS)
