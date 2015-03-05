set(HEADERS
    bitmap_toggle_button.h
    button_base.h
    button_style.h
    color.h
    text_button.h
    )

set(SRCS
    bitmap_toggle_button.cc
    button_base.cc
    button_style.cc
    color.cc
    text_button.cc
    )

add_library(ui ${SRCS} ${HEADERS})
