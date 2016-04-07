if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(SRCS
    action.cc
    action.h
    binding.cc
    binding.h
    compile_config.h
    defs.h
    file_io.cc
    file_io.h
    ft_plugin.cc
    ft_plugin.h
    key.cc
    key.h
    lex.cc
    lex.h
    line_nr_area.cc
    line_nr_area.h
    lua_proxy.cc
    lua_proxy.h
    option.cc
    option.h
    relite.cc
    relite.h
    renderer.cc
    renderer.h
    selection.h
    style.cc
    style.h
    text_area.cc
    text_area.h
    text_buffer.cc
    text_buffer.h
    text_extent.cc
    text_extent.h
    text_func.cc
    text_func.h
    text_line.cc
    text_line.h
    text_listener.h
    text_point.cc
    text_point.h
    text_range.cc
    text_range.h
    text_window.cc
    text_window.h
    tab.cc
    tab.h
    theme.h
    tip.cc
    tip.h
    util.cc
    util.h
    wrap.cc
    wrap.h
    )

add_library(editor ${SRCS})

COMMON_TARGET_SETTING(editor SRCS)

# Unit test
if(JIL_ENABLE_UT)
    set(UT_SRCS
        action_unittest.cc
		relite_unittest.cc
        text_line_unittest.cc
        text_buffer_unittest.cc
        text_range_unittest.cc
        )
    set(UT_TARGET_NAME editor_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor uchardet gtest lua)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()
