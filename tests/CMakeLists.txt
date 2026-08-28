include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ncl_osal_tests
    test_osal_dmabuf.cpp
    test_osal_queue.cpp
    test_osal_event.cpp
    test_osal_mutex.cpp
    test_osal_thread.cpp
)

target_link_libraries(ncl_osal_tests PRIVATE ncl_osal GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ncl_osal_tests
    PROPERTIES TIMEOUT 10
    DISCOVERY_MODE PRE_TEST
)
