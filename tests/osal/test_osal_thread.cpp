#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Thread.h"
#include <chrono>
#include <thread>

// Hàm dummy để thread thực thi
void* dummy_thread_func(void* arg) {
    int* val = static_cast<int*>(arg);
    *val = 1;
    NCL_OSAL_ThreadExit(NULL);
    return NULL;
}

TEST_CASE("OSAL_Thread: Create and Terminate", "[osal][thread]") {
    NCL_HANDLETYPE threadHandle;
    int dummy = 0;
    
    REQUIRE(NCL_OSAL_ThreadCreate(&threadHandle, (NCL_PTR)dummy_thread_func, &dummy, NCL_THREAD_JOINABLE) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_ThreadTerminate(&threadHandle) == NCL_ErrorNone);
}

TEST_CASE("OSAL_Thread: Execution", "[osal][thread]") {
    NCL_HANDLETYPE threadHandle;
    int flag = 0;
    
    REQUIRE(NCL_OSAL_ThreadCreate(&threadHandle, (NCL_PTR)dummy_thread_func, &flag, NCL_THREAD_JOINABLE) == NCL_ErrorNone);
    
    // Chờ một chút để thread kịp chạy
    NCL_OSAL_SleepMillisec(50);
    
    REQUIRE(flag == 1);
    
    NCL_OSAL_ThreadTerminate(&threadHandle);
}

TEST_CASE("OSAL_Thread: Sleep", "[osal][thread]") {
    auto start = std::chrono::high_resolution_clock::now();
    
    NCL_OSAL_SleepMillisec(100);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Kiểm tra xem thời gian ngủ có xấp xỉ 100ms không (cho phép sai số nhỏ)
    REQUIRE(duration >= 100);
    REQUIRE(duration < 200);
}
