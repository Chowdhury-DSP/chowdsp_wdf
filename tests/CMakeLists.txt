add_executable(chowdsp_wdf_tests)
target_include_directories(chowdsp_wdf_tests PRIVATE .)
target_link_libraries(chowdsp_wdf_tests PRIVATE ${PROJECT_NAME} chowdsp_wdf)
target_sources(chowdsp_wdf_tests
    PRIVATE
        BasicCircuitTest.cpp
        StaticBasicCircuitTest.cpp
        OmegaTest.cpp
        RTypeTest.cpp
        SIMDTest.cpp
        TestRunner.cpp
)

add_custom_command(TARGET chowdsp_wdf_tests
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:chowdsp_wdf_tests> to test-binary"
        COMMAND ${CMAKE_COMMAND} -E make_directory test-binary
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:chowdsp_wdf_tests>" test-binary)

option(CHOWDSP_WDF_TEST_WITH_XSIMD "Tests chowdsp_wdf with XSIMD" ON)
if(CHOWDSP_WDF_TEST_WITH_XSIMD)
    message(STATUS "Importing XSIMD with CPM")
    CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG 8.0.5
    )

    target_link_libraries(chowdsp_wdf_tests PRIVATE ${PROJECT_NAME} xsimd)
    target_compile_definitions(chowdsp_wdf_tests PRIVATE CHOWDSP_WDF_TEST_WITH_XSIMD=1)
endif()

option(CHOWDSP_WDF_CODE_COVERAGE "Build tests with code coverage flags" OFF)
if(CHOWDSP_WDF_CODE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add required flags (GCC & LLVM/Clang)
        message(STATUS "chowdsp_wdf_tests -- Appending code coverage compiler flags: -O0 -g --coverage")
        target_compile_options(chowdsp_wdf_tests PUBLIC
            -O0        # no optimization
            -g         # generate debug info
            --coverage # sets all required flags
        )
        target_link_options(chowdsp_wdf_tests PUBLIC --coverage)
    endif()
endif()

option(CHOWDSP_WDF_RUN_CLANG_TIDY "Run clang-tidy on chowdsp WDF library" OFF)
if(CHOWDSP_WDF_RUN_CLANG_TIDY)
    message(STATUS "Configuring clang-tidy target")
    add_custom_target(chowdsp_wdf_clang_tidy
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND rm -f chowdsp_wdf_clang_tidy.cpp
        COMMAND ${CMAKE_COMMAND} -E echo "\\#include \\\"include/chowdsp_wdf/chowdsp_wdf.h\\\"" > chowdsp_wdf_clang_tidy.cpp
        COMMAND clang-tidy -p build "chowdsp_wdf_clang_tidy.cpp"
    )
endif()
