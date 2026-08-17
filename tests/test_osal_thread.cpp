extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Thread.h"
}

#include <gtest/gtest.h>

static void *ThreadBody(void *arg) {
  (void)arg;
  return nullptr;
}

TEST(OSAL_Thread, CreateAndTerminateJoinable) {
  NCL_HANDLETYPE thread = nullptr;
  ASSERT_EQ(NCL_OSAL_ThreadCreate(&thread, reinterpret_cast<NCL_PTR>(&ThreadBody),
                                   nullptr, NCL_THREAD_JOINABLE),
            NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_ThreadTerminate(thread), NCL_ErrorNone);
}
