if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(HEADERS
    bitmap_toggle_button.h
    color.h
    label_ctrl.h
    static_box.h
    )

set(SRCS
    bitmap_toggle_button.cc
    color.cc
    label_ctrl.cc
    static_box.cc
    )

add_library(ui ${SRCS} ${HEADERS})
