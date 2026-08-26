#ifndef __V4L2_CAPTURE_H__
#define __V4L2_CAPTURE_H__

#include "NCL_Core.h"
#include "NCL_Types.h"
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * buffer is in flight (single-buffer, single-shot capture, matching step 1
 * of the roadmap) -- call NCL_V4L2_CaptureStop right after reading the
 * frame, don't call GetFrame again without re-queuing first.
 * *frameData points directly into the mmap'd buffer (zero-copy) and stays
 * valid until NCL_V4L2_CaptureTerminate.
 */
NCL_ERRORTYPE NCL_V4L2_CaptureGetFrame(NCL_HANDLETYPE captureHandle,
                                       NCL_OUT NCL_PTR *frameData,
                                       NCL_OUT NCL_U32 *frameSize);

/** VIDIOC_STREAMOFF. */
NCL_ERRORTYPE NCL_V4L2_CaptureStop(NCL_HANDLETYPE captureHandle);

/** munmap + close + free. */
NCL_ERRORTYPE NCL_V4L2_CaptureTerminate(NCL_HANDLETYPE captureHandle);

#ifdef __cplusplus
}
#endif

#endif // __V4L2_CAPTURE_H__
