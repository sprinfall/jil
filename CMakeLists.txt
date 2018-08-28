cmake_minimum_required(VERSION 3.1.0)

if(POLICY CMP0074)
    # find_package() uses <PackageName>_ROOT variables.
    # This policy was introduced in CMake version 3.12.
    cmake_policy(SET CMP0074 NEW)
endif()

project(jil)

option(JIL_ENABLE_UT "Enable unit test?" OFF)

if(WIN32)
    option(JIL_ENABLE_VLD "Enable VLD (Visual Leak Detector)?" OFF)
    if(JIL_ENABLE_VLD)
        add_definitions(-DJIL_ENABLE_VLD)
    endif()
endif()

add_definitions(-DUNICODE -D_UNICODE)

if(WIN32)
    # Disable warning on boost string algorithms.
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# C++ standard requirements.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Group sources by dir.
macro(GROUP_SOURCES_BY_DIR source_files)
    set(sgbd_cur_dir ${CMAKE_CURRENT_BINARY_DIR})
    foreach(sgbd_file ${${source_files}})
        #message("sgbd_fpath=${sgbd_fpath}")
        string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
        string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
        string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
        string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
        if(sgbd_nogroup)
            set(sgbd_group_name "\\")
        endif()
        #message("group name=${sgbd_group_name}")
        source_group(${sgbd_group_name} FILES ${sgbd_file})
    endforeach()
endmacro()

# Require "qa" and "xml" for debug report.
if(MSVC)
    find_package(wxWidgets REQUIRED COMPONENTS core base qa xml)
else()
    # Require "net" for create server/client between two instances.
    find_package(wxWidgets REQUIRED COMPONENTS core base net qa xml)
endif()

include(${wxWidgets_USE_FILE})

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
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

add_definitions(-DLIBCONFIG_STATIC -DYY_NO_UNISTD_H -DYY_USE_CONST)

add_subdirectory(src)
