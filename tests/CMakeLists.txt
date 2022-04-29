
add_executable(chowdsp_wdf_tests)
target_include_directories(chowdsp_wdf_tests PRIVATE .)
target_sources(chowdsp_wdf_tests
    PRIVATE
        BasicCircuitTest.cpp
        StaticBasicCircuitTest.cpp
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
