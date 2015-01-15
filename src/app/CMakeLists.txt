set(SRCS
    app.cc
    app.h
    bitmap_toggle_button.cc
    bitmap_toggle_button.h
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
    save.cc
    save.h
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

# Resource files
if(WIN32)
    set(JIL_RC ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
elseif(APPLE)
    set(JIL_RC jil.icns)
    set_source_files_properties(jil.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${JIL_RC} ${SRCS})

set(EXTRA_LIBS
    base
    editor
    libconfig
    pugixml
    uchardet
    )

if(WIN32)
    if(JIL_ENABLE_VLD)
        set(EXTRA_LIBS ${EXTRA_LIBS} vld)
    endif()
endif()

# NOTE: Put ${wxWidgets_LIBRARIES} after others, or there will be link errors in Linux/GTK+.
target_link_libraries(${TARGET_NAME} ${EXTRA_LIBS} ${wxWidgets_LIBRARIES})

set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_NAME jil)
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "/wd4290")
endif()

# Install
install(FILES ${CMAKE_SOURCE_DIR}/data/options.cfg DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/data/binding.cfg DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/data/file_types.cfg DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/data/status_fields.cfg DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/theme DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/ftplugin DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/app/icon DESTINATION .)

