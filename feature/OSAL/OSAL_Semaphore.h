#ifndef __OSAL_SEMAPHORE_H__
#define __OSAL_SEMAPHORE_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif    

NCL_ERRORTYPE NCL_OSAL_SemaphoreCreate(NCL_HANDLETYPE *semaphoreHandle);
NCL_ERRORTYPE NCL_OSAL_SemaphoreTerminate(NCL_HANDLETYPE semaphoreHandle);
NCL_ERRORTYPE NCL_OSAL_SemaphoreTryWait(NCL_HANDLETYPE semaphoreHandle);
NCL_ERRORTYPE NCL_OSAL_SemaphoreWait(NCL_HANDLETYPE semaphoreHandle);
NCL_HANDLETYPE NCL_OSAL_SemaphorePost(NCL_HANDLETYPE semaphoreHandle);
NCL_HANDLETYPE NCL_OSAL_Get_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle, NCL_OUT NCL_U32* value);
NCL_HANDLETYPE NCL_OSAL_Set_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle, NCL_IN const NCL_U32 value);


#ifdef __cplusplus
}
#endif

#endif // __OSAL_SEMAPHORE_H__