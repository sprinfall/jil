if(JIL_ENABLE_UT)
    file(GLOB UT_SRCS *.cc)

    set(UT_TARGET_NAME editor_unittest)
    
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor uchardet gtest lua)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()
