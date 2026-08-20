#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OSAL_Memory.h"

static _Atomic(int) mem_cnt = 0;

NCL_PTR NCL_OSAL_Malloc(NCL_U32 size) {
  // mem_cnt++;
  atomic_fetch_add(&mem_cnt, 1);
  return (NCL_PTR)malloc(size);
}

void NCL_OSAL_Free(NCL_PTR addr) {
  // mem_cnt--;
  if (addr) {
    atomic_fetch_sub(&mem_cnt, 1);
    free(addr);
  }
  return;
}

NCL_PTR NCL_OSAL_Memset(NCL_PTR dest, NCL_S32 src, NCL_S32 size) {
  return memset(dest, src, size);
}

NCL_PTR NCL_OSAL_Memcpy(NCL_PTR dest, NCL_PTR src, NCL_S32 size) {
  return memcpy(dest, src, size);
}

NCL_PTR NCL_OSAL_Memmove(NCL_PTR dest, NCL_PTR src, NCL_S32 size) {
  return memmove(dest, src, size);
}
