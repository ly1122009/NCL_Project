#ifndef __OSAL_MEMORY_H__
#define __OSAL_MEMORY_H__

#include "NCL_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

NCL_PTR NCL_OSAL_Malloc(NCL_U32 size);
void    NCL_OSAL_Free(NCL_PTR addr);
NCL_PTR NCL_OSAL_Memset(NCL_PTR dest, NCL_S32 src, NCL_S32 size);
NCL_PTR NCL_OSAL_Memcpy(NCL_PTR dest, NCL_S32 src, NCL_S32 size);
NCL_PTR NCL_OSAL_Memmove(NCL_PTR dest, NCL_S32 src, NCL_S32 size);


#ifdef __cplusplus
}
#endif

#endif // __OSAL_MEMORY_H__