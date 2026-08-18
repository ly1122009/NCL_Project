#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Event.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "OSAL_Queue.h"
#include "OSAL_Thread.h"
#include "V4L2_CaptureThread.h"
#include "V4L2_Device.h"

#define NCL_LOG_TAG "NCL_TAG"
#define NCL_LOG_TAG2 "V4L2_CAP_THREAD"

typedef struct _NCL_V4L2_CAPTURE_THREAD {
  NCL_HANDLETYPE device;    // not owned
  NCL_HANDLETYPE frameQueue; // not owned
  NCL_HANDLETYPE thread;
  NCL_HANDLETYPE stopEvent;
  NCL_U32 bufferCount;
  NCL_V4L2_DequeueBufferFn dequeueFn;
  NCL_V4L2_QueueBufferFn queueFn;
  NCL_V4L2_FRAME frames[NCL_V4L2_MAX_BUFFERS]; // pre-allocated, indexed by buffer index
} NCL_V4L2_CAPTURE_THREAD;

static void *CaptureThreadLoop(void *arg) {
  NCL_V4L2_CAPTURE_THREAD *capture = (NCL_V4L2_CAPTURE_THREAD *)arg;

  for (;;) {
    NCL_U32 index = 0;
    NCL_U8 *data = NULL;
    NCL_U32 bytesUsed = 0;

    NCL_ERRORTYPE ret =
        capture->dequeueFn(capture->device, &index, &data, &bytesUsed);
    if (ret != NCL_ErrorNone) {
      LOGE(NCL_LOG_TAG2, "[CaptureThreadLoop] - DequeueBuffer failed: %d", ret);
      break;
    }

    if (index < capture->bufferCount) {
      capture->frames[index].index = index;
      capture->frames[index].data = data;
      capture->frames[index].bytesUsed = bytesUsed;

      if (NCL_OSAL_Enqueue(capture->frameQueue, &capture->frames[index]) !=
          NCL_ErrorNone) {
        LOGW(NCL_LOG_TAG2,
             "[CaptureThreadLoop] - frame queue full, dropping frame (index=%u)",
             index);
        capture->queueFn(capture->device, index);
      }
    } else {
      LOGE(NCL_LOG_TAG2,
           "[CaptureThreadLoop] - driver returned out-of-range index %u",
           index);
    }

    if (NCL_OSAL_SignalWait(capture->stopEvent, 0) == NCL_ErrorNone) {
      LOGI(NCL_LOG_TAG2, "[CaptureThreadLoop] - stop requested, exiting");
      break;
    }
  }

  return NULL;
}

NCL_ERRORTYPE NCL_V4L2_CaptureThreadStartWithOps(
    NCL_HANDLETYPE *captureHandle, NCL_HANDLETYPE device,
    NCL_HANDLETYPE frameQueue, NCL_U32 bufferCount,
    NCL_V4L2_DequeueBufferFn dequeueFn, NCL_V4L2_QueueBufferFn queueFn) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE_THREAD *capture = NULL;

  if (!captureHandle || !device || !frameQueue || !dequeueFn || !queueFn ||
      bufferCount == 0 || bufferCount > NCL_V4L2_MAX_BUFFERS) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  capture =
      (NCL_V4L2_CAPTURE_THREAD *)NCL_OSAL_Malloc(sizeof(NCL_V4L2_CAPTURE_THREAD));
  if (!capture) {
    ret = NCL_ErrorInsufficientResources;
    goto EXIT;
  }
  NCL_OSAL_Memset(capture, 0, sizeof(NCL_V4L2_CAPTURE_THREAD));
  capture->device = device;
  capture->frameQueue = frameQueue;
  capture->bufferCount = bufferCount;
  capture->dequeueFn = dequeueFn;
  capture->queueFn = queueFn;

  ret = NCL_OSAL_SignalCreate(&capture->stopEvent);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureThreadStart] - SignalCreate failed: %d",
         ret);
    goto FREE_CAPTURE;
  }

  ret = NCL_OSAL_ThreadCreate(&capture->thread, (NCL_PTR)CaptureThreadLoop,
                              capture, NCL_THREAD_JOINABLE);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "[NCL_V4L2_CaptureThreadStart] - ThreadCreate failed: %d",
         ret);
    goto TERMINATE_EVENT;
  }

  *captureHandle = (NCL_HANDLETYPE)capture;
  goto EXIT;

TERMINATE_EVENT:
  NCL_OSAL_SignalTerminate(capture->stopEvent);
FREE_CAPTURE:
  NCL_OSAL_Free(capture);
EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureThreadStart] - ret %d", ret);
  return ret;
}

NCL_ERRORTYPE NCL_V4L2_CaptureThreadStart(NCL_HANDLETYPE *captureHandle,
                                          NCL_HANDLETYPE device,
                                          NCL_HANDLETYPE frameQueue,
                                          NCL_U32 bufferCount) {
  return NCL_V4L2_CaptureThreadStartWithOps(captureHandle, device, frameQueue,
                                            bufferCount, NCL_V4L2_DequeueBuffer,
                                            NCL_V4L2_QueueBuffer);
}

NCL_ERRORTYPE NCL_V4L2_CaptureThreadStop(NCL_HANDLETYPE captureHandle) {
  NCL_ERRORTYPE ret = NCL_ErrorNone;
  NCL_V4L2_CAPTURE_THREAD *capture = (NCL_V4L2_CAPTURE_THREAD *)captureHandle;

  if (!capture) {
    ret = NCL_ErrorBadParameter;
    goto EXIT;
  }

  NCL_OSAL_SignalSet(capture->stopEvent);
  NCL_OSAL_ThreadTerminate(capture->thread);
  NCL_OSAL_SignalTerminate(capture->stopEvent);
  NCL_OSAL_Free(capture);

EXIT:
  LOGI(NCL_LOG_TAG2, "[NCL_V4L2_CaptureThreadStop] - ret %d", ret);
  return ret;
}
