#ifndef __OSAL_EVENT_H__
#define __OSAL_EVENT_H__

#include "NCL_Core.h"
#include "NCL_Types.h"
#include <pthread.h>

#define MAX_WAIT_TIME 0xFFFFFFFF

typedef struct _NCL_OSAL_THREADEVENT {
  NCL_BOOLEAN m_signal;
  NCL_HANDLETYPE m_mutex;
  pthread_cond_t m_condition;
} NCL_OSAL_THREADEVENT;

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif

NCL_ERRORTYPE NCL_OSAL_SignalCreate(NCL_HANDLETYPE *eventHandle);
NCL_ERRORTYPE NCL_OSAL_SignalTerminate(NCL_HANDLETYPE eventHandle);
NCL_ERRORTYPE NCL_OSAL_SignalReset(NCL_HANDLETYPE eventHandle);
NCL_ERRORTYPE NCL_OSAL_SignalSet(NCL_HANDLETYPE eventHandle);
NCL_ERRORTYPE NCL_OSAL_SignalWait(NCL_HANDLETYPE eventHandle, NCL_U32 ms);

#ifdef __cplusplus
}
#endif

#endif // __OSAL_EVENT_H__
