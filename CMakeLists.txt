cmake_minimum_required(VERSION 3.1)
project(chowdsp_wdf VERSION 1.0.0 LANGUAGES C CXX)
include(cmake/CXXStandard.cmake)

include(cmake/CPM.cmake)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

# Check if this is the top-level project
get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else ()
    set(is_toplevel 0)
endif ()

# if we are the top-level project, then configure the project tests
option(CHOWDSP_WDF_BUILD_TESTS "Add targets for building and running chowdsp_wdf tests" ${is_toplevel})

if (CHOWDSP_WDF_BUILD_TESTS)
    add_subdirectory(tests)
endif()
