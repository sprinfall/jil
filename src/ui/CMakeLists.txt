set(HEADERS
    bitmap_toggle_button.h
    color.h
    )

set(SRCS
    bitmap_toggle_button.cc
    color.cc
    )

add_library(ui ${SRCS} ${HEADERS})
