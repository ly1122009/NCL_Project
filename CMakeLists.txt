cmake_minimum_required(VERSION 3.15)
project(NCL_Project LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

set(NCL_OSAL_SOURCES
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

add_library(ncl_osal STATIC ${NCL_OSAL_SOURCES})

target_include_directories(ncl_osal PUBLIC
    feature/core/inc
    feature/OSAL
    interface
)

target_link_libraries(ncl_osal PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

if(MSVC)
    target_compile_options(ncl_osal PRIVATE /W4)
else()
    target_compile_options(ncl_osal PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Executable
add_executable(ncl_app main.c)
target_link_libraries(ncl_app PRIVATE ncl_osal)

# Tests
enable_testing()
add_subdirectory(tests)
