if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

set(INDENT_HEADERS
    indent.h
    indent_cfg.h
    indent_cpp.h
    )
set(INDENT_SRCS
    indent.cc
    indent_cfg.cc
    indent_cpp.cc
    )

set(HEADERS
    ${INDENT_HEADERS}
    binding.h
    buffer_listener.h
    compile_config.h
    defs.h
    action.h
    file_io.h
    ft_plugin.h
    key.h
    lex.h
    line_nr_area.h
    option.h
    relite.h
    renderer.h
    selection.h
    status_bar.h
    style.h
    text_area.h
    text_buffer.h
    text_extent.h
    text_func.h
    text_line.h
    text_point.cc
    text_point.h
    text_range.h
    text_window.h
    tab.h
    theme.h
    tip.h
    util.h
    wrap.h
    )

set(SRCS
    ${INDENT_SRCS}
    binding.cc
    action.cc
    file_io.cc
    ft_plugin.cc
    key.cc
    lex.cc
    line_nr_area.cc
    option.cc
    relite.cc
    renderer.cc
    status_bar.cc
    style.cc
    text_area.cc
    text_buffer.cc
    text_extent.cc
    text_func.cc
    text_line.cc
    text_range.cc
    text_window.cc
    tab.cc
    tip.cc
    util.cc
    wrap.cc
    )

add_library(editor ${SRCS} ${HEADERS})

# Unit test
if(JIL_ENABLE_UT)
    set(UT_SRCS
        action_unittest.cc
	relite_unittest.cc
        text_line_unittest.cc
        text_buffer_unittest.cc
        text_range_unittest.cc
        indent_cfg_unittest.cc
        indent_cpp_unittest.cc
        )
    set(UT_TARGET_NAME editor_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor uchardet gtest)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()
