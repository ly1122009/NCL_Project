#include <linux/videodev2.h>
#include <stdio.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "V4L2_Device.h"

#define NCL_LOG_TAG2 "V4L2_DEMO"

// Most UVC webcams haven't converged auto-exposure/auto-white-balance on
// the very first frame after STREAMON — it commonly comes back black or
// badly exposed. Discard this many frames before keeping one.
#define NCL_V4L2_WARMUP_FRAMES 15

// Milestone 1: open a UVC webcam, negotiate a format, capture a frame with
// MMAP buffers, and dump it to disk. No threading, no zero-copy yet — just
// proving the core V4L2 ioctl sequence works end to end.
//
// Usage: v4l2_capture_demo [/dev/videoN] [output.raw]
// Viewing the result (raw YUYV, no container/header):
//   ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x480
//       -i output.raw output.png
int main(int argc, char *argv[]) {
  const char *devicePath = (argc > 1) ? argv[1] : "/dev/video0";
  const char *outputPath = (argc > 2) ? argv[2] : "frame.raw";
  NCL_HANDLETYPE device = NULL;
  NCL_V4L2_FORMAT format;
  NCL_U32 index = 0;
  NCL_U8 *data = NULL;
  NCL_U32 bytesUsed = 0;
  NCL_ERRORTYPE ret;
  int exitCode = 1;

  NCL_OSAL_LogInit("V4L2_DEMO", 0);
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

  ret = NCL_V4L2_RequestBuffers(device, 4);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "RequestBuffers failed: %d", ret);
    goto CLOSE;
  }

  ret = NCL_V4L2_StreamOn(device);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "StreamOn failed: %d", ret);
    goto CLOSE;
  }

  for (int warmup = 0; warmup < NCL_V4L2_WARMUP_FRAMES; warmup++) {
    ret = NCL_V4L2_DequeueBuffer(device, &index, &data, &bytesUsed);
    if (ret != NCL_ErrorNone) {
      LOGE(NCL_LOG_TAG2, "Warm-up DequeueBuffer(%d) failed: %d", warmup, ret);
      goto STREAM_OFF;
    }
    ret = NCL_V4L2_QueueBuffer(device, index);
    if (ret != NCL_ErrorNone) {
      LOGE(NCL_LOG_TAG2, "Warm-up QueueBuffer(%d) failed: %d", warmup, ret);
      goto STREAM_OFF;
    }
  }
  LOGI(NCL_LOG_TAG2, "Discarded %d warm-up frame(s), capturing the keeper",
       NCL_V4L2_WARMUP_FRAMES);

  ret = NCL_V4L2_DequeueBuffer(device, &index, &data, &bytesUsed);
  if (ret != NCL_ErrorNone) {
    LOGE(NCL_LOG_TAG2, "DequeueBuffer failed: %d", ret);
    goto STREAM_OFF;
  }
  LOGI(NCL_LOG_TAG2, "Captured frame: buffer index=%u bytes=%u", index,
       bytesUsed);

  FILE *outFile = fopen(outputPath, "wb");
  if (!outFile) {
    LOGE(NCL_LOG_TAG2, "Cannot open %s for writing", outputPath);
  } else {
    fwrite(data, 1, bytesUsed, outFile);
    fclose(outFile);
    LOGI(NCL_LOG_TAG2, "Wrote %u bytes to %s", bytesUsed, outputPath);
    exitCode = 0;
  }

  NCL_V4L2_QueueBuffer(device, index);

STREAM_OFF:
  NCL_V4L2_StreamOff(device);
CLOSE:
  NCL_V4L2_Close(device);
DEINIT:
  NCL_OSAL_LogDeinit();
  return exitCode;
}
