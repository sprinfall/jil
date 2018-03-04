set(SRCS
    math_util.h
    math_util.cc
    string_util.h
    string_util.cc
    )

add_library(base ${SRCS})

COMMON_TARGET_SETTING(base SRCS)
