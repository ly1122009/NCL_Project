extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Event.h"
}

#include <gtest/gtest.h>

// Each test below performs one "leaky" NCL_OSAL_SignalWait() call, then
// immediately performs a second wait/set round trip on the same event. If
// the first call left the internal mutex locked, the second round trip
// deadlocks — the test hangs instead of failing an assertion. That hang
// (visible as a ctest TIMEOUT) *is* the bug report.

TEST(OSAL_Event, ZeroTimeoutThenLaterWaitDoesNotDeadlock) {
  NCL_HANDLETYPE event = nullptr;
  ASSERT_EQ(NCL_OSAL_SignalCreate(&event), NCL_ErrorNone);

  // Not signaled yet: ms == 0 should return immediately with a timeout.
  EXPECT_EQ(NCL_OSAL_SignalWait(event, 0), NCL_ErrorTimeout);

  ASSERT_EQ(NCL_OSAL_SignalSet(event), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_SignalWait(event, MAX_WAIT_TIME), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_SignalTerminate(event), NCL_ErrorNone);
}

TEST(OSAL_Event, FiniteTimeoutThenLaterWaitDoesNotDeadlock) {
  NCL_HANDLETYPE event = nullptr;
  ASSERT_EQ(NCL_OSAL_SignalCreate(&event), NCL_ErrorNone);

  // Never signaled: expect a real timeout after ~50ms.
  EXPECT_EQ(NCL_OSAL_SignalWait(event, 50), NCL_ErrorTimeout);

  ASSERT_EQ(NCL_OSAL_SignalSet(event), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_SignalWait(event, MAX_WAIT_TIME), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_SignalTerminate(event), NCL_ErrorNone);
}

// Mirrors the real usage pattern: a worker thread waits forever, does
// work, resets, then waits forever again for the next signal.
TEST(OSAL_Event, WaitForeverCanBeReusedAfterReset) {
  NCL_HANDLETYPE event = nullptr;
  ASSERT_EQ(NCL_OSAL_SignalCreate(&event), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_SignalSet(event), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_SignalWait(event, MAX_WAIT_TIME), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_SignalReset(event), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_SignalSet(event), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_SignalWait(event, MAX_WAIT_TIME), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_SignalTerminate(event), NCL_ErrorNone);
}
