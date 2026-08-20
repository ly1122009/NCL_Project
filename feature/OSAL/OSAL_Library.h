#ifndef __OSAL_LIBRARY_H__
#define __OSAL_LIBRARY_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void *NCL_OSAL_dlopen(const char *filename, int flag);
void *NCL_OSAL_dlsym(void *handle, const char *symbol);
int NCL_OSAL_dlclose(void *handle);
const char *NCL_OSAL_dlerror(void);

#ifdef __cplusplus
}
#endif

#endif // __OSAL_LIBRARY_H__