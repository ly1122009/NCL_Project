#include <bits/pthread_types.h>
#include <pthread.h>
#include <stdio.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "OSAL_Mutex.h"

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "OSAL_MUTEX"

NCL_ERRORTYPE NCL_OSAL_MutexCreate(NCL_HANDLETYPE *mutexHandle) {
  int init_ret = 0;
  NCL_ERRORTYPE ret = NCL_ErrorNone;

  if (!mutexHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  pthread_mutex_t *mutex;

  mutex = (pthread_mutex_t *)NCL_OSAL_Malloc(sizeof(pthread_mutex_t));
  if (!mutex) {
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }

  init_ret = pthread_mutex_init(mutex, NULL);
  if (init_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  *mutexHandle = (NCL_HANDLETYPE)mutex;

  return ret;

EXIT:
  LOGE(NCL_LOG_TAG2, "ERROR: [NCL_OSAL_MutexCreate] - %d", ret);
  NCL_OSAL_Free(mutex);
  *mutexHandle = NULL;
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_MutexTerminate(NCL_HANDLETYPE mutexHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int destroy_ret = 0;

  if (!mutexHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  pthread_mutex_t *mutex = (pthread_mutex_t *)mutexHandle;
  destroy_ret = pthread_mutex_destroy(mutex);
  if (destroy_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
  NCL_OSAL_Free(mutex);

EXIT:
  LOGE(NCL_LOG_TAG2, "ERROR: [NCL_OSAL_MutexTerminate] - %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_MutexLock(NCL_HANDLETYPE mutexHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int lock_ret = 0;

  if (!mutexHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  pthread_mutex_t *mutex = (pthread_mutex_t *)mutexHandle;
  lock_ret = pthread_mutex_lock(mutex);
  if (lock_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  return ret;
EXIT:
  LOGE(NCL_LOG_TAG2, "ERROR: [NCL_OSAL_MutexLock] - %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_MutexUnlock(NCL_HANDLETYPE mutexHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int unlock_ret = 0;

  if (!mutexHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  pthread_mutex_t *mutex = (pthread_mutex_t *)mutexHandle;
  unlock_ret = pthread_mutex_unlock(mutex);
  if (unlock_ret != 0) {
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  return ret;
EXIT:
  LOGE(NCL_LOG_TAG2, "ERROR: [NCL_OSAL_MutexUnlock] - %d", ret);
  return ret;
}
