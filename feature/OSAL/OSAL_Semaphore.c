#include "OSAL_Semaphore.h"
#include "NCL_Core.h"
#include "OSAL_Memory.h"
#include <semaphore.h>
#include <pthread.h>
// #include <stdlib.h>

NCL_ERRORTYPE NCL_OSAL_SemaphoreCreate(NCL_HANDLETYPE *semaphoreHandle)
{
    NCL_ERRORTYPE ret = NCL_ErrorNone;
    int sem_ret = 0;


    if (!semaphoreHandle)
    {
        ret = NCL_ErrorBadParameter;
        goto EXIT;    
    }
    sem_t* semaphore;
    semaphore = (sem_t*)NCL_OSAL_Malloc(sizeof(sem_t));
    sem_ret = sem_init(semaphore, 0, 0);
    if (sem_ret != 0)
    {
        NCL_OSAL_Free(semaphore);
        ret = NCL_ErrorUndefined;
        goto EXIT;
    }
    


EXIT:
    printf("ERROR: [NCL_OSAL_SemaphoreCreate] - %d\n",ret);
    return ret;
}

NCL_ERRORTYPE NCL_OSAL_SemaphoreTerminate(NCL_HANDLETYPE semaphoreHandle);
NCL_ERRORTYPE NCL_OSAL_SemaphoreTryWait(NCL_HANDLETYPE semaphoreHandle);
NCL_ERRORTYPE NCL_OSAL_SemaphoreWait(NCL_HANDLETYPE semaphoreHandle);
NCL_HANDLETYPE NCL_OSAL_SemaphorePost(NCL_HANDLETYPE semaphoreHandle);
NCL_HANDLETYPE NCL_OSAL_Get_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle, NCL_OUT NCL_U32* value);
NCL_HANDLETYPE NCL_OSAL_Set_SemaphoreCount(NCL_HANDLETYPE semaphoreHandle, NCL_IN const NCL_U32 value);