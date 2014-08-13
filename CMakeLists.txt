cmake_minimum_required(VERSION 3.0)
project(jil)

option(JIL_ENABLE_TEST "Enable unit test?" off)

add_definitions(-DUNICODE -D_UNICODE)

# Require "qa" and "xml" for debug report.
if(MSVC)
    find_package(wxWidgets REQUIRED COMPONENTS core base qa xml)
else()
    # Require "net" for create server/client between two instances.
    find_package(wxWidgets REQUIRED COMPONENTS core base net qa xml)
endif()
include(${wxWidgets_USE_FILE})

set(Boost_USE_STATIC_LIBS ON)
# Boost.regex of version 1.49 has memory leak. The latest version 1.55 doesn't
# have this issue. Remember to test it every time boost is upgraded.
find_package(Boost 1.55.0 REQUIRED COMPONENTS regex)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

if(JIL_ENABLE_TEST)
    enable_testing()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(src)
