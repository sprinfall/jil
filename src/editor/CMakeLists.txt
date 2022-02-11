file(GLOB SRCS *.cc *.h)

add_library(editor ${SRCS})
target_link_libraries(editor base)
