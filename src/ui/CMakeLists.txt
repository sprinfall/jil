if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(HEADERS
    bitmap_toggle_button.h
    color.h
    static_box.h
    util.h
    )

set(SRCS
    bitmap_toggle_button.cc
    color.cc
    static_box.cc
    util.cc
    )

add_library(ui ${SRCS} ${HEADERS})
