#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Memory.h"
#include "OSAL_Mutex.h"
#include "OSAL_Queue.h"

// typedef struct _NCL_QUEUE {
//   NCL_QElem *m_first;
//   NCL_QElem *m_last;
//   int numELem;
//   int maxNumElem;
//   NCL_HANDLETYPE m_qMutex;
// } NCL_QUEUE;

NCL_ERRORTYPE NCL_OSAL_QueueCreate(NCL_HANDLETYPE *queueHandle,
                                   int maxNumElem) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE *queue = NULL;    // output
  NCL_QElem *currElem = NULL; // Traveling
  NCL_QElem *newElem = NULL;
  int qMutex_ret = 0;

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
      printf("[NCL_OSAL_QueueCreate] - Cannot allocate new node for queue\n");
      ret = NCL_ErrorInsufficientResources;
      currElem = queue->m_first;
      while (currElem != NULL) {
        NCL_QElem *temp = currElem;
        currElem = currElem->m_qNext;
        NCL_OSAL_Free(temp);
      }
      NCL_OSAL_MutexTerminate(queue->m_qMutex);
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

  queue->m_last = queue->m_first;
  queue->maxNumElem = maxNumElem;
  queue->numELem = 0;

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

NCL_ERRORTYPE NCL_OSAL_Queue(NCL_HANDLETYPE queueHandle,
                             NCL_IN const NCL_PTR data)
{
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_QUEUE* queue = (NCL_QUEUE*)queueHandle;
  NCL_QElem* newElem = NULL;
  if (!queue)
  {
    ret = NCL_ErrorBadParameter;
    goto EXIT;    
  }
  if (queue->m_last == queue->m_first && queue->numELem > queue->maxNumElem)
  {
    printf("[NCL_OSAL_Queue]: cannot queue anymore, full of queue!");
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }
    NCL_OSAL_MutexLock(&queue->m_qMutex);
    newElem = queue->m_last;
    newElem->m_qNext->m_data = data;
    queue->m_last = queue->m_last->m_qNext;

  NCL_OSAL_MutexUnlock(&queue->m_qMutex);
EXIT:
  printf("[NCL_OSAL_Queue] - ret %d\n", ret);
  return ret;
}
NCL_ERRORTYPE NCL_OSAL_Dequeue(NCL_HANDLETYPE queueHandle,
                               NCL_OUT NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_Queue_SetElem(NCL_HANDLETYPE queueHandle,
                                     NCL_IN const NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_Queue_GetELem(NCL_HANDLETYPE queueHandle,
                                     NCL_OUT NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_QueueReset(NCL_HANDLETYPE queueHandle);