# Unit test
if(JIL_ENABLE_UT)
    set(UT_SRCS
        indent_c_unittest.cc
        indent_cfg_unittest.cc
        indent_cpp_unittest.cc
        )
    set(UT_TARGET_NAME indent_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor uchardet gtest lua)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()
