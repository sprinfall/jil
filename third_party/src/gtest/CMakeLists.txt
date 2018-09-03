set(SRCS
    gtest-all.cc
    gtest_main.cc
    )

add_library(gtest ${SRCS})

install(TARGETS gtest DESTINATION .)
