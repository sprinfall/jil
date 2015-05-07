if(JIL_ENABLE_UT)
    add_subdirectory(gtest)
endif()

if(NOT APPLE)
    add_subdirectory(libconfig)
endif()

add_subdirectory(pugixml)
add_subdirectory(uchardet)
add_subdirectory(base)
add_subdirectory(editor)
add_subdirectory(app)
add_subdirectory(ui)
