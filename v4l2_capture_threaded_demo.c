#include <linux/videodev2.h>
#include <stdio.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Queue.h"
#include "OSAL_Thread.h"
#include "V4L2_CaptureThread.h"
#include "V4L2_Device.h"

#define NCL_LOG_TAG2 "V4L2_THREADED_DEMO"
#define NCL_FRAME_QUEUE_CAPACITY 4
#define NCL_FRAMES_TO_CAPTURE 30

// Milestone 2: capture runs on its own thread (V4L2_CaptureThread), handing
// each frame to this (main/consumer) thread through OSAL_Queue. The
// consumer is the one that calls NCL_V4L2_QueueBuffer once it is done
// reading a frame — see the ownership note in V4L2_CaptureThread.h.
//
// Usage: v4l2_capture_threaded_demo [/dev/videoN] [output.raw]
int main(int argc, char *argv[]) {
  const char *devicePath = (argc > 1) ? argv[1] : "/dev/video0";
  const char *outputPath = (argc > 2) ? argv[2] : "frame_threaded.raw";
  NCL_HANDLETYPE device = NULL;
  NCL_HANDLETYPE frameQueue = NULL;
  NCL_HANDLETYPE captureThread = NULL;
  NCL_V4L2_FORMAT format;
  NCL_ERRORTYPE ret;
  NCL_U32 framesProcessed = 0;
  int exitCode = 1;

  NCL_OSAL_LogInit("V4L2_THREADED_DEMO", 0);
  NCL_OSAL_LogSetLevel(LOG_VERBOSE);

  ret = NCL_V4L2_Open(&device, devicePath);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "Open(%s) failed: %d", devicePath, ret);
    goto DEINIT;
  }

  format.width = 640;
  format.height = 480;
  format.pixelFormat = V4L2_PIX_FMT_YUYV;
  ret = NCL_V4L2_SetFormat(device, &format);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "SetFormat failed: %d", ret);
    goto CLOSE;
  }
  LOGI(NCL_LOG_TAG2, "Negotiated format: %ux%u fourcc=0x%08x", format.width,
       format.height, format.pixelFormat);

  ret = NCL_V4L2_RequestBuffers(device, NCL_FRAME_QUEUE_CAPACITY);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "RequestBuffers failed: %d", ret);
    goto CLOSE;
  }

  ret = NCL_OSAL_QueueCreate(&frameQueue, NCL_FRAME_QUEUE_CAPACITY);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "QueueCreate failed: %d", ret);
    goto CLOSE;
  }

  ret = NCL_V4L2_StreamOn(device);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "StreamOn failed: %d", ret);
    goto TERMINATE_QUEUE;
  }

  ret = NCL_V4L2_CaptureThreadStart(&captureThread, device, frameQueue,
                                    NCL_FRAME_QUEUE_CAPACITY);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "CaptureThreadStart failed: %d", ret);
    goto STREAM_OFF;
  }

  while (framesProcessed < NCL_FRAMES_TO_CAPTURE) {
    NCL_PTR item = NULL;
    if (NCL_OSAL_Dequeue(frameQueue, &item) != NCL_ErrorNone) {
      NCL_OSAL_SleepMillisec(5);
      continue;
    }

    NCL_V4L2_FRAME *frame = (NCL_V4L2_FRAME *)item;
    framesProcessed++;
    LOGI(NCL_LOG_TAG2, "Frame #%u: buffer index=%u bytes=%u", framesProcessed,
         frame->index, frame->bytesUsed);

    if (framesProcessed == NCL_FRAMES_TO_CAPTURE) {
      FILE *outFile = fopen(outputPath, "wb");
      if (!outFile) {
        LOGE(NCL_LOG_TAG2, "Cannot open %s for writing", outputPath);
      } else {
        fwrite(frame->data, 1, frame->bytesUsed, outFile);
        fclose(outFile);
        LOGI(NCL_LOG_TAG2, "Wrote %u bytes to %s", frame->bytesUsed, outputPath);
        exitCode = 0;
      }
    }

    NCL_V4L2_QueueBuffer(device, frame->index);
  }

  NCL_V4L2_CaptureThreadStop(captureThread);
STREAM_OFF:
  NCL_V4L2_StreamOff(device);
TERMINATE_QUEUE:
  NCL_OSAL_QueueTerminate(frameQueue);
CLOSE:
  NCL_V4L2_Close(device);
DEINIT:
  NCL_OSAL_LogDeinit();
  return exitCode;
}
