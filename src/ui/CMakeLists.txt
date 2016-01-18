if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(HEADERS
    bitmap_button.h
    bitmap_toggle_button.h
    button_base.h
    button_style.h
    color.h
    combo_box.h
    label.h
    static_box.h
    text_button.h
    util.h
    )

set(SRCS
    bitmap_button.cc
    bitmap_toggle_button.cc
    button_base.cc
    button_style.cc
    color.cc
    combo_box.cc
    label.cc
    static_box.cc
    text_button.cc
    util.cc
    )

add_library(ui ${SRCS} ${HEADERS})
