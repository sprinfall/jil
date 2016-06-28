if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(SRCS
    bitmap_button.cc
    bitmap_button.h
    bitmap_toggle_button.cc
    bitmap_toggle_button.h
    bold_item_combo_box.cc
    bold_item_combo_box.h
    button_base.cc
    button_base.h
    button_style.cc
    button_style.h
    color.cc
    color.h
    label.cc
    label.h
    static_box.cc
    static_box.h
    string_list_ctrl.cc
    string_list_ctrl.h
    text_button.cc
    text_button.h
    util.cc
    util.h
    )

add_library(ui ${SRCS})

COMMON_TARGET_SETTING(ui SRCS)
