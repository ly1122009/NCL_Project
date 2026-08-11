#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Event.h"

TEST_CASE("OSAL_Event: Create and Terminate", "[osal][event]") {
    NCL_HANDLETYPE eventHandle;
    
    REQUIRE(NCL_OSAL_SignalCreate(&eventHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_SignalTerminate(eventHandle) == NCL_ErrorNone);
}

TEST_CASE("OSAL_Event: Set and Wait", "[osal][event]") {
    NCL_HANDLETYPE eventHandle;
    NCL_OSAL_SignalCreate(&eventHandle);

    // Test Set and Wait
    REQUIRE(NCL_OSAL_SignalSet(eventHandle) == NCL_ErrorNone);
    REQUIRE(NCL_OSAL_SignalWait_ms(eventHandle, 10) == NCL_ErrorNone);

    NCL_OSAL_SignalTerminate(eventHandle);
}

TEST_CASE("OSAL_Event: Reset and Timeout", "[osal][event]") {
    NCL_HANDLETYPE eventHandle;
    NCL_OSAL_SignalCreate(&eventHandle);

    // Set the event
    REQUIRE(NCL_OSAL_SignalSet(eventHandle) == NCL_ErrorNone);
    
    // Reset the event
    REQUIRE(NCL_OSAL_SignalReset(eventHandle) == NCL_ErrorNone);

    // Wait should timeout
    REQUIRE(NCL_OSAL_SignalWait_ms(eventHandle, 10) == NCL_ErrorTimeout);

    NCL_OSAL_SignalTerminate(eventHandle);
}
