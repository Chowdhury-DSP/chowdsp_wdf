message(STATUS "Importing Googe Benchmark with CPM")
CPMAddPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    VERSION 1.5.2
    OPTIONS "BENCHMARK_ENABLE_TESTING Off"
)

# setup_benchmark(<target-name> <file-name>)
#
# Sets up a minimal benchmarking app
function(setup_benchmark target file)
    message(STATUS "Configuring Benchmark ${target}")

    add_executable(${target})
    target_sources(${target} PRIVATE ${file})

    target_link_libraries(${target} PRIVATE
        chowdsp_wdf
        benchmark::benchmark
    )

    if(NOT (${CHOWDSP_WDF_TEST_WITH_XSIMD_VERSION} STREQUAL ""))
        target_link_libraries(${target} PRIVATE ${PROJECT_NAME} xsimd)
        target_compile_definitions(${target} PRIVATE CHOWDSP_WDF_TEST_WITH_XSIMD=1)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Any Clang
            if(CMAKE_CXX_COMPILER_ID MATCHES "^AppleClang$")
                # Apple Clang only
            endif()
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "^GNU$")
            # GCC only
            target_compile_options(${target} PUBLIC
                -Wno-pessimizing-move
                -Wno-redundant-decls
            )
        endif()
    endif()

    add_custom_command(TARGET ${target}
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:${target}> to bench-binary"
        COMMAND ${CMAKE_COMMAND} -E make_directory bench-binary
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target}>" bench-binary
    )
endfunction(setup_benchmark)

setup_benchmark(wright_omega_bench WrightOmegaBench.cpp)
