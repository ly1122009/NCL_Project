#include <dlfcn.h>
#include <stdio.h>

#include "OSAL_Library.h"

void *NCL_OSAL_dlopen(const char *filename, int flag) {
  return dlopen(filename, flag);
}

void *NCL_OSAL_dlsym(void *handle, const char *symbol) {
  return dlsym(handle, symbol);
}

int NCL_OSAL_dlclose(void *handle) { return dlclose(handle); }
const char *NCL_OSAL_dlerror(void) { return dlerror(); }
