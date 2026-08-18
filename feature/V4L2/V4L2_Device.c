#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "V4L2_Device.h"

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "V4L2_DEVICE"

typedef struct _NCL_V4L2_MMAP_BUFFER {
  void *start;
  size_t length;
} NCL_V4L2_MMAP_BUFFER;

typedef struct _NCL_V4L2_DEVICE {
  int fd;
  NCL_V4L2_MMAP_BUFFER buffers[NCL_V4L2_MAX_BUFFERS];
  NCL_U32 bufferCount;
  NCL_BOOLEAN streaming;
} NCL_V4L2_DEVICE;

// ioctl() can be interrupted by a signal (EINTR) without anything actually
// being wrong; the standard V4L2 idiom is to just retry it.
static int xioctl(int fd, unsigned long request, void *arg) {
  int ret;
  do {
    ret = ioctl(fd, request, arg);
  } while (ret == -1 && errno == EINTR);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_Open(NCL_HANDLETYPE *deviceHandle, const char *devicePath) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = NULL;
  struct v4l2_capability cap;

  if (!deviceHandle || !devicePath) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  device = (NCL_V4L2_DEVICE *)NCL_OSAL_Malloc(sizeof(NCL_V4L2_DEVICE));
  if (!device) {
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }
  NCL_OSAL_Memset(device, 0, sizeof(NCL_V4L2_DEVICE));

  device->fd = open(devicePath, O_RDWR);
  if (device->fd < 0) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_Open] - open(%s) failed: %s", devicePath,
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto FREE_DEVICE;
  }

  NCL_OSAL_Memset(&cap, 0, sizeof(cap));
  if (xioctl(device->fd, VIDIOC_QUERYCAP, &cap) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_Open] - VIDIOC_QUERYCAP failed: %s",
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto CLOSE_FD;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_Open] - %s is not a video capture device",
         devicePath);
    ret = NCL_ErrorHardware;
    goto CLOSE_FD;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_Open] - %s does not support streaming I/O",
         devicePath);
    ret = NCL_ErrorHardware;
    goto CLOSE_FD;
  }

  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_Open] - opened %s (driver=%s, card=%s)",
       devicePath, cap.driver, cap.card);
  *deviceHandle = (NCL_HANDLETYPE)device;
  goto EXIT;

CLOSE_FD:
  close(device->fd);
FREE_DEVICE:
  NCL_OSAL_Free(device);
EXIT:
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_Close(NCL_HANDLETYPE deviceHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  NCL_U32 i;

  if (!device) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (device->streaming) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(device->fd, VIDIOC_STREAMOFF, &type);
    device->streaming = NCL_FALSE;
  }

  for (i = 0; i < device->bufferCount; i++) {
    if (device->buffers[i].start) {
      munmap(device->buffers[i].start, device->buffers[i].length);
      device->buffers[i].start = NULL;
    }
  }

  close(device->fd);
  NCL_OSAL_Free(device);

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_Close] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_SetFormat(NCL_HANDLETYPE deviceHandle, NCL_V4L2_FORMAT *format) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  struct v4l2_format fmt;

  if (!device || !format) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_Memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = format->width;
  fmt.fmt.pix.height = format->height;
  fmt.fmt.pix.pixelformat = format->pixelFormat;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;

  if (xioctl(device->fd, VIDIOC_S_FMT, &fmt) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_SetFormat] - VIDIOC_S_FMT failed: %s",
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto EXIT;
  }

  if (fmt.fmt.pix.width != format->width || fmt.fmt.pix.height != format->height ||
      fmt.fmt.pix.pixelformat != format->pixelFormat) {
    LOGW(NCL_LOG_TAG2,
         "[NCL_V4L2_SetFormat] - driver adjusted format to %ux%u fourcc=0x%08x",
         fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
  }

  // Report back what the driver actually negotiated, not what was asked for.
  format->width = fmt.fmt.pix.width;
  format->height = fmt.fmt.pix.height;
  format->pixelFormat = fmt.fmt.pix.pixelformat;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_SetFormat] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_RequestBuffers(NCL_HANDLETYPE deviceHandle, NCL_U32 count) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  NCL_U32 i;

  if (!device || count == 0 || count > NCL_V4L2_MAX_BUFFERS) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_Memset(&req, 0, sizeof(req));
  req.count = count;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (xioctl(device->fd, VIDIOC_REQBUFS, &req) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_RequestBuffers] - VIDIOC_REQBUFS failed: %s",
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto EXIT;
  }

  if (req.count < 2) {
    LOGE(NCL_LOG_TAG2,
         "[NCL_V4L2_RequestBuffers] - driver only granted %u buffer(s)",
         req.count);
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }

  for (i = 0; i < req.count; i++) {
    NCL_OSAL_Memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if (xioctl(device->fd, VIDIOC_QUERYBUF, &buf) == -1) {
      LOGE(NCL_LOG_TAG2,
           "[NCL_V4L2_RequestBuffers] - VIDIOC_QUERYBUF(%u) failed: %s", i,
           strerror(errno));
      ret = NCL_ErrorHardware;
      goto UNMAP;
    }

    device->buffers[i].length = buf.length;
    device->buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, device->fd, buf.m.offset);
    if (device->buffers[i].start == MAP_FAILED) {
      LOGE(NCL_LOG_TAG2, "[NCL_V4L2_RequestBuffers] - mmap(%u) failed: %s", i,
           strerror(errno));
      device->buffers[i].start = NULL;
      ret = NCL_ErrorHardware;
      goto UNMAP;
    }

    if (xioctl(device->fd, VIDIOC_QBUF, &buf) == -1) {
      LOGE(NCL_LOG_TAG2, "[NCL_V4L2_RequestBuffers] - VIDIOC_QBUF(%u) failed: %s",
           i, strerror(errno));
      ret = NCL_ErrorHardware;
      goto UNMAP;
    }
  }

  device->bufferCount = req.count;
  goto EXIT;

UNMAP:
  for (i = 0; i < NCL_V4L2_MAX_BUFFERS; i++) {
    if (device->buffers[i].start) {
      munmap(device->buffers[i].start, device->buffers[i].length);
      device->buffers[i].start = NULL;
    }
  }
EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_RequestBuffers] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_StreamOn(NCL_HANDLETYPE deviceHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (!device) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (xioctl(device->fd, VIDIOC_STREAMON, &type) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_StreamOn] - VIDIOC_STREAMON failed: %s",
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto EXIT;
  }
  device->streaming = NCL_TRUE;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_StreamOn] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_StreamOff(NCL_HANDLETYPE deviceHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (!device) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (xioctl(device->fd, VIDIOC_STREAMOFF, &type) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_StreamOff] - VIDIOC_STREAMOFF failed: %s",
         strerror(errno));
    ret = NCL_ErrorHardware;
    goto EXIT;
  }
  device->streaming = NCL_FALSE;

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_StreamOff] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_DequeueBuffer(NCL_HANDLETYPE deviceHandle, NCL_U32 *index,
                                     NCL_U8 **data, NCL_U32 *bytesUsed) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  struct v4l2_buffer buf;

  if (!device || !index || !data || !bytesUsed) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_Memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (xioctl(device->fd, VIDIOC_DQBUF, &buf) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_DequeueBuffer] - VIDIOC_DQBUF failed: %s",
         strerror(errno));
    ret = (errno == EAGAIN) ? NCL_ErrorNotReady : NCL_ErrorHardware;
    goto EXIT;
  }

  if (buf.index >= device->bufferCount) {
    LOGE(NCL_LOG_TAG2,
         "[NCL_V4L2_DequeueBuffer] - driver returned out-of-range index %u",
         buf.index);
    ret = NCL_ErrorUndefined;
    goto EXIT;
  }

  *index = buf.index;
  *data = (NCL_U8 *)device->buffers[buf.index].start;
  *bytesUsed = buf.bytesused;

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_V4L2_DequeueBuffer] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_QueueBuffer(NCL_HANDLETYPE deviceHandle, NCL_U32 index) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_DEVICE *device = (NCL_V4L2_DEVICE *)deviceHandle;
  struct v4l2_buffer buf;

  if (!device || index >= device->bufferCount) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_Memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = index;

  if (xioctl(device->fd, VIDIOC_QBUF, &buf) == -1) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_QueueBuffer] - VIDIOC_QBUF(%u) failed: %s",
         index, strerror(errno));
    ret = NCL_ErrorHardware;
    goto EXIT;
  }

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_V4L2_QueueBuffer] - ret %d", ret);
  return ret;
}
