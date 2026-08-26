#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "V4L2_Capture.h"

#define NCL_LOG_TAG2 "V4L2_CAPTURE"

NCL_ERRORTYPE NCL_V4L2_CaptureCreate(NCL_HANDLETYPE *captureHandle,
                                     const char *devicePath,
                                     NCL_U32 width,
                                     NCL_U32 height,
                                     NCL_U32 pixelFormat) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE *capture = NULL;
  struct v4l2_capability cap;
  struct v4l2_format fmt;
  struct v4l2_requestbuffers reqbufs;
  int ioctlRet = 0;

  if (!captureHandle || !devicePath) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  capture = (NCL_V4L2_CAPTURE *)NCL_OSAL_Malloc(sizeof(NCL_V4L2_CAPTURE));
  if (!capture) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - cannot allocate capture");
    goto EXIT;
  }
  NCL_OSAL_Memset(capture, 0, sizeof(NCL_V4L2_CAPTURE));

  capture->m_fd = open(devicePath, O_RDWR);
  if (capture->m_fd < 0) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - open %s failed: %s",
         devicePath, strerror(errno));
    goto EXIT_FREE;
  }

  memset(&cap, 0, sizeof(cap));
  ioctlRet = ioctl(capture->m_fd, VIDIOC_QUERYCAP, &cap);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - VIDIOC_QUERYCAP failed: %s",
         strerror(errno));
    goto EXIT_CLOSE;
  }
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - driver=%s card=%s bus=%s",
       cap.driver, cap.card, cap.bus_info);

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
      !(cap.capabilities & V4L2_CAP_STREAMING)) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2,
         "[NCL_V4L2_CaptureCreate] - device lacks CAPTURE/STREAMING capability");
    goto EXIT_CLOSE;
  }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = pixelFormat;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;

  ioctlRet = ioctl(capture->m_fd, VIDIOC_S_FMT, &fmt);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - VIDIOC_S_FMT failed: %s",
         strerror(errno));
    goto EXIT_CLOSE;
  }

  capture->m_width = fmt.fmt.pix.width;
  capture->m_height = fmt.fmt.pix.height;
  capture->m_pixelFormat = fmt.fmt.pix.pixelformat;
  if (capture->m_width != width || capture->m_height != height ||
      capture->m_pixelFormat != pixelFormat) {
    LOGW(NCL_LOG_TAG2,
         "[NCL_V4L2_CaptureCreate] - driver adjusted format: asked %ux%u fmt=0x%x, got %ux%u fmt=0x%x",
         width, height, pixelFormat, capture->m_width, capture->m_height,
         capture->m_pixelFormat);
  }

  memset(&reqbufs, 0, sizeof(reqbufs));
  reqbufs.count = 1;
  reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbufs.memory = V4L2_MEMORY_MMAP;

  ioctlRet = ioctl(capture->m_fd, VIDIOC_REQBUFS, &reqbufs);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - VIDIOC_REQBUFS failed: %s",
         strerror(errno));
    goto EXIT_CLOSE;
  }
  if (reqbufs.count < 1) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - driver allocated 0 buffers");
    goto EXIT_CLOSE;
  }

  memset(&capture->m_buf, 0, sizeof(capture->m_buf));
  capture->m_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  capture->m_buf.memory = V4L2_MEMORY_MMAP;
  capture->m_buf.index = 0;

  ioctlRet = ioctl(capture->m_fd, VIDIOC_QUERYBUF, &capture->m_buf);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - VIDIOC_QUERYBUF failed: %s",
         strerror(errno));
    goto EXIT_CLOSE;
  }

  capture->m_bufferLength = capture->m_buf.length;
  capture->m_bufferStart = mmap(NULL, capture->m_buf.length,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                capture->m_fd, capture->m_buf.m.offset);
  if (capture->m_bufferStart == MAP_FAILED) {
    ret = NCL_ErrorInsufficientResources;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - mmap failed: %s",
         strerror(errno));
    capture->m_bufferStart = NULL;
    goto EXIT_CLOSE;
  }

  ioctlRet = ioctl(capture->m_fd, VIDIOC_QBUF, &capture->m_buf);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - VIDIOC_QBUF failed: %s",
         strerror(errno));
    goto EXIT_UNMAP;
  }

  *captureHandle = (NCL_HANDLETYPE)capture;
  goto EXIT;

EXIT_UNMAP:
  munmap(capture->m_bufferStart, capture->m_bufferLength);
EXIT_CLOSE:
  close(capture->m_fd);
EXIT_FREE:
  NCL_OSAL_Free(capture);
EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureCreate] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_CaptureStart(NCL_HANDLETYPE captureHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE *capture = (NCL_V4L2_CAPTURE *)captureHandle;
  int ioctlRet = 0;

  if (!capture) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  ioctlRet = ioctl(capture->m_fd, VIDIOC_STREAMON, &capture->m_buf.type);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureStart] - VIDIOC_STREAMON failed: %s",
         strerror(errno));
    goto EXIT;
  }

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureStart] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_CaptureGetFrame(NCL_HANDLETYPE captureHandle,
                                       NCL_PTR *frameData,
                                       NCL_U32 *frameSize) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE *capture = (NCL_V4L2_CAPTURE *)captureHandle;
  int ioctlRet = 0;

  if (!capture || !frameData || !frameSize) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  ioctlRet = ioctl(capture->m_fd, VIDIOC_DQBUF, &capture->m_buf);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureGetFrame] - VIDIOC_DQBUF failed: %s",
         strerror(errno));
    goto EXIT;
  }

  *frameData = capture->m_bufferStart;
  *frameSize = capture->m_buf.bytesused;

EXIT:
  LOGD(NCL_LOG_TAG2, "[NCL_V4L2_CaptureGetFrame] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_CaptureStop(NCL_HANDLETYPE captureHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE *capture = (NCL_V4L2_CAPTURE *)captureHandle;
  int ioctlRet = 0;

  if (!capture) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  ioctlRet = ioctl(capture->m_fd, VIDIOC_STREAMOFF, &capture->m_buf.type);
  if (ioctlRet) {
    ret = NCL_ErrorHardware;
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureStop] - VIDIOC_STREAMOFF failed: %s",
         strerror(errno));
    goto EXIT;
  }

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureStop] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_CaptureTerminate(NCL_HANDLETYPE captureHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE *capture = (NCL_V4L2_CAPTURE *)captureHandle;

  if (!capture) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  if (capture->m_bufferStart) {
    munmap(capture->m_bufferStart, capture->m_bufferLength);
  }
  if (capture->m_fd >= 0) {
    close(capture->m_fd);
  }
  NCL_OSAL_Free(capture);

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureTerminate] - ret %d", ret);
  return ret;
}
