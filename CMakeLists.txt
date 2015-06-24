cmake_minimum_required(VERSION 2.8)
project(jil)

option(JIL_ENABLE_UT "Enable unit test?" OFF)

if(WIN32)
    option(JIL_ENABLE_VLD "Enable VLD (Visual Leak Detector)?" OFF)

    if(JIL_ENABLE_VLD)
        add_definitions(-DJIL_ENABLE_VLD)
    endif()
endif()

add_definitions(-DUNICODE -D_UNICODE)

# Require "qa" and "xml" for debug report.
if(MSVC)
    # adv for testing wxPropertySheetDialog
    find_package(wxWidgets REQUIRED COMPONENTS core base qa xml adv)
else()
    # Require "net" for create server/client between two instances.
    find_package(wxWidgets REQUIRED COMPONENTS core base net qa xml)
endif()

include(${wxWidgets_USE_FILE})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(JIL_ENABLE_UT)
    enable_testing()
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include/lua)
if(WIN32)
    include_directories(${PROJECT_SOURCE_DIR}/third_party/win32/include)
    link_directories(${PROJECT_SOURCE_DIR}/third_party/win32/lib)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_definitions(-DLIBCONFIG_STATIC -DYY_NO_UNISTD_H -DYY_USE_CONST)

add_subdirectory(src)
