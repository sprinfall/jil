if(UNIX)
    add_definitions(-D_GLIBCXX_USE_WCHAR_T -std=c++11)
endif()

# NOTE:
# You have to build libconfig and install to home directory (~/include and ~/lib).
# I don't add a cmake project for Mac because I don't know how to do it.
# 
if(APPLE)
    set(LIBCONFIG_INCLUDE_DIR $ENV{HOME}/include)
    set(LIBCONFIG_LIBRARY_DIR $ENV{HOME}/lib)

    include_directories(${LIBCONFIG_INCLUDE_DIR})
    link_directories(${LIBCONFIG_LIBRARY_DIR})
endif()

set(SRCS
    app.cc
    app.h
    binding_config.cc
    binding_config.h
    book_ctrl.cc
    book_ctrl.h
    book_frame.cc
    book_frame.h
    compile_config.h
    config.cc
    config.h
    defs.h
    find_result_page.cc
    find_result_page.h
    find_window.cc
    find_window.h
    font_util.cc
    font_util.h
    goto_dialog.cc
    goto_dialog.h
    id.h
    i18n_strings.h
    i18n_util.h
    lex_config.cc
    lex_config.h
    navigation_dialog.cc
    navigation_dialog.h
    option.cc
    option.h
    option_config.cc
    option_config.h
    pref/advanced_editor_config_dialog.cc
    pref/advanced_editor_config_dialog.h
    pref/defs.h
    pref/editor_config_panel.cc
    pref/editor_config_panel.h
    pref/editor_page.cc
    pref/editor_page.h
    pref/font_config_dialog.cc
    pref/font_config_dialog.h
    pref/general_config_panel.cc
    pref/general_config_panel.h
    pref/general_page.cc
    pref/general_page.h
    save.cc
    save.h
    separator.h
    session.cc
    session.h
    skin.cc
    skin.h
    splitter.cc
    splitter.h
    status_fields_config.cc
    status_fields_config.h
    text_book.cc
    text_book.h
    text_page.cc
    text_page.h
    theme_config.cc
    theme_config.h
    tool_book.cc
    tool_book.h
    util.cc
    util.h
    )

set(TARGET_NAME app)

set(TARGET_OUTPUT_NAME Jil)

# Resource files
if(WIN32)
    set(JIL_RC ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
elseif(APPLE)
    set(JIL_RC ${CMAKE_CURRENT_SOURCE_DIR}/icon/editor.icns)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icon/editor.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${JIL_RC} ${SRCS})

set(EXTRA_LIBS
    base
    ui
    editor
    pugixml
    uchardet
    )

if(APPLE)
    # The static lib of libconfig on Mac is named as "libconfig.a".
    # the prefix "lib" doesn't have to be specified in the link option.
    set(EXTRA_LIBS ${EXTRA_LIBS} config)
else()
    set(EXTRA_LIBS ${EXTRA_LIBS} libconfig)
endif()

# NOTE: Put ${wxWidgets_LIBRARIES} after others, or there will be link errors in Linux/GTK+.
target_link_libraries(${TARGET_NAME} ${EXTRA_LIBS} ${wxWidgets_LIBRARIES})

set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_NAME ${TARGET_OUTPUT_NAME})
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "/wd4290")
endif()

# Installation

if(WIN32)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/file_types.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_fields.cfg DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/ftplugin DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/app/icon DESTINATION .)

    if(JIL_ENABLE_VLD)
        install(FILES ${PROJECT_SOURCE_DIR}/third_party/win32/bin/dbghelp.dll DESTINATION .)
        install(FILES ${PROJECT_SOURCE_DIR}/third_party/win32/bin/Microsoft.DTfW.DHL.manifest DESTINATION .)
        install(FILES ${PROJECT_SOURCE_DIR}/third_party/win32/bin/vld_x86.dll DESTINATION .)
    endif()
elseif(APPLE)
    set(MAC_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${TARGET_OUTPUT_NAME}.app/Contents/MacOS)
    set(MAC_INSTALL_RES_DIR ${CMAKE_INSTALL_PREFIX}/${TARGET_OUTPUT_NAME}.app/Contents/Resources)

    message(STATUS "Mac install bin dir: " ${MAC_INSTALL_BIN_DIR})
    message(STATUS "Mac install res dir: " ${MAC_INSTALL_RES_DIR})

    # TODO: Is there any variable for the target output dir?
    #install(FILES ${CMAKE_INSTALL_PREFIX}/liblibconfig.dylib DESTINATION ${MAC_INSTALL_BIN_DIR})

    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/file_types.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_fields.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION ${MAC_INSTALL_RES_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/ftplugin DESTINATION ${MAC_INSTALL_RES_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/app/icon DESTINATION ${MAC_INSTALL_RES_DIR})
endif()
