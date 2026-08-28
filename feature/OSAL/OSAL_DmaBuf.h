#ifndef __OSAL_DMABUF_H__
#define __OSAL_DMABUF_H__

#include "NCL_Core.h"
#include "NCL_Types.h"
#include <sys/types.h>

#ifdef __cplusplus // avoid overloading
extern "C" {
#endif

/**
 * Matches the DMA_BUF_IOCTL_SYNC read/write flags (DMA_BUF_SYNC_READ/
 * WRITE/RW) one-to-one -- kept explicit rather than a bool so callers state
 * the same CPU access intent the kernel cache-sync ioctl actually wants.
 */
typedef enum {
  NCL_DMABUF_SYNC_READ,
  NCL_DMABUF_SYNC_WRITE,
  NCL_DMABUF_SYNC_RW
} NCL_DMABUF_SYNC_MODE;

/**
 * Test seam: every NCL_OSAL_DmaBuf* call goes through this table instead of
 * calling open/ioctl/mmap/munmap/close/dup directly, so unit tests can run
 * without a real dma_heap device node. Production code never touches
 * this -- it's wired to the real syscalls by default.
 */
typedef struct _NCL_DMABUF_SYSCALLS {
  int (*open)(const char *pathname, int flags);
  int (*ioctl)(int fd, unsigned long request, void *arg);
  void *(*mmap)(void *addr, size_t length, int prot, int flags, int fd,
                off_t offset);
  int (*munmap)(void *addr, size_t length);
  int (*close)(int fd);
  int (*dup)(int fd);
} NCL_DMABUF_SYSCALLS;

/** Pass NULL to restore the real syscalls. Test-only -- don't call from
 * production code. */
void NCL_OSAL_DmaBuf_SetSyscalls(const NCL_DMABUF_SYSCALLS *syscalls);

/**
 * Allocates size bytes of dma-buf-backed memory from a DMA-BUF Heap
 * (DMA_HEAP_IOCTL_ALLOC against heapPath, e.g. "/dev/dma_heap/linux,cma")
 * and returns a handle wrapping the new dma-buf fd. This is the
 * driver-agnostic way to get a dma-buf without needing some other
 * subsystem (V4L2, DRM, ...) to export one for you.
 */
NCL_ERRORTYPE NCL_OSAL_DmaBufAlloc(NCL_HANDLETYPE *bufHandle,
                                   NCL_IN const char *heapPath,
                                   NCL_IN NCL_U32 size);

/**
 * Wraps an existing dma-buf fd (e.g. from NCL_V4L2_CaptureExportBuffer) in
 * a handle. If takeOwnership is NCL_TRUE, this handle now owns fd -- the
 * caller must not touch or close(fd) again. If NCL_FALSE, the handle
 * dup()s its own copy, leaving the caller free to keep using/closing
 * their original fd independently. Either way,
 * NCL_OSAL_DmaBufTerminate always closes exactly the fd this handle
 * itself holds -- there is no separate "owns it or not" flag to get
 * wrong later.
 */
NCL_ERRORTYPE NCL_OSAL_DmaBufImport(NCL_HANDLETYPE *bufHandle,
                                    NCL_IN int fd, NCL_IN NCL_U32 size,
                                    NCL_IN NCL_BOOLEAN takeOwnership);

/** mmap's the underlying fd into this process. Idempotent -- calling Map
 * again on an already-mapped handle just returns the cached pointer. */
NCL_ERRORTYPE NCL_OSAL_DmaBufMap(NCL_HANDLETYPE bufHandle,
                                 NCL_OUT NCL_PTR *addr);

/** munmap's the buffer. Safe to call even if never mapped (no-op). */
NCL_ERRORTYPE NCL_OSAL_DmaBufUnmap(NCL_HANDLETYPE bufHandle);

/**
 * DMA_BUF_IOCTL_SYNC bracketing -- call SyncStart before the CPU reads/
 * writes the mapped memory and SyncEnd once done, so the kernel can keep
 * CPU and device views coherent. Requires Map to have been called first
 * (NCL_ErrorInvalidState otherwise, since there's no CPU mapping to sync).
 */
NCL_ERRORTYPE NCL_OSAL_DmaBufSyncStart(NCL_HANDLETYPE bufHandle,
                                       NCL_IN NCL_DMABUF_SYNC_MODE mode);
NCL_ERRORTYPE NCL_OSAL_DmaBufSyncEnd(NCL_HANDLETYPE bufHandle,
                                     NCL_IN NCL_DMABUF_SYNC_MODE mode);

/** Returns a dup()'d fd the caller now owns, to hand to another
 * subsystem (or, later, across a process boundary via SCM_RIGHTS). */
NCL_ERRORTYPE NCL_OSAL_DmaBufGetFd(NCL_HANDLETYPE bufHandle,
                                   NCL_OUT int *fd);

NCL_ERRORTYPE NCL_OSAL_DmaBufGetSize(NCL_HANDLETYPE bufHandle,
                                     NCL_OUT NCL_U32 *size);

/** Unmaps (if mapped) and closes this handle's fd, then frees the
 * handle. */
NCL_ERRORTYPE NCL_OSAL_DmaBufTerminate(NCL_HANDLETYPE bufHandle);

#ifdef __cplusplus
}
#endif

#endif // __OSAL_DMABUF_H__
