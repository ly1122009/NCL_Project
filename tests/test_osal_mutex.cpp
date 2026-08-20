extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Mutex.h"
}

#include <gtest/gtest.h>

TEST(OSAL_Mutex, CreateLockUnlockTerminate) {
  NCL_HANDLETYPE mutex = nullptr;
  ASSERT_EQ(NCL_OSAL_MutexCreate(&mutex), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_MutexLock(mutex), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_MutexUnlock(mutex), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_MutexTerminate(mutex), NCL_ErrorNone);
}

// A NULL out-parameter is exactly the case the leading guard clause in
// NCL_OSAL_MutexCreate exists to reject. It should come back as
// NCL_ErrorBadParameter, not crash the process.
TEST(OSAL_Mutex, CreateRejectsNullOutParam) {
  EXPECT_EQ(NCL_OSAL_MutexCreate(nullptr), NCL_ErrorBadParameter);
}
