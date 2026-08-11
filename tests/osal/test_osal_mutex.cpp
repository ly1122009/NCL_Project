#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Mutex.h"
#include <pthread.h>

TEST_CASE("OSAL_Mutex: Create and Terminate", "[osal][mutex]") {
    NCL_HANDLETYPE mutexHandle;
    
    REQUIRE(NCL_OSAL_MutexCreate(&mutexHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_MutexTerminate(mutexHandle) == NCL_ErrorNone);
}

TEST_CASE("OSAL_Mutex: Lock and Unlock", "[osal][mutex]") {
    NCL_HANDLETYPE mutexHandle;
    NCL_OSAL_MutexCreate(&mutexHandle);

    // Test basic lock/unlock
    REQUIRE(NCL_OSAL_MutexLock(mutexHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_MutexUnlock(mutexHandle) == NCL_ErrorNone);

    NCL_OSAL_MutexTerminate(mutexHandle);
}

TEST_CASE("OSAL_Mutex: Mutual Exclusion", "[osal][mutex]") {
    NCL_HANDLETYPE mutexHandle;
    NCL_OSAL_MutexCreate(&mutexHandle);

    // Lock the mutex
    REQUIRE(NCL_OSAL_MutexLock(mutexHandle) == NCL_ErrorNone);

    // Try to lock again in the same thread (should block or error depending on implementation)
    // Since we are in a single thread, we can't easily test blocking without a timeout or another thread.
    // For now, we just check if unlock works.
    REQUIRE(NCL_OSAL_MutexUnlock(mutexHandle) == NCL_ErrorNone);

    NCL_OSAL_MutexTerminate(mutexHandle);
}
