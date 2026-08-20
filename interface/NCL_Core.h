#ifndef __OSAL_CORE_H__
#define __OSAL_CORE_H__

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif    

#include <stdint.h>

typedef enum _NCL_STATETYPE
{
    NCL_StateInvalid = 0,
    NCL_StateWaitingForResources = 1,

    
    NCL_StateMax = 0x7FFFFFFF 
} NCL_STATETYPE;

typedef enum _NCL_ERRORTYPE
{
    NCL_ErrorNone = 0,
    NCL_ErrorBadParameter,  // Invalid input
    NCL_NotImplemented,     // Feature isn't implemented
    NCL_ErrorInvalidState,  // State machine violation
    NCL_ErrorTimeout,       // For semaphore and signals
    NCL_ErrorHardware,      // For Hardware doesnot support or not
    NCL_ErrorNotReady,       // For not ready component
    NCL_ErrorInsufficientResources, // For insufficient resources
    NCL_ErrorUndefined      // For the error could not be determined
} NCL_ERRORTYPE;


#ifdef __cplusplus
}
#endif

#endif // __OSAL_CORE_H__