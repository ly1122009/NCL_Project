extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "V4L2_Capture.h"
}

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace {

constexpr int kFakeFd = 42;
constexpr NCL_U32 kNegotiatedWidth = 640;
constexpr NCL_U32 kNegotiatedHeight = 480;
constexpr NCL_U32 kFrameBytes = kNegotiatedWidth * kNegotiatedHeight * 2;  // YUYV = 2B/px

// Mutable state for the fake V4L2 device, reset before every test.
struct FakeDeviceState {
  bool openShouldFail = false;
  bool capabilitiesOk = true;
  int streamOnCount = 0;
  int qbufCount = 0;
  int dqbufCount = 0;
  NCL_U8 fillByte = 0xAB;
  void *mmapBuffer = nullptr;
};

FakeDeviceState g_state;

extern "C" int fake_open(const char *path, int flags) {
  (void)flags;
  EXPECT_STREQ(path, "/dev/video0");
  if (g_state.openShouldFail) {
    errno = ENOENT;
    return -1;
  }
  return kFakeFd;
}

extern "C" int fake_ioctl(int fd, unsigned long request, void *arg) {
  EXPECT_EQ(fd, kFakeFd);
  switch (request) {
    case VIDIOC_QUERYCAP: {
      auto *cap = static_cast<struct v4l2_capability *>(arg);
      std::memset(cap, 0, sizeof(*cap));
      std::strcpy(reinterpret_cast<char *>(cap->driver), "fake-uvc");
      std::strcpy(reinterpret_cast<char *>(cap->card), "Fake Camera");
      cap->capabilities = g_state.capabilitiesOk
                              ? (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING)
                              : 0;
      return 0;
    }
    case VIDIOC_S_FMT: {
      // Simulate the driver negotiating down, like the real camera does.
      auto *fmt = static_cast<struct v4l2_format *>(arg);
      fmt->fmt.pix.width = kNegotiatedWidth;
      fmt->fmt.pix.height = kNegotiatedHeight;
      return 0;
    }
    case VIDIOC_REQBUFS: {
      auto *req = static_cast<struct v4l2_requestbuffers *>(arg);
      req->count = 1;
      return 0;
    }
    case VIDIOC_QUERYBUF: {
      auto *buf = static_cast<struct v4l2_buffer *>(arg);
      buf->length = kFrameBytes;
      buf->m.offset = 0;
      return 0;
    }
    case VIDIOC_QBUF:
      g_state.qbufCount++;
      return 0;
    case VIDIOC_STREAMON:
      g_state.streamOnCount++;
      return 0;
    case VIDIOC_DQBUF: {
      auto *buf = static_cast<struct v4l2_buffer *>(arg);
      g_state.dqbufCount++;
      buf->bytesused = kFrameBytes;
      if (g_state.mmapBuffer) {
        std::memset(g_state.mmapBuffer, g_state.fillByte, kFrameBytes);
      }
      return 0;
    }
    case VIDIOC_STREAMOFF:
      g_state.streamOnCount--;
      return 0;
    default:
      ADD_FAILURE() << "unexpected ioctl request " << request;
      return -1;
  }
}

extern "C" void *fake_mmap(void *addr, size_t length, int prot, int flags,
                           int fd, off_t offset) {
  (void)addr;
  (void)prot;
  (void)flags;
  (void)offset;
  EXPECT_EQ(fd, kFakeFd);
  g_state.mmapBuffer = std::malloc(length);
  return g_state.mmapBuffer;
}

extern "C" int fake_munmap(void *addr, size_t length) {
  (void)length;
  std::free(addr);
  if (addr == g_state.mmapBuffer) {
    g_state.mmapBuffer = nullptr;
  }
  return 0;
}

extern "C" int fake_close(int fd) {
  EXPECT_EQ(fd, kFakeFd);
  return 0;
}

const NCL_V4L2_SYSCALLS kFakeSyscalls = {
    fake_open, fake_ioctl, fake_mmap, fake_munmap, fake_close,
};

class V4L2CaptureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_state = FakeDeviceState{};
    NCL_V4L2_SetSyscalls(&kFakeSyscalls);
  }
  void TearDown() override { NCL_V4L2_SetSyscalls(nullptr); }
};

}  // namespace

TEST_F(V4L2CaptureTest, FullLifecycleSucceeds) {
  NCL_HANDLETYPE capture = nullptr;
  ASSERT_EQ(NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 1920, 1080,
                                   V4L2_PIX_FMT_YUYV),
            NCL_ErrorNone);
  ASSERT_NE(capture, nullptr);
  EXPECT_EQ(g_state.qbufCount, 1);  // Create queues the buffer once, ready to stream

  ASSERT_EQ(NCL_V4L2_CaptureStart(capture), NCL_ErrorNone);
  EXPECT_EQ(g_state.streamOnCount, 1);

  NCL_PTR data = nullptr;
  NCL_U32 size = 0;
  ASSERT_EQ(NCL_V4L2_CaptureGetFrame(capture, &data, &size), NCL_ErrorNone);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(size, kFrameBytes);
  EXPECT_EQ(static_cast<NCL_U8 *>(data)[0], g_state.fillByte);

  ASSERT_EQ(NCL_V4L2_CaptureReleaseFrame(capture), NCL_ErrorNone);
  EXPECT_EQ(g_state.qbufCount, 2);

  ASSERT_EQ(NCL_V4L2_CaptureStop(capture), NCL_ErrorNone);
  EXPECT_EQ(g_state.streamOnCount, 0);

  ASSERT_EQ(NCL_V4L2_CaptureTerminate(capture), NCL_ErrorNone);
  EXPECT_EQ(g_state.mmapBuffer, nullptr);  // Terminate must munmap
}

// Matches how the step-3 capture thread actually drives the module: repeated
// GetFrame/ReleaseFrame cycles on the same single buffer.
TEST_F(V4L2CaptureTest, RepeatedGetFrameReleaseFrameCycles) {
  NCL_HANDLETYPE capture = nullptr;
  ASSERT_EQ(NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 640, 480,
                                   V4L2_PIX_FMT_YUYV),
            NCL_ErrorNone);
  ASSERT_EQ(NCL_V4L2_CaptureStart(capture), NCL_ErrorNone);

  for (int i = 0; i < 5; ++i) {
    NCL_PTR data = nullptr;
    NCL_U32 size = 0;
    ASSERT_EQ(NCL_V4L2_CaptureGetFrame(capture, &data, &size), NCL_ErrorNone);
    EXPECT_EQ(size, kFrameBytes);
    ASSERT_EQ(NCL_V4L2_CaptureReleaseFrame(capture), NCL_ErrorNone);
  }
  EXPECT_EQ(g_state.dqbufCount, 5);
  EXPECT_EQ(g_state.qbufCount, 6);  // 1 from Create + 5 from ReleaseFrame

  NCL_V4L2_CaptureStop(capture);
  NCL_V4L2_CaptureTerminate(capture);
}

TEST_F(V4L2CaptureTest, CreateFailsWhenOpenFails) {
  g_state.openShouldFail = true;
  NCL_HANDLETYPE capture = nullptr;
  EXPECT_EQ(NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 640, 480,
                                   V4L2_PIX_FMT_YUYV),
            NCL_ErrorHardware);
  EXPECT_EQ(capture, nullptr);
}

TEST_F(V4L2CaptureTest, CreateFailsWhenDeviceLacksCaptureCapability) {
  g_state.capabilitiesOk = false;
  NCL_HANDLETYPE capture = nullptr;
  EXPECT_EQ(NCL_V4L2_CaptureCreate(&capture, "/dev/video0", 640, 480,
                                   V4L2_PIX_FMT_YUYV),
            NCL_ErrorHardware);
  EXPECT_EQ(capture, nullptr);
  EXPECT_EQ(g_state.qbufCount, 0);  // must bail out before ever queuing a buffer
}

TEST_F(V4L2CaptureTest, CreateRejectsNullArguments) {
  NCL_HANDLETYPE capture = nullptr;
  EXPECT_EQ(NCL_V4L2_CaptureCreate(nullptr, "/dev/video0", 640, 480,
                                   V4L2_PIX_FMT_YUYV),
            NCL_ErrorBadParameter);
  EXPECT_EQ(
      NCL_V4L2_CaptureCreate(&capture, nullptr, 640, 480, V4L2_PIX_FMT_YUYV),
      NCL_ErrorBadParameter);
}

TEST_F(V4L2CaptureTest, OperationsOnNullHandleReturnBadParameter) {
  EXPECT_EQ(NCL_V4L2_CaptureStart(nullptr), NCL_ErrorBadParameter);
  EXPECT_EQ(NCL_V4L2_CaptureReleaseFrame(nullptr), NCL_ErrorBadParameter);
  EXPECT_EQ(NCL_V4L2_CaptureStop(nullptr), NCL_ErrorBadParameter);
  EXPECT_EQ(NCL_V4L2_CaptureTerminate(nullptr), NCL_ErrorBadParameter);

  NCL_PTR data = nullptr;
  NCL_U32 size = 0;
  EXPECT_EQ(NCL_V4L2_CaptureGetFrame(nullptr, &data, &size),
            NCL_ErrorBadParameter);
}
