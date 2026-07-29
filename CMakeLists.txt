cmake_minimum_required(VERSION 3.15)
project(NCL_Project LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Source files
set(SOURCES
    main.cpp
    feature/core/src/Math.cpp
    feature/OSAL/OSAL_Memory.c
    feature/OSAL/OSAL_Thread.c
)

# Executable
add_executable(ncl_app ${SOURCES})

# Include directories (Modern CMake approach)
target_include_directories(ncl_app PRIVATE
    feature/core/inc
    feature/OSAL
    interface
)

# Add compiler warnings for better development experience
if(MSVC)
    target_compile_options(ncl_app PRIVATE /W4)
else()
    target_compile_options(ncl_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
