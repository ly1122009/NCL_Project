#ifndef __V4L2_CAPTURE_THREAD_H__
#define __V4L2_CAPTURE_THREAD_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

// One captured frame handed from the capture thread to a consumer via
// OSAL_Queue. `data`/`bytesUsed` alias the V4L2 device's mmap'd buffer —
// valid only until the consumer calls NCL_V4L2_QueueBuffer(device, index)
// to hand the buffer back to the driver.
typedef struct _NCL_V4L2_FRAME {
  NCL_U32 index;
  NCL_U8 *data;
  NCL_U32 bytesUsed;
} NCL_V4L2_FRAME;

// Starts a background thread that loops NCL_V4L2_DequeueBuffer and pushes
// each captured frame onto `frameQueue`. Does NOT call NCL_V4L2_StreamOn —
// the caller must do that first so the very first DQBUF has something to
// wait for. `bufferCount` must match what was passed to
// NCL_V4L2_RequestBuffers (used to size an internal, pre-allocated pool of
// NCL_V4L2_FRAME slots, one per buffer index, so nothing is malloc'd on
// the per-frame hot path).
//
// The consumer draining `frameQueue` owns returning each frame to the
// driver via NCL_V4L2_QueueBuffer once it is done reading `data` — this
// thread never requeues a frame it successfully handed off.
NCL_ERRORTYPE NCL_V4L2_CaptureThreadStart(NCL_HANDLETYPE *captureHandle,
                                          NCL_HANDLETYPE device,
                                          NCL_HANDLETYPE frameQueue,
                                          NCL_U32 bufferCount);

// Signals the capture thread to stop and joins it. Takes effect after the
// in-flight NCL_V4L2_DequeueBuffer call returns (at most one frame period).
NCL_ERRORTYPE NCL_V4L2_CaptureThreadStop(NCL_HANDLETYPE captureHandle);

// Function-pointer seam matching NCL_V4L2_DequeueBuffer/NCL_V4L2_QueueBuffer.
// Exists so tests can substitute a fake device and exercise the loop's
// logic (frame hand-off, queue-full drop path, clean stop on a hardware
// error) without a real camera.
typedef NCL_ERRORTYPE (*NCL_V4L2_DequeueBufferFn)(NCL_HANDLETYPE device,
                                                   NCL_U32 *index,
                                                   NCL_U8 **data,
                                                   NCL_U32 *bytesUsed);
typedef NCL_ERRORTYPE (*NCL_V4L2_QueueBufferFn)(NCL_HANDLETYPE device,
                                                 NCL_U32 index);

// Identical to NCL_V4L2_CaptureThreadStart but lets the caller supply the
// Dequeue/QueueBuffer implementation. NCL_V4L2_CaptureThreadStart is just
// this function called with the real NCL_V4L2_DequeueBuffer/QueueBuffer —
// production code should keep using that; this is the test seam.
NCL_ERRORTYPE NCL_V4L2_CaptureThreadStartWithOps(
    NCL_HANDLETYPE *captureHandle, NCL_HANDLETYPE device,
    NCL_HANDLETYPE frameQueue, NCL_U32 bufferCount,
    NCL_V4L2_DequeueBufferFn dequeueFn, NCL_V4L2_QueueBufferFn queueFn);

#ifdef __cplusplus
}
#endif

#endif // __V4L2_CAPTURE_THREAD_H__
