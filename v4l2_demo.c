#include <stdio.h>
#include <linux/videodev2.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "V4L2_Capture.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  NCL_HANDLETYPE capture = NULL;
  NCL_ERRORTYPE ret;
  NCL_PTR frameData = NULL;
  NCL_U32 frameSize = 0;

  ret = NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 1920, 1080,
                               V4L2_PIX_FMT_YUYV);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_V4L2_CaptureCreate failed: %d\n", ret);
    return 1;
  }

  ret = NCL_V4L2_CaptureStart(capture);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_V4L2_CaptureStart failed: %d\n", ret);
    NCL_V4L2_CaptureTerminate(capture);
    return 1;
  }

  ret = NCL_V4L2_CaptureGetFrame(capture, &frameData, &frameSize);
  if (ret != NCL_ErrorNone) {
    printf("ERROR: NCL_V4L2_CaptureGetFrame failed: %d\n", ret);
    NCL_V4L2_CaptureStop(capture);
    NCL_V4L2_CaptureTerminate(capture);
    return 1;
  }
  printf("Captured frame: %u bytes\n", frameSize);

  FILE *out = fopen("frame.raw", "wb");
  if (out) {
    fwrite(frameData, 1, frameSize, out);
    fclose(out);
  } else {
    printf("ERROR: fopen frame.raw failed\n");
  }

  NCL_V4L2_CaptureStop(capture);
  NCL_V4L2_CaptureTerminate(capture);

  printf("Done. Wrote frame.raw (%u bytes)\n", frameSize);

  return 0;
}
