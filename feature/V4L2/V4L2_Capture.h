#ifndef __V4L2_CAPTURE_H__
#define __V4L2_CAPTURE_H__

#include "NCL_Core.h"
#include "NCL_Types.h"
#include <linux/videodev2.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Test seam: every NCL_V4L2_Capture* call goes through this table instead
 * of calling open/ioctl/mmap/munmap/close directly, so unit tests can swap
 * in a fake V4L2 device and run without real hardware. Production code
 * never touches this -- it's wired to the real syscalls by default.
 */
typedef struct _NCL_V4L2_SYSCALLS {
  int (*open)(const char *pathname, int flags);
  int (*ioctl)(int fd, unsigned long request, void *arg);
  void *(*mmap)(void *addr, size_t length, int prot, int flags, int fd,
                off_t offset);
  int (*munmap)(void *addr, size_t length);
  int (*close)(int fd);
} NCL_V4L2_SYSCALLS;

/** Pass NULL to restore the real syscalls. Test-only -- don't call from
 * production code. */
void NCL_V4L2_SetSyscalls(const NCL_V4L2_SYSCALLS *syscalls);

typedef struct _NCL_V4L2_CAPTURE {
    int m_fd;
    void *m_bufferStart;
    NCL_U32 m_bufferLength;
    struct v4l2_buffer m_buf;
    NCL_U32 m_width;
    NCL_U32 m_height;
    NCL_U32 m_pixelFormat;
} NCL_V4L2_CAPTURE;

/**
 * Opens devicePath, negotiates width/height/pixelFormat via VIDIOC_S_FMT,
 * requests a single MMAP buffer, and queues it (VIDIOC_QBUF). Leaves the
 * device ready for NCL_V4L2_CaptureStart. On success, capture->m_width/
 * m_height/m_pixelFormat hold what the driver actually negotiated, which
 * may differ from what was requested.
 */
NCL_ERRORTYPE NCL_V4L2_CaptureCreate(NCL_HANDLETYPE *captureHandle,
                                     NCL_IN const char *devicePath,
                                     NCL_IN NCL_U32 width,
                                     NCL_IN NCL_U32 height,
                                     NCL_IN NCL_U32 pixelFormat);

/** VIDIOC_STREAMON. */
NCL_ERRORTYPE NCL_V4L2_CaptureStart(NCL_HANDLETYPE captureHandle);

/**
 * VIDIOC_DQBUF (blocks until the driver has filled the buffer). Only one
 * buffer exists (single-buffer capture), so the driver has nowhere to write
 * the next frame until that buffer is handed back -- call
 * NCL_V4L2_CaptureReleaseFrame once done reading *frameData, before calling
 * GetFrame again. *frameData points directly into the mmap'd buffer
 * (zero-copy) and is only valid until the next ReleaseFrame call.
 */
NCL_ERRORTYPE NCL_V4L2_CaptureGetFrame(NCL_HANDLETYPE captureHandle,
                                       NCL_OUT NCL_PTR *frameData,
                                       NCL_OUT NCL_U32 *frameSize);

/**
 * VIDIOC_QBUF -- hands the buffer from the last GetFrame back to the driver
 * so it can capture the next frame into it. Must be called after every
 * GetFrame before the next GetFrame (or before Stop, for a single-shot
 * capture that skips it, that's fine too -- Stop doesn't need the buffer
 * back).
 */
NCL_ERRORTYPE NCL_V4L2_CaptureReleaseFrame(NCL_HANDLETYPE captureHandle);

/**
 * VIDIOC_EXPBUF -- exports the capture buffer as a DMA-BUF file descriptor
 * that a different driver/subsystem (another V4L2 device via
 * V4L2_MEMORY_DMABUF, DRM/KMS, EGL, ...) can import and access with zero
 * copy -- it's the same physical memory as *frameData from GetFrame, just
 * reachable through a kernel-standard fd instead of this process's mmap.
 * The caller owns *dmaBufFd and must close() it themselves once done;
 * NCL_V4L2_CaptureTerminate doesn't touch it.
 */
NCL_ERRORTYPE NCL_V4L2_CaptureExportBuffer(NCL_HANDLETYPE captureHandle,
                                           NCL_OUT int *dmaBufFd);

/** VIDIOC_STREAMOFF. */
NCL_ERRORTYPE NCL_V4L2_CaptureStop(NCL_HANDLETYPE captureHandle);

/** munmap + close + free. */
NCL_ERRORTYPE NCL_V4L2_CaptureTerminate(NCL_HANDLETYPE captureHandle);

#ifdef __cplusplus
}
#endif

#endif // __V4L2_CAPTURE_H__
