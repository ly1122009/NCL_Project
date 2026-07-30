#ifndef __OSAL_THREAD_H__
#define __OSAL_THREAD_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

typedef enum {
    NCL_THREAD_JOINABLE,
    NCL_THREAD_DETACHED
} NCL_THREAD_TYPE;

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif    

NCL_ERRORTYPE NCL_OSAL_ThreadCreate(NCL_HANDLETYPE *thread_handle, NCL_PTR func_name, NCL_PTR param, NCL_THREAD_TYPE thread_type);
NCL_ERRORTYPE NCL_OSAL_ThreadTerminate(NCL_HANDLETYPE *thread_handle);
void NCL_OSAL_ThreadExit(void* value_ptr);
void NCL_OSAL_SleepMillisec(NCL_U32 ms);

#ifdef __cplusplus
}
#endif

#endif // __OSAL_THREAD_H__