# NOTE:
# You have to build libconfig and install to home directory (~/include and ~/lib).
# I don't add a cmake project for Mac because I don't know how to do it.
if(APPLE)
    set(LIBCONFIG_INCLUDE_DIR $ENV{HOME}/include)
    set(LIBCONFIG_LIBRARY_DIR $ENV{HOME}/lib)

    include_directories(${LIBCONFIG_INCLUDE_DIR})
    link_directories(${LIBCONFIG_LIBRARY_DIR})
endif()

file(GLOB_RECURSE SRCS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

GROUP_SOURCES_BY_DIR(SRCS)

set(TARGET_NAME app)
set(TARGET_OUTPUT_NAME jil)

# Resource files
if(WIN32)
    set(JIL_RC ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
elseif(APPLE)
    set(JIL_RC
        ${CMAKE_CURRENT_SOURCE_DIR}/icon/editor.icns
        )
    # Add icons to bundle.
    set_source_files_properties(${JIL_RC} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# Include resource files in the target.
set(SRCS ${SRCS} ${JIL_RC})

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${SRCS})

set(EXTRA_LIBS
    base
    ui
    editor
    pugixml
    uchardet
    lua
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

if(APPLE)
    set_target_properties(${TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in)

    set(MACOSX_BUNDLE_ICON_FILE editor.icns)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${TARGET_OUTPUT_NAME})

    # Version displayed on App Store and Finder, at most three parts: x.y.z
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0)

    # Internal project version, could be any format.
    set(MACOSX_BUNDLE_BUNDLE_VERSION 1.1.0)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_NAME ${TARGET_OUTPUT_NAME})

if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "/wd4290")
endif()

# Installation

message(STATUS "[debug] Project binary dir: " ${PROJECT_BINARY_DIR})
message(STATUS "[debug] Project source dir: " ${PROJECT_SOURCE_DIR})
message(STATUS "[debug] CMake install prefix: " ${CMAKE_INSTALL_PREFIX})

if(WIN32)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/ft.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_fields.cfg DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/ftplugin DESTINATION .)

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

    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/ft.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_fields.cfg DESTINATION ${MAC_INSTALL_RES_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION ${MAC_INSTALL_RES_DIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/ftplugin DESTINATION ${MAC_INSTALL_RES_DIR})
elseif(UNIX)
    # Install executable to ${CMAKE_INSTALL_PREFIX}.
    install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION .)

    # Install resource files to the same directory.
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/ft.cfg DESTINATION .)
    install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_fields.cfg DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION .)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/ftplugin DESTINATION .)
endif()
