cmake_minimum_required(VERSION 3.15)
project(NCL_Project LANGUAGES C)

# Set C standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Source files
set(SOURCES
    main.c
    feature/core/src/Math.c
    feature/OSAL/OSAL_Memory.c
    feature/OSAL/OSAL_Thread.c
    feature/OSAL/OSAL_Event.c
    feature/OSAL/OSAL_Library.c
    feature/OSAL/OSAL_Log.c
    feature/OSAL/OSAL_Mutex.c
    feature/OSAL/OSAL_Queue.c
    feature/OSAL/OSAL_Semaphore.c
)

# Executable
add_executable(ncl_app ${SOURCES})

# Add tests subdirectory
add_subdirectory(tests)

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
