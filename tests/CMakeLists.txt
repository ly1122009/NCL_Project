cmake_minimum_required(VERSION 3.15)
project(NCL_Tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/core/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL
    ${CMAKE_CURRENT_SOURCE_DIR}/../interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/test-support/catch2
)

# OSAL Source files
set(OSAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Thread.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Event.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Library.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Mutex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../feature/OSAL/OSAL_Semaphore.c
)

# Test files
set(TEST_FILES
    osal/test_main.cpp
    osal/test_osal_memory.cpp
    osal/test_osal_queue.cpp
    osal/test_osal_mutex.cpp
    osal/test_osal_semaphore.cpp
    osal/test_osal_event.cpp
    osal/test_osal_thread.cpp
)

# Create test executable
add_executable(osal_tests ${TEST_FILES} ${OSAL_SOURCES})

# Link libraries (if any)
# target_link_libraries(osal_tests PRIVATE...)

# Add pthread for thread tests
find_package(Threads REQUIRED)
target_link_libraries(osal_tests PRIVATE Threads::Threads)
