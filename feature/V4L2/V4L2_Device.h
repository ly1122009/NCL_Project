#ifndef __V4L2_DEVICE_H__
#define __V4L2_DEVICE_H__

#include "NCL_Core.h"
#include "NCL_Types.h"

#define NCL_V4L2_MAX_BUFFERS 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NCL_V4L2_FORMAT {
  NCL_U32 width;
  NCL_U32 height;
  NCL_U32 pixelFormat; // fourcc, e.g. V4L2_PIX_FMT_YUYV from <linux/videodev2.h>
} NCL_V4L2_FORMAT;

// Opens the device node and validates it supports capture + streaming I/O.
NCL_ERRORTYPE NCL_V4L2_Open(NCL_HANDLETYPE *deviceHandle, const char *devicePath);

// Stops streaming (if active), unmaps buffers, and closes the device.
NCL_ERRORTYPE NCL_V4L2_Close(NCL_HANDLETYPE deviceHandle);

// Must be called before NCL_V4L2_RequestBuffers. The driver may adjust
// width/height/pixelFormat to the nearest value it actually supports —
// the negotiated values are written back into *format.
NCL_ERRORTYPE NCL_V4L2_SetFormat(NCL_HANDLETYPE deviceHandle, NCL_V4L2_FORMAT *format);

// Requests `count` MMAP buffers, maps each one, and queues them all with
// the driver so capture can start as soon as NCL_V4L2_StreamOn is called.
NCL_ERRORTYPE NCL_V4L2_RequestBuffers(NCL_HANDLETYPE deviceHandle, NCL_U32 count);

NCL_ERRORTYPE NCL_V4L2_StreamOn(NCL_HANDLETYPE deviceHandle);
NCL_ERRORTYPE NCL_V4L2_StreamOff(NCL_HANDLETYPE deviceHandle);

// Blocks until the driver has a filled buffer ready. *data points directly
// into the mmap'd buffer — valid only until the matching
// NCL_V4L2_QueueBuffer(index) call hands the buffer back to the driver.
NCL_ERRORTYPE NCL_V4L2_DequeueBuffer(NCL_HANDLETYPE deviceHandle, NCL_U32 *index,
                                     NCL_U8 **data, NCL_U32 *bytesUsed);

// Returns a buffer (previously obtained via DequeueBuffer) to the driver
// so it can be filled with the next frame.
NCL_ERRORTYPE NCL_V4L2_QueueBuffer(NCL_HANDLETYPE deviceHandle, NCL_U32 index);

#ifdef __cplusplus
}
#endif

#endif // __V4L2_DEVICE_H__
