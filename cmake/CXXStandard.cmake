if("${CMAKE_CXX_STANDARD}" STREQUAL "")
    message(STATUS "A C++ Standard has not been set for this project! chowdsp_wdf is selecting C++14...")
    set(CMAKE_CXX_STANDARD 14)
else()
    if(${CMAKE_CXX_STANDARD} LESS 14)
        message(FATAL_ERROR "chowdsp_wdf requires C++ 14 or later")
    endif()
endif()
