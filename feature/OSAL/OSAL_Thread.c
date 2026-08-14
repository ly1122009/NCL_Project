#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "OSAL_Thread.h"

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "OSAL_THREAD"

typedef struct _NCL_THREAD_HANDLE_TYPE {
  pthread_t pthread;
  pthread_attr_t attr;
  struct sched_param schedparam;
  int stack_size;
  NCL_THREAD_TYPE detachState;
} NCL_THREAD_HANDLE_TYPE;

NCL_ERRORTYPE NCL_OSAL_ThreadCreate(NCL_HANDLETYPE *thread_handle,
                                    NCL_PTR func_name, NCL_PTR param,
                                    NCL_THREAD_TYPE thread_type) {
  int thread_ret = 0;
  int detach_ret = 0;
  int init_ret = 0;
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_THREAD_HANDLE_TYPE *thread = NULL;
  int detachState = PTHREAD_CREATE_DETACHED;
  if (!thread_handle || !func_name) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  switch (thread_type) {
  case NCL_THREAD_DETACHED:
    detachState = PTHREAD_CREATE_DETACHED;
    break;
  case NCL_THREAD_JOINABLE:
    detachState = PTHREAD_CREATE_JOINABLE;
    break;
  default:
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  thread =
      (NCL_THREAD_HANDLE_TYPE *)NCL_OSAL_Malloc(sizeof(NCL_THREAD_HANDLE_TYPE));
  if (!thread) {
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }
  NCL_OSAL_Memset(thread, 0, sizeof(NCL_THREAD_HANDLE_TYPE));

  // pthread_attr_init(&thread->attr);
  init_ret = pthread_attr_init(&thread->attr);
  if (init_ret != 0) {
    LOGE(NCL_LOG_TAG2,
         "ERROR: [NCL_OSAL_ThreadCreate] - pthread_attr_init failed! %d",
         init_ret);
    ret = NCL_ErrorUndefined;
    goto FREE_THREAD;
  }

  // TODO: Stack size and priority
  if (thread->stack_size != 0)
    pthread_attr_setstacksize(&thread->attr, thread->stack_size);

  if (thread->schedparam.sched_priority != 0)
    pthread_attr_setschedparam(&thread->attr, &thread->schedparam);

  thread->detachState = detachState;
  detach_ret = pthread_attr_setdetachstate(&thread->attr, thread->detachState);
  if (detach_ret != 0) {
    LOGE(NCL_LOG_TAG2,
         "ERROR: [NCL_OSAL_ThreadCreate] - pthread_attr_setdetachstate "
         "failed! %d",
         detach_ret);
    ret = NCL_ErrorUndefined;
    goto DESTROY_ATTR;
  }

  // Convert NCL_PTR -> Function pointer
  void *(*start_routine)(void *) = (void *(*)(void *))func_name;
  thread_ret = pthread_create(&thread->pthread, &thread->attr, start_routine,
                              (void *)param);
  pthread_attr_destroy(&thread->attr);
  switch (thread_ret) {
  case 0:
    *thread_handle = (NCL_HANDLETYPE)thread;
    LOGI(NCL_LOG_TAG2, "[NCL_OSAL_ThreadCreate] - thread id %lu is created",
         thread->pthread);
    ret = NCL_ErrorNone;
    break;
  case EAGAIN:
    LOGE(NCL_LOG_TAG2,
         "ERROR: [NCL_OSAL_ThreadCreate] -  pthread_create failed EAGAIN %d",
         thread_ret);
    ret = NCL_ErrorUndefined;
    goto FREE_THREAD;
  default:
    LOGE(NCL_LOG_TAG2,
         "ERROR: [NCL_OSAL_ThreadCreate] -  pthread_create failed %d",
         thread_ret);
    ret = NCL_ErrorUndefined;
    goto FREE_THREAD;
  }

  return ret;

DESTROY_ATTR:
  pthread_attr_destroy(&thread->attr);
FREE_THREAD:
  NCL_OSAL_Free(thread);
EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_ThreadCreate] out - ret: %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_ThreadTerminate(NCL_HANDLETYPE *thread_handle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  int join_ret = 0;
  NCL_THREAD_HANDLE_TYPE *thread = (NCL_THREAD_HANDLE_TYPE *)thread_handle;
  if (!thread) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (thread->detachState == PTHREAD_CREATE_JOINABLE) {
    join_ret = pthread_join(thread->pthread, NULL);
    if (0 != join_ret) {
      LOGE(NCL_LOG_TAG2,
           "ERROR: [NCL_OSAL_ThreadTerminate] - pthread_join failed %d",
           join_ret);
      ret = NCL_ErrorBadParameter;
      goto EXIT;
    }
  }
  NCL_OSAL_Free(thread);
  LOGI(NCL_LOG_TAG2,
       "[NCL_OSAL_ThreadTerminate] thread id %lu join successfully %d",
       thread->pthread, join_ret);
  ret = NCL_ErrorNone;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_ThreadTerminate] out - ret: %d", ret);
  return ret;
}

void NCL_OSAL_ThreadExit(void *value_ptr) {
  pthread_exit(value_ptr);
  return;
}

void NCL_OSAL_SleepMillisec(NCL_S32 ms) {
  usleep(ms * 1000);
  return;
}
