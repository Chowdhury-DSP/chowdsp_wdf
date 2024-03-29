cmake_minimum_required(VERSION 3.5)
project(chowdsp_wdf VERSION 1.0.0 LANGUAGES C CXX)
include(cmake/CXXStandard.cmake)

include(cmake/CPM.cmake)

message(STATUS "Configuring ${PROJECT_NAME} library...")
add_library(${PROJECT_NAME} INTERFACE)
add_library(chowdsp::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME}
        INTERFACE
            _USE_MATH_DEFINES=1 # So that we can have M_PI on MSVC
    )
endif()

# Check if this is the top-level project
get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else ()
    set(is_toplevel 0)
endif ()

# if we are the top-level project, then configure the project tests and benchmarks
option(CHOWDSP_WDF_BUILD_TESTS "Add targets for building and running chowdsp_wdf tests" ${is_toplevel})
option(CHOWDSP_WDF_BUILD_BENCHMARKS "Add targets for building and running chowdsp_wdf benchmarks" ${is_toplevel})

set(CHOWDSP_WDF_TEST_WITH_XSIMD_VERSION "" CACHE STRING "Tests chowdsp_wdf with XSIMD version")
if(NOT ("${CHOWDSP_WDF_TEST_WITH_XSIMD_VERSION}" STREQUAL ""))
    message(STATUS "Importing XSIMD version ${CHOWDSP_WDF_TEST_WITH_XSIMD_VERSION} with CPM")
    CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG ${CHOWDSP_WDF_TEST_WITH_XSIMD_VERSION}
    )
endif()

if (CHOWDSP_WDF_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (CHOWDSP_WDF_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()
