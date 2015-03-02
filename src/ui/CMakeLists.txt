set(HEADERS
	bitmap_toggle_button.h
	color.h
    text_button.h
    )

set(SRCS
	bitmap_toggle_button.cc
	color.cc
    text_button.cc
    )

add_library(ui ${SRCS} ${HEADERS})
