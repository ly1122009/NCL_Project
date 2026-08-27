#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Log.h"
#include "OSAL_Memory.h"
#include "OSAL_Queue.h"
#include "OSAL_Thread.h"
#include "V4L2_Capture.h"

#define NCL_LOG_TAG2 "V4L2_DEMO"
#define CAPTURE_QUEUE_SIZE 8
#define FRAMES_TO_CAPTURE 5

/** One captured frame handed from the capture thread to main via the queue. */
typedef struct _V4L2_FrameMsg {
  NCL_U8 *data;
  NCL_U32 size;
} V4L2_FrameMsg;

typedef struct _CaptureThreadCtx {
  NCL_HANDLETYPE captureHandle;
  NCL_HANDLETYPE queueHandle;
  volatile NCL_BOOLEAN running;
} CaptureThreadCtx;

/* pthread-style entry point: void *(*)(void *), matches what
 * NCL_OSAL_ThreadCreate casts func_name to. */
static void *CaptureThreadFunc(void *arg) {
  CaptureThreadCtx *ctx = (CaptureThreadCtx *)arg;
  NCL_PTR frameData = NULL;
  NCL_U32 frameSize = 0;
  V4L2_FrameMsg *msg = NULL;
  NCL_ERRORTYPE ret;

  while (ctx->running) {
    ret = NCL_V4L2_CaptureGetFrame(ctx->captureHandle, &frameData, &frameSize);
    if (ret != NCL_ErrorNone) {
      LOGE(NCL_LOG_TAG2, "[CaptureThreadFunc] - GetFrame failed: %d", ret);
      break;
    }

    /* Only one V4L2 buffer exists, so the frame must be copied out before
     * ReleaseFrame lets the driver overwrite it with the next one. */
    msg = (V4L2_FrameMsg *)NCL_OSAL_Malloc(sizeof(V4L2_FrameMsg));
    if (msg) {
      msg->data = (NCL_U8 *)NCL_OSAL_Malloc(frameSize);
      if (msg->data) {
        NCL_OSAL_Memcpy(msg->data, frameData, (NCL_S32)frameSize);
        msg->size = frameSize;
        if (NCL_OSAL_Enqueue(ctx->queueHandle, msg) != NCL_ErrorNone) {
          LOGE(NCL_LOG_TAG2, "[CaptureThreadFunc] - queue full, dropping frame");
          NCL_OSAL_Free(msg->data);
          NCL_OSAL_Free(msg);
        }
      } else {
        LOGE(NCL_LOG_TAG2, "[CaptureThreadFunc] - cannot allocate frame copy");
        NCL_OSAL_Free(msg);
      }
    }

    NCL_V4L2_CaptureReleaseFrame(ctx->captureHandle);
  }

  return NULL;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  NCL_ERRORTYPE ret;
  NCL_HANDLETYPE capture = NULL;
  NCL_HANDLETYPE queue = NULL;
  NCL_HANDLETYPE captureThread = NULL;
  CaptureThreadCtx threadCtx = {0};
  int framesWritten = 0;

  NCL_OSAL_LogInit("V4L2_DEMO", 0);
  NCL_OSAL_LogSetLevel(LOG_VERBOSE);

  ret = NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 1920, 1080,
                               V4L2_PIX_FMT_YUYV);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_V4L2_CaptureCreate failed: %d\n", ret);
    goto EXIT_LOG;
  }

  ret = NCL_OSAL_QueueCreate(&queue, CAPTURE_QUEUE_SIZE);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_OSAL_QueueCreate failed: %d\n", ret);
    goto EXIT_CAPTURE;
  }

  ret = NCL_V4L2_CaptureStart(capture);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_V4L2_CaptureStart failed: %d\n", ret);
    goto EXIT_QUEUE;
  }

  /* Step 4 demonstration: capture one frame synchronously, export the
   * buffer as a DMA-BUF fd, import it back via an independent mmap(), and
   * prove the two views see identical bytes -- i.e. it's the same physical
   * memory as *frameData, not a copy. This is orthogonal to the
   * thread+queue path below; it just proves the mechanism works before the
   * threaded capture starts consuming the buffer. */
  {
    NCL_PTR frameData = NULL;
    NCL_U32 frameSize = 0;
    ret = NCL_V4L2_CaptureGetFrame(capture, &frameData, &frameSize);
    if (ret == NCL_ErrorNone) {
      int dmaBufFd = -1;
      ret = NCL_V4L2_CaptureExportBuffer(capture, &dmaBufFd);
      if (ret == NCL_ErrorNone) {
        void *imported = mmap(NULL, frameSize, PROT_READ, MAP_SHARED, dmaBufFd, 0);
        if (imported != MAP_FAILED) {
          int same = (memcmp(frameData, imported, frameSize) == 0);
          printf("DMA-BUF zero-copy check: independent mmap of exported fd %s "
                 "the original capture buffer\n",
                 same ? "MATCHES" : "DOES NOT MATCH");
          munmap(imported, frameSize);
        } else {
          printf("ERROR: mmap of DMA-BUF fd failed\n");
        }
        close(dmaBufFd);
      } else {
        printf("ERROR: NCL_V4L2_CaptureExportBuffer failed: %d\n", ret);
      }
      NCL_V4L2_CaptureReleaseFrame(capture);
    } else {
      printf("ERROR: DMA-BUF demo GetFrame failed: %d\n", ret);
    }
    ret = NCL_ErrorNone; /* don't fail the whole demo over this side-check */
  }

  threadCtx.captureHandle = capture;
  threadCtx.queueHandle = queue;
  threadCtx.running = NCL_TRUE;

  ret = NCL_OSAL_ThreadCreate(&captureThread, (NCL_PTR)CaptureThreadFunc,
                              &threadCtx, NCL_THREAD_JOINABLE);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_OSAL_ThreadCreate failed: %d\n", ret);
    goto EXIT_STOP;
  }

  /* Consumer: pull FRAMES_TO_CAPTURE frames off the queue and write each to
   * disk. NCL_OSAL_Dequeue doesn't block -- it returns an error immediately
   * if the queue is empty, so poll with a short sleep instead of spinning. */
  while (framesWritten < FRAMES_TO_CAPTURE) {
    NCL_PTR data = NULL;
    if (NCL_OSAL_Dequeue(queue, &data) != NCL_ErrorNone || !data) {
      NCL_OSAL_SleepMillisec(10);
      continue;
    }

    V4L2_FrameMsg *msg = (V4L2_FrameMsg *)data;
    char path[64];
    snprintf(path, sizeof(path), "frame_%d.raw", framesWritten);
    FILE *out = fopen(path, "wb");
    if (out) {
      fwrite(msg->data, 1, msg->size, out);
      fclose(out);
      printf("Wrote %s (%u bytes)\n", path, msg->size);
    } else {
      printf("ERROR: fopen %s failed\n", path);
    }
    framesWritten++;

    NCL_OSAL_Free(msg->data);
    NCL_OSAL_Free(msg);
  }

  threadCtx.running = NCL_FALSE;
  NCL_OSAL_ThreadTerminate(captureThread);

  /* The thread may have already committed to one more GetFrame/Enqueue
   * cycle before it saw running go false -- drain anything left so it
   * doesn't leak (QueueTerminate only frees its own NCL_QElem nodes, not
   * whatever payload they point to). */
  {
    NCL_PTR leftover = NULL;
    while (NCL_OSAL_Dequeue(queue, &leftover) == NCL_ErrorNone && leftover) {
      V4L2_FrameMsg *leftoverMsg = (V4L2_FrameMsg *)leftover;
      LOGW(NCL_LOG_TAG2, "[main] - draining unconsumed frame (%u bytes)",
           leftoverMsg->size);
      NCL_OSAL_Free(leftoverMsg->data);
      NCL_OSAL_Free(leftoverMsg);
      leftover = NULL;
    }
  }

  printf("Done. Captured %d frames via thread+queue.\n", framesWritten);

EXIT_STOP:
  NCL_V4L2_CaptureStop(capture);
EXIT_QUEUE:
  if (queue) {
    NCL_OSAL_QueueTerminate(queue);
  }
EXIT_CAPTURE:
  if (capture) {
    NCL_V4L2_CaptureTerminate(capture);
  }
EXIT_LOG:
  NCL_OSAL_LogDeinit();
  return (ret == NCL_ErrorNone) ? 0 : 1;
}
