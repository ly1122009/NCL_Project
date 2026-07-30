#ifndef __OSAL_MUTEX_H__
#define __OSAL_MUTEX_H__

#include "NCL_Types.h"
#include "NCL_Core.h"

#ifdef __cplusplus
extern "C" {
#endif

NCL_ERRORTYPE NCL_OSAL_MutexCreate(NCL_HANDLETYPE* mutexHandle);
NCL_ERRORTYPE NCL_OSAL_MutexTerminate(NCL_HANDLETYPE mutexHandle);
NCL_ERRORTYPE NCL_OSAL_MutexLock(NCL_HANDLETYPE mutexHandle);
NCL_ERRORTYPE NCL_OSAL_MutexUnlock(NCL_HANDLETYPE mutexHandle);


#ifdef __cplusplus
}
#endif

#endif // __OSAL_MUTEX_H__