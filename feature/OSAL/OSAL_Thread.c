#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

#include "OSAL_Thread.h"
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Memory.h"

typedef struct _NCL_THREAD_HANDLE_TYPE
{
    pthread_t pthread;
    pthread_attr_t attr;
    struct sched_param schedparam;
    int stack_size;
} NCL_THREAD_HANDLE_TYPE;

NCL_ERRORTYPE NCL_OSAL_ThreadCreate(NCL_HANDLETYPE *thread_handle, NCL_PTR func_name, NCL_PTR param, NCL_THREAD_TYPE thread_type)
{
    int thread_ret = 0;
    int detach_ret = 0;
    NCL_ERRORTYPE ret = NCL_ErrorNone;
    NCL_THREAD_HANDLE_TYPE *thread = NULL;
    int detachState = PTHREAD_CREATE_DETACHED;
    if (!thread_handle)
    {
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

    thread = NCL_OSAL_Malloc(sizeof(NCL_THREAD_HANDLE_TYPE));
    if (thread == NULL)
    {
        ret = NCL_ErrorInsufficientResources;
        *thread_handle = NULL;
        goto EXIT;
    }
    NCL_OSAL_Memset(thread, 0, sizeof(NCL_THREAD_HANDLE_TYPE));

    pthread_attr_init(&thread->attr);
    
    // TODO: Stack size and priority
    if (thread->stack_size != 0)
        pthread_attr_setstacksize(&thread->attr, thread->stack_size);

    if (thread->schedparam.sched_priority != 0)
        pthread_attr_setschedparam(&thread->attr, &thread->schedparam);

    detach_ret = pthread_attr_setdetachstate(&thread->attr, detachState);
    if (detach_ret != 0)
    {
        printf("ERROR: [NCL_OSAL_ThreadCreate] - pthread_attr_setdetachstate failed! %d\n", detach_ret);
        NCL_OSAL_Free(thread);
        *thread_handle = NULL;
        ret = NCL_ErrorUndefined;
        goto EXIT;
    }

    // Convert NCL_PTR -> Function pointer
    thread_ret = pthread_create(&thread->pthread, &thread->attr, (void *(*)(void *))func_name, (void*)param);
    switch (thread_ret) {
        case 0:
            *thread_handle = (NCL_HANDLETYPE)thread;
            printf("[NCL_OSAL_ThreadCreate] - thread id %lu is created\n", thread->pthread);
            ret = NCL_ErrorNone;
            break;
        case EAGAIN:
            *thread_handle = NULL;
            NCL_OSAL_Free(thread);
            printf("ERROR: [NCL_OSAL_ThreadCreate] -  pthread_create failed EAGAIN %d\n", thread_ret);
            ret = NCL_ErrorUndefined;
            break;
        default:
            *thread_handle = NULL;
            NCL_OSAL_Free(thread);
            printf("ERROR: [NCL_OSAL_ThreadCreate] -  pthread_create failed %d\n", thread_ret);
            ret = NCL_ErrorUndefined;
            break;
    }

EXIT:
    printf("ERROR: [NCL_OSAL_ThreadCreate] out - ret: %d\n", ret);
    return ret;
}

NCL_ERRORTYPE NCL_OSAL_ThreadTerminate(NCL_HANDLETYPE *thread_handle)
{
    NCL_ERRORTYPE ret = NCL_ErrorNone;
    int join_ret = 0;
    NCL_THREAD_HANDLE_TYPE *thread = (NCL_THREAD_HANDLE_TYPE*)thread_handle;
    if (!thread_handle)
    {
        ret = NCL_ErrorBadParameter;
        goto EXIT;
    }
    join_ret = pthread_join(thread->pthread, NULL);
    if(0 != join_ret)
    {
        printf("ERROR: [NCL_OSAL_ThreadTerminate] - pthread_join failed %d\n", join_ret);
        ret = NCL_ErrorBadParameter;
        goto EXIT;
    }

    NCL_OSAL_Free(&thread->pthread);
    printf("[NCL_OSAL_ThreadTerminate] thread id %lu join successfully %d\n", thread->pthread ,join_ret);
    ret = NCL_ErrorNone;


EXIT:
    printf("ERROR: [NCL_OSAL_ThreadCreate] out - ret: %d", ret);
    return ret;
}

void NCL_OSAL_ThreadExit(void* value_ptr)
{
    pthread_exit(value_ptr);
    return;
}

void NCL_OSAL_SleepMillisec(NCL_U32 ms)
{
    usleep(ms);
    return;
}
