#include <asm-generic/errno.h>
// #include <bits/pthread_types.h>  // For termuxer
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Event.h"
#include "OSAL_Memory.h"
#include "OSAL_Mutex.h"
#include "OSAL_Log.h"

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "OSAL_EVENT"

NCL_ERRORTYPE NCL_OSAL_SignalCreate(NCL_HANDLETYPE *eventHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_OSAL_THREADEVENT *event;
  int mutex_ret = 0;
  int cond_ret = 0;

  if (!eventHandle) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  event = (NCL_OSAL_THREADEVENT *)NCL_OSAL_Malloc(sizeof(NCL_OSAL_THREADEVENT));
  if (!event) {
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }
  event->m_signal = NCL_FALSE;

  mutex_ret = NCL_OSAL_MutexCreate(&event->m_mutex);
  if (mutex_ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalCreate]: Mutex create failed - %d", mutex_ret);
    NCL_OSAL_Free(event);
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }

  cond_ret = pthread_cond_init(&event->m_condition, NULL);
  if (cond_ret) {
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalCreate]: Cond create failed - %d", cond_ret);
    NCL_OSAL_MutexTerminate(event->m_mutex);
    NCL_OSAL_Free(event);
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }

  *eventHandle = (NCL_HANDLETYPE)event;
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SignalCreate] : ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SignalTerminate(NCL_HANDLETYPE eventHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int condTerminate_ret = 0;
  int mutexTerminate_ret = 0;
  NCL_OSAL_THREADEVENT *event = (NCL_OSAL_THREADEVENT *)eventHandle;

  if (!event) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  condTerminate_ret = pthread_cond_destroy(&event->m_condition);
  if (condTerminate_ret != 0) {
    ret = NCL_ErrorUndefined;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalTerminate] - cond destroy failed %d",
           condTerminate_ret);
    NCL_OSAL_Free(event);
    goto EXIT;
  }

  mutexTerminate_ret = NCL_OSAL_MutexTerminate(event->m_mutex);
  if (mutexTerminate_ret != 0) {
    ret = NCL_ErrorUndefined;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalTerminate] - mutex destroy failed %d",
           mutexTerminate_ret);
    NCL_OSAL_Free(event);
    goto EXIT;
  }

  ret = NCL_ErrorNone;
  goto EXIT;
EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SignalTerminate]: ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SignalReset(NCL_HANDLETYPE eventHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_OSAL_THREADEVENT *event = (NCL_OSAL_THREADEVENT *)eventHandle;
  if (!event) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  ret = NCL_OSAL_MutexLock(event->m_mutex);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalReset] - mutex lock failed %d", ret);
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  event->m_signal = NCL_FALSE;
  NCL_OSAL_MutexUnlock(event->m_mutex);

  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SignalReset] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SignalSet(NCL_HANDLETYPE eventHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_OSAL_THREADEVENT *event = (NCL_OSAL_THREADEVENT *)eventHandle;
  if (!event) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  ret = NCL_OSAL_MutexLock(event->m_mutex);
  if (ret!= NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalSet] - mutex lock failed %d", ret);
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  event->m_signal = NCL_TRUE;
  pthread_cond_signal(&event->m_condition);

  NCL_OSAL_MutexUnlock(event->m_mutex);

  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SignalSet] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_SignalWait_ms(NCL_HANDLETYPE eventHandle, NCL_U32 ms) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_OSAL_THREADEVENT *event = (NCL_OSAL_THREADEVENT *)eventHandle;
  struct timespec timeout;
  struct timeval now;
  int timeWait_ret = 0;
  long tv_us = 0;

  if (!event) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  gettimeofday(&now, NULL);

  tv_us = now.tv_usec + ((long)ms * 1000);
  timeout.tv_sec = now.tv_sec + tv_us / 1000000;
  timeout.tv_nsec = (tv_us % 1000000) * 1000;

  ret = NCL_OSAL_MutexLock(event->m_mutex);
  if (ret!= NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_SignalWait_ms] - mutex lock failed %d", ret);
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (ms == 0) {
    if (!event->m_signal) {
      ret = NCL_ErrorTimeout;
      goto EXIT;
    }
  } else if (ms == MAX_WAIT_TIME) {
    while (!event->m_signal) {
      pthread_cond_wait(&event->m_condition, (pthread_mutex_t *)event->m_mutex);
    }
    ret = NCL_ErrorNone;
    goto EXIT;
  } else {
    while (!event->m_signal) {
      timeWait_ret = pthread_cond_timedwait(
          &event->m_condition, (pthread_mutex_t *)event->m_mutex, &timeout);
      if (!event->m_signal && timeWait_ret == ETIMEDOUT) {
        ret = NCL_ErrorTimeout;
        goto EXIT;
      }
    }
  }

  NCL_OSAL_MutexUnlock(event->m_mutex);
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_SignalWait_ms] - ret %d", ret);
  return ret;
}
