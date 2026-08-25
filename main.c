// // #include "Math.h"
// #include "NCL_Core.h"
// #include "NCL_Types.h"
// #include "OSAL_Log.h"
// #include "OSAL_Memory.h"
// #include "OSAL_Mutex.h"
// #include "OSAL_Semaphore.h"
// #include "OSAL_Thread.h"
// #include <stdio.h>

// int main() {
//   printf("HEllo\n");

//   NCL_OSAL_LogInit("MY_APP", 0);
//   NCL_OSAL_LogSetLevel(LOG_VERBOSE);

//   LOGI("cam_cap", "Device opened: %s", "/dev/video0");
//   LOGW("cam_proc", "Queue 90%% full (%d/%d)", 230, 256);
//   LOGD("log_wrkr", "Log thread started, queue=%d", 256);
//   LOGE("cam_cap", "VIDIOC_DQBUF timeout!");

//   NCL_OSAL_LogDeinit();

//   return 0;
// }
