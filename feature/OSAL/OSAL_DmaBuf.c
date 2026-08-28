#include <errno.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_DmaBuf.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"

#define NCL_LOG_TAG2 "OSAL_DMABUF"

typedef struct _NCL_DMABUF {
  int m_fd;
  void *m_addr; // NULL until Map()
  NCL_U32 m_size;
} NCL_DMABUF;

static const NCL_DMABUF_SYSCALLS kRealSyscalls = {
    (int (*)(const char *, int))open,
    (int (*)(int, unsigned long, void *))ioctl,
    mmap,
    munmap,
    close,
    dup,
};

static const NCL_DMABUF_SYSCALLS *s_syscalls = &kRealSyscalls;

void NCL_OSAL_DmaBuf_SetSyscalls(const NCL_DMABUF_SYSCALLS *syscalls) {
  s_syscalls = syscalls ? syscalls : &kRealSyscalls;
}

/* Translates our public read/write/rw enum into the kernel's
 * DMA_BUF_SYNC_READ/WRITE/RW bits, combined with START or END. Shared by
 * SyncStart/SyncEnd so the bit-mapping lives in exactly one place. */
static __u64 ToKernelSyncFlags(NCL_DMABUF_SYNC_MODE mode, NCL_BOOLEAN isStart) {
  __u64 flags = isStart ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END;

  switch (mode) {
  case NCL_DMABUF_SYNC_READ:
    flags |= DMA_BUF_SYNC_READ;
    break;
  case NCL_DMABUF_SYNC_WRITE:
    flags |= DMA_BUF_SYNC_WRITE;
    break;
  case NCL_DMABUF_SYNC_RW:
  default:
    flags |= DMA_BUF_SYNC_RW;
    break;
  }
  return flags;
}

static NCL_ERRORTYPE DoSync(NCL_HANDLETYPE bufHandle, NCL_DMABUF_SYNC_MODE mode,
                            NCL_BOOLEAN isStart, const char *fnName) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;
  struct dma_buf_sync sync;
  int ioctlRet = 0;

  if (!buf) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }
  if (!buf->m_addr) {
    ret = NCL_ErrorInvalidState;
    LOGE(NCL_LOG_TAG2, "[%s] - not mapped, call Map first", fnName);
    goto EXIT;
  }

  memset(&sync, 0, sizeof(sync));
  sync.flags = ToKernelSyncFlags(mode, isStart);

  ioctlRet = s_syscalls->ioctl(buf->m_fd, DMA_BUF_IOCTL_SYNC, &sync);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[%s] - DMA_BUF_IOCTL_SYNC failed: %s", fnName,
         strerror(errno));
    goto EXIT;
  }

EXIT:
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufAlloc(NCL_HANDLETYPE *bufHandle,
                                   const char *heapPath, NCL_U32 size) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = NULL;
  int heapFd = -1;
  int ioctlRet = 0;
  struct dma_heap_allocation_data heapData;

  if (!bufHandle || !heapPath || size == 0) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  buf = (NCL_DMABUF *)NCL_OSAL_Malloc(sizeof(NCL_DMABUF));
  if (!buf) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufAlloc] - cannot allocate handle");
    goto EXIT;
  }
  NCL_OSAL_Memset(buf, 0, sizeof(NCL_DMABUF));

  heapFd = s_syscalls->open(heapPath, O_RDWR | O_CLOEXEC);
  if (heapFd < 0) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufAlloc] - open %s failed: %s",
         heapPath, strerror(errno));
    goto EXIT_FREE;
  }

  memset(&heapData, 0, sizeof(heapData));
  heapData.len = size;
  heapData.fd_flags = O_CLOEXEC | O_RDWR;
  heapData.heap_flags = 0;

  /* The heap device fd is only needed to make this one allocation ioctl --
   * the returned dma-buf fd is a separate object, so close it either way. */
  ioctlRet = s_syscalls->ioctl(heapFd, DMA_HEAP_IOCTL_ALLOC, &heapData);
  s_syscalls->close(heapFd);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2,
         "[NCL_OSAL_DmaBufAlloc] - DMA_HEAP_IOCTL_ALLOC failed: %s",
         strerror(errno));
    goto EXIT_FREE;
  }

  buf->m_fd = (int)heapData.fd;
  buf->m_size = size;
  *bufHandle = (NCL_HANDLETYPE)buf;
  goto EXIT;

EXIT_FREE:
  NCL_OSAL_Free(buf);
  goto EXIT;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufAlloc] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufImport(NCL_HANDLETYPE *bufHandle, int fd,
                                    NCL_U32 size, NCL_BOOLEAN takeOwnership) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = NULL;
  int ownFd = -1;

  if (!bufHandle || fd < 0 || size == 0) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (takeOwnership) {
    ownFd = fd;
  } else {
    ownFd = s_syscalls->dup(fd);
    if (ownFd < 0) {
      ret = NCL_ErrorInsufficientResources;
      LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufImport] - dup failed: %s",
           strerror(errno));
      goto EXIT;
    }
  }

  buf = (NCL_DMABUF *)NCL_OSAL_Malloc(sizeof(NCL_DMABUF));
  if (!buf) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufImport] - cannot allocate handle");
    if (!takeOwnership) {
      s_syscalls->close(ownFd);
    }
    goto EXIT;
  }
  NCL_OSAL_Memset(buf, 0, sizeof(NCL_DMABUF));
  buf->m_fd = ownFd;
  buf->m_size = size;

  *bufHandle = (NCL_HANDLETYPE)buf;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufImport] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufMap(NCL_HANDLETYPE bufHandle, NCL_PTR *addr) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;

  if (!buf || !addr) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (buf->m_addr) {
    *addr = buf->m_addr; // already mapped -- idempotent
    goto EXIT;
  }

  buf->m_addr = s_syscalls->mmap(NULL, buf->m_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, buf->m_fd, 0);
  if (buf->m_addr == MAP_FAILED) {
    buf->m_addr = NULL;
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufMap] - mmap failed: %s",
         strerror(errno));
    goto EXIT;
  }

  *addr = buf->m_addr;

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufMap] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufUnmap(NCL_HANDLETYPE bufHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;

  if (!buf) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (buf->m_addr) {
    s_syscalls->munmap(buf->m_addr, buf->m_size);
    buf->m_addr = NULL;
  }

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufUnmap] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufSyncStart(NCL_HANDLETYPE bufHandle,
                                       NCL_DMABUF_SYNC_MODE mode) {
  NCL_ERRORTYPE ret = DoSync(bufHandle, mode, NCL_TRUE,
                             "NCL_OSAL_DmaBufSyncStart");
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufSyncStart] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufSyncEnd(NCL_HANDLETYPE bufHandle,
                                     NCL_DMABUF_SYNC_MODE mode) {
  NCL_ERRORTYPE ret = DoSync(bufHandle, mode, NCL_FALSE,
                             "NCL_OSAL_DmaBufSyncEnd");
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufSyncEnd] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufGetFd(NCL_HANDLETYPE bufHandle, int *fd) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;

  if (!buf || !fd) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  *fd = s_syscalls->dup(buf->m_fd);
  if (*fd < 0) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufGetFd] - dup failed: %s",
         strerror(errno));
    goto EXIT;
  }

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufGetFd] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufGetSize(NCL_HANDLETYPE bufHandle, NCL_U32 *size) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;

  if (!buf || !size) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  *size = buf->m_size;

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufGetSize] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_OSAL_DmaBufTerminate(NCL_HANDLETYPE bufHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_DMABUF *buf = (NCL_DMABUF *)bufHandle;

  if (!buf) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (buf->m_addr) {
    s_syscalls->munmap(buf->m_addr, buf->m_size);
    buf->m_addr = NULL;
  }
  s_syscalls->close(buf->m_fd);
  NCL_OSAL_Free(buf);

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_OSAL_DmaBufTerminate] - ret %d", ret);
  return ret;
}
