# Unit test
if(JIL_ENABLE_UT)
    set(UT_SRCS
        indent_test_base.cc
        indent_test_base.h
        indent_test_c.cc
        indent_test_cfg.cc
        indent_test_cpp.cc
        indent_test_go.cc
        )
    set(UT_TARGET_NAME indent_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor uchardet gtest lua)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()
