#include <stdio.h>
#include <sys/types.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Memory.h"
#include "OSAL_Mutex.h"
#include "OSAL_Queue.h"

NCL_ERRORTYPE NCL_OSAL_QueueCreate(NCL_HANDLETYPE *queueHandle,
                                   int maxNumElem) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = NULL;    // output
  NCL_QElem *currElem = NULL; // Traveling
  NCL_QElem *newElem = NULL;
  int qMutex_ret = -1;

  if (!queueHandle || maxNumElem <= 0) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  queue = (NCL_QUEUE *)NCL_OSAL_Malloc(sizeof(NCL_QUEUE));
  if (!queue) {
    ret = NCL_ErrorInsufficientResources;
    printf("[NCL_OSAL_QueueCreate] - cannot allocate queue\n");
    goto EXIT;
  }
  NCL_OSAL_Memset(queue, 0, sizeof(NCL_QUEUE));

  qMutex_ret = NCL_OSAL_MutexCreate(&queue->m_qMutex);
  if (qMutex_ret != NCL_ErrorNone) {
    printf("[NCL_OSAL_QueueCreate] - mutex create failed\n");
    NCL_OSAL_Free(queue);
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  for (int i = 0; i < maxNumElem; i++) {
    newElem = (NCL_QElem *)NCL_OSAL_Malloc(sizeof(NCL_QElem));
    if (!newElem) {
      printf("[NCL_OSAL_QueueCreate] - Cannot allocate node %d\n", i);
      ret = NCL_ErrorInsufficientResources;
      currElem = queue->m_first;
      while (currElem != NULL) {
        NCL_QElem *temp = currElem;
        currElem = currElem->m_qNext;
        NCL_OSAL_Free(temp);
      }
      NCL_OSAL_MutexTerminate(&queue->m_qMutex);
      NCL_OSAL_Free(queue);
      goto EXIT;
    }
    NCL_OSAL_Memset(newElem, 0, sizeof(NCL_QElem));
    if (queue->m_first == NULL) {
      queue->m_first = newElem;
    } else {
      currElem->m_qNext = newElem;
    }
    currElem = newElem;
  }

  currElem->m_qNext = queue->m_first;
  queue->m_last = queue->m_first;
  queue->maxNumElem = maxNumElem;
  queue->numElem = 0;

  *queueHandle = (NCL_HANDLETYPE)queue;
  goto EXIT;

EXIT:
  printf("[NCL_OSAL_QueueCreate] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_QueueTerminate(NCL_HANDLETYPE queueHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;
  NCL_QElem *currElem = NULL;
  if (!queue) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  currElem = queue->m_first;
  while (currElem) {
    NCL_QElem *temp = currElem;
    currElem = currElem->m_qNext;
    NCL_OSAL_Free(temp);
  }
  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
  NCL_OSAL_MutexTerminate(&queue->m_qMutex);
  NCL_OSAL_Free(queue);

  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  printf("[NCL_OSAL_QueueTerminate] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_Enqueue(NCL_HANDLETYPE queueHandle,
                               NCL_IN const NCL_PTR data) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;

  if (!queue) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  if (queue->m_last == NULL || queue->numElem >= queue->maxNumElem) {
    printf("[NCL_OSAL_Queue]: cannot enqueue anymore, queue is full!\n");
    ret = NCL_ErrorUndefined;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  queue->m_last->m_data = (NCL_PTR)data;
  queue->m_last = queue->m_last->m_qNext;
  queue->numElem++;

  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
EXIT:
  printf("[NCL_OSAL_Queue] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_Dequeue(NCL_HANDLETYPE queueHandle,
                               NCL_OUT NCL_PTR *data) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;

  if (!queue || !data) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  if (queue->m_first == NULL || queue->numElem == 0) {
    printf("[NCL_OSAL_Dequeue]: cannot dequeue anymore, queue is empty!\n");
    ret = NCL_ErrorUndefined;
    *data = NULL;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  *data = queue->m_first->m_data;
  queue->m_first->m_data = NULL;
  queue->m_first = queue->m_first->m_qNext;
  queue->numElem--;
  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
EXIT:
  printf("[NCL_OSAL_Dequeue] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_Queue_Set_numElem(NCL_HANDLETYPE queueHandle,
                                         NCL_IN const NCL_U32 data) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;
  if (!queue) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  queue->numElem = data;
  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  printf("[NCL_OSAL_Queue_Set_numElem] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_Queue_Get_numElem(NCL_HANDLETYPE queueHandle,
                                         NCL_OUT NCL_U32 *data) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;
  if (!queue) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  *data = queue->numElem;
  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  printf("[NCL_OSAL_Queue_Get_numElem] - ret %d\n", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_QueueReset(NCL_HANDLETYPE queueHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = (NCL_QUEUE *)queueHandle;

  if (!queue) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_MutexLock(&queue->m_qMutex);
  if (!queue->m_first) {
    ret = NCL_ErrorBadParameter;
    NCL_OSAL_MutexUnlock(&queue->m_qMutex);
    goto EXIT;
  }

  while (queue->m_first != NULL && queue->numElem > 0) {
    queue->m_first->m_data = NULL;
    queue->m_first = queue->m_first->m_qNext;
    queue->numElem--;
  }
  queue->m_last = queue->m_first;
  queue->numElem = 0;

  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
  ret = NCL_ErrorNone;
  goto EXIT;

EXIT:
  printf("[NCL_OSAL_QueueReset] - ret %d\n", ret);
  return ret;
}
