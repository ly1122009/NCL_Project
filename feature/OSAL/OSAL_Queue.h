#ifndef __OSAL_QUEUE_H__
#define __OSAL_QUEUE_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

#define MAX_QUEUE_ELEMENTS 40

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif

typedef struct _NCL_QElem {
  void *m_data;
  struct _NCL_QElem *m_qNext;
} NCL_QElem;

typedef struct _NCL_QUEUE {
  NCL_QElem *m_first;
  NCL_QElem *m_last;
  int numELem;
  int maxNumElem;
  NCL_HANDLETYPE m_qMutex;
} NCL_QUEUE;

NCL_ERRORTYPE NCL_OSAL_QueueCreate(NCL_HANDLETYPE *queueHandle, int maxNumElem);
NCL_ERRORTYPE NCL_OSAL_QueueTerminate(NCL_HANDLETYPE queueHandle);
NCL_ERRORTYPE NCL_OSAL_Queue(NCL_HANDLETYPE queueHandle,
                             NCL_IN const NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_Dequeue(NCL_HANDLETYPE queueHandle,
                               NCL_OUT NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_Queue_SetElem(NCL_HANDLETYPE queueHandle,
                                     NCL_IN const NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_Queue_GetELem(NCL_HANDLETYPE queueHandle,
                                     NCL_OUT NCL_PTR data);
NCL_ERRORTYPE NCL_OSAL_QueueReset(NCL_HANDLETYPE queueHandle);

#ifdef __cplusplus
}
#endif

#endif // __OSAL_QUEUE_H__