cmake_minimum_required(VERSION 3.0)
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
    find_package(wxWidgets REQUIRED COMPONENTS core base qa xml)
else()
    # Require "net" for create server/client between two instances.
    find_package(wxWidgets REQUIRED COMPONENTS core base net qa xml)
endif()

include(${wxWidgets_USE_FILE})

set(Boost_USE_STATIC_LIBS ON)
# Boost.regex of version 1.49 has memory leak. The latest version 1.55 doesn't
# have this issue. Remember to test it every time boost is upgraded.
find_package(Boost REQUIRED COMPONENTS regex)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

if(JIL_ENABLE_UT)
    enable_testing()
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)

if(WIN32)
    include_directories(${PROJECT_SOURCE_DIR}/third_party/win32/include)
    link_directories(${PROJECT_SOURCE_DIR}/third_party/win32/lib)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(src)
