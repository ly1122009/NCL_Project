#include "OSAL_Thread.h"
#include <bits/pthread_types.h>
#include <linux/sched/types.h>

typedef struct _NCL_THREAD_HANDLE_TYPE
{
    pthread_t pthread;
    pthread_attr_t attr;
    struct sched_attr schedparam;
    int stack_size;
} NCL_THREAD_HANDLE_TYPE;

// NCL_ERRORTYPE NCL_OSAL_ThreadCreate(NCL_HANDLETYPE *thread_handle, NCL_PTR func_name, NCL_PTR param, NCL_THREAD_TYPE thread_type)
// {
//     NCL_ERRORTYPE ret = NCL_ErrorNode;
//     if (!thread_handle)
//     {
        
//     }
// }

NCL_ERRORTYPE NCL_OSAL_ThreadTerminate(NCL_HANDLETYPE *thread_handle);
NCL_ERRORTYPE NCL_OSAL_ThreadExit(NCL_HANDLETYPE thread_handle);
NCL_ERRORTYPE NCL_OSAL_ThreadCancel(void* value_ptr);
NCL_ERRORTYPE NCL_OSAL_SleepMillisec(OMX_U32 ms);