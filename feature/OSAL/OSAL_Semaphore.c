#include "OSAL_Semaphore.h"
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Memory.h"
#include "OSAL_Log.h"
#include <pthread.h>
#include <semaphore.h>
// #include <stdlib.h>

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "OSAL_SEM"

NCL_ERRORTYPE NCL_OSAL_SemaphoreCreate(NCL_HANDLETYPE *semaphoreHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int sem_ret = 0;

  if (!semaphoreHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  sem_t *semaphore;
  semaphore = (sem_t *)NCL_OSAL_Malloc(sizeof(sem_t));
  sem_ret = sem_init(semaphore, 0, 0);
  if (sem_ret != 0) {
    NCL_OSAL_Free(semaphore);
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  *semaphoreHandle = (NCL_HANDLETYPE)semaphore;

EXIT:
  LOGE(NCL_LOG_TAG2, "ERROR: [NCL_OSAL_SemaphoreCreate] - %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SemaphoreTerminate(NCL_HANDLETYPE semaphoreHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int destroy_ret = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;
  if (!semaphore) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  destroy_ret = sem_destroy(semaphore);
  if (destroy_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  NCL_OSAL_Free(semaphore);
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SemaphoreTerminate] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SemaphoreTryWait(NCL_HANDLETYPE semaphoreHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int trywait_ret = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;

  trywait_ret = sem_trywait(semaphore);
  if (trywait_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SemaphoreTryWait] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SemaphoreWait(NCL_HANDLETYPE semaphoreHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int wait_ret = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;

  wait_ret = sem_wait(semaphore);
  if (wait_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SemaphoreWait] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SemaphorePost(NCL_HANDLETYPE semaphoreHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int post_ret = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;

  post_ret = sem_post(semaphore);
  if (post_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SemaphorePost] - ret %d", ret);
  return ret;
}
NCL_ERRORTYPE NCL_OSAL_Get_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle,
                                          NCL_OUT NCL_S32 *value) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int getvalue_ret = 0;
  NCL_S32 semVal = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;

  getvalue_ret = sem_getvalue(semaphore, &semVal);
  if (getvalue_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  *value = semVal;
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_Get_SemaphoreCount] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_Set_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle,
                                          NCL_IN const NCL_S32 value) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int setvalue_ret = 0;
  sem_t *semaphore = (sem_t *)semaphoreHandle;

  setvalue_ret = sem_init(semaphore, 0, value);
  if (setvalue_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_Set_SemaphoreCount] - ret %d", ret);
  return ret;
}
