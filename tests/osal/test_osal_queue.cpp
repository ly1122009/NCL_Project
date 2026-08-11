#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Queue.h"
#include <string.h>

TEST_CASE("OSAL_Queue: Create and Terminate", "[osal][queue]") {
    NCL_HANDLETYPE queueHandle;
    int maxElem = 10;
    
    REQUIRE(NCL_OSAL_QueueCreate(&queueHandle, maxElem) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_QueueTerminate(queueHandle) == NCL_ErrorNone);
}

TEST_CASE("OSAL_Queue: Enqueue and Dequeue", "[osal][queue]") {
    NCL_HANDLETYPE queueHandle;
    NCL_OSAL_QueueCreate(&queueHandle, 5);

    int val1 = 100;
    int val2 = 200;
    int outVal;

    // Test Enqueue
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val1) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val2) == NCL_ErrorNone);

    // Test Get numElem
    NCL_U32 count;
    NCL_OSAL_Queue_Get_numELem(queueHandle, &count);
    REQUIRE(count == 2);

    // Test Dequeue
    REQUIRE(NCL_OSAL_Dequeue(queueHandle, (NCL_PTR*)&outVal) == NCL_ErrorNone);
    REQUIRE(outVal == 100);

    REQUIRE(NCL_OSAL_Dequeue(queueHandle, (NCL_PTR*)&outVal) == NCL_ErrorNone);
    REQUIRE(outVal == 200);

    // Test Queue Empty
    REQUIRE(NCL_OSAL_Dequeue(queueHandle, (NCL_PTR*)&outVal)!= NCL_ErrorNone);

    NCL_OSAL_QueueTerminate(queueHandle);
}

TEST_CASE("OSAL_Queue: Full Queue", "[osal][queue]") {
    NCL_HANDLETYPE queueHandle;
    int maxElem = 3;
    NCL_OSAL_QueueCreate(&queueHandle, maxElem);

    int val = 42;
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val) == NCL_ErrorNone);
    
    // Queue is now full
    REQUIRE(NCL_OSAL_Enqueue(queueHandle, &val)!= NCL_ErrorNone);

    NCL_OSAL_QueueTerminate(queueHandle);
}

TEST_CASE("OSAL_Queue: Reset", "[osal][queue]") {
    NCL_HANDLETYPE queueHandle;
    NCL_OSAL_QueueCreate(&queueHandle, 5);

    int val = 1;
    NCL_OSAL_Enqueue(queueHandle, &val);
    NCL_OSAL_Enqueue(queueHandle, &val);

    REQUIRE(NCL_OSAL_QueueReset(queueHandle) == NCL_ErrorNone);

    NCL_U32 count;
    NCL_OSAL_Queue_Get_numELem(queueHandle, &count);
    REQUIRE(count == 0);

    NCL_OSAL_QueueTerminate(queueHandle);
}
