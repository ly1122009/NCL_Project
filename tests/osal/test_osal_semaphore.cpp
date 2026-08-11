#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Semaphore.h"

TEST_CASE("OSAL_Semaphore: Create and Terminate", "[osal][semaphore]") {
    NCL_HANDLETYPE semaphoreHandle;
    
    REQUIRE(NCL_OSAL_SemaphoreCreate(&semaphoreHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_SemaphoreTerminate(semaphoreHandle) == NCL_ErrorNone);
}

TEST_CASE("OSAL_Semaphore: Post and Wait", "[osal][semaphore]") {
    NCL_HANDLETYPE semaphoreHandle;
    NCL_OSAL_SemaphoreCreate(&semaphoreHandle);

    // Test Post and Wait
    REQUIRE(NCL_OSAL_SemaphorePost(semaphoreHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_SemaphoreWait(semaphoreHandle) == NCL_ErrorNone);

    NCL_OSAL_SemaphoreTerminate(semaphoreHandle);
}

TEST_CASE("OSAL_Semaphore: TryWait", "[osal][semaphore]") {
    NCL_HANDLETYPE semaphoreHandle;
    NCL_OSAL_SemaphoreCreate(&semaphoreHandle);

    // Semaphore starts at 0 (assuming default behavior)
    // TryWait should fail if count is 0
    REQUIRE(NCL_OSAL_SemaphoreTryWait(semaphoreHandle)!= NCL_ErrorNone);

    // Post and then TryWait should succeed
    REQUIRE(NCL_OSAL_SemaphorePost(semaphoreHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_SemaphoreTryWait(semaphoreHandle) == NCL_ErrorNone);

    NCL_OSAL_SemaphoreTerminate(semaphoreHandle);
}

TEST_CASE("OSAL_Semaphore: Count and Set", "[osal][semaphore]") {
    NCL_HANDLETYPE semaphoreHandle;
    NCL_OSAL_SemaphoreCreate(&semaphoreHandle);

    NCL_S32 count;
    
    // Initial count (assuming 0)
    NCL_OSAL_Get_SemaphoreCount(semaphoreHandle, &count);
    // Note: We don't assert count == 0 because implementation might differ, 
    // but we will test the Set/Get cycle.

    // Set count to 5
    REQUIRE(NCL_OSAL_Set_SemaphoreCount(semaphoreHandle, 5) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_Get_SemaphoreCount(semaphoreHandle, &count) == NCL_ErrorNone);
    REQUIRE(count == 5);

    // Post and Wait
    REQUIRE(NCL_OSAL_SemaphorePost(semaphoreHandle) == NCL_ErrorNone); // count = 6
    REQUIRE(NCL_OSAL_SemaphoreWait(semaphoreHandle) == NCL_ErrorNone); // count = 5
    REQUIRE(NCL_OSAL_Get_SemaphoreCount(semaphoreHandle, &count) == NCL_ErrorNone);
    REQUIRE(count == 5);

    NCL_OSAL_SemaphoreTerminate(semaphoreHandle);
}
