extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Queue.h"
#include "V4L2_CaptureThread.h"
#include "V4L2_Device.h"
}

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

// V4L2_CaptureThread.c never calls a real V4L2 device directly — it goes
// through the NCL_V4L2_DequeueBufferFn/NCL_V4L2_QueueBufferFn function
// pointers stored on the capture handle. NCL_V4L2_CaptureThreadStart wires
// up the real hardware calls; here we wire up these fakes instead, via
// NCL_V4L2_CaptureThreadStartWithOps. That's the whole trick: production
// code and test code share the same loop logic, they just point it at
// different implementations of "talk to the device".
namespace {

struct FakeDeviceState {
  std::atomic<int> dequeueCalls{0};
  std::atomic<int> queueCalls{0};
  std::atomic<int> succeedForCalls{0}; // how many DequeueBuffer calls succeed before it starts failing
  std::atomic<NCL_U32> bytesPerFrame{100};
  std::atomic<NCL_U32> bufferCount{4};
};

FakeDeviceState g_fake;

// Real V4L2 buffers are backed by mmap; here a small static array stands
// in so each "frame" still has a real, distinct address to hand back. Its
// contents are never read by the code under test.
NCL_U8 g_fakeBuffers[NCL_V4L2_MAX_BUFFERS][16];

void ResetFake(int succeedForCalls, NCL_U32 bytesPerFrame = 100,
               NCL_U32 bufferCount = 4) {
  g_fake.dequeueCalls = 0;
  g_fake.queueCalls = 0;
  g_fake.succeedForCalls = succeedForCalls;
  g_fake.bytesPerFrame = bytesPerFrame;
  g_fake.bufferCount = bufferCount;
}

NCL_ERRORTYPE FakeDequeueBuffer(NCL_HANDLETYPE /*device*/, NCL_U32 *index,
                                NCL_U8 **data, NCL_U32 *bytesUsed) {
  int call = g_fake.dequeueCalls.fetch_add(1);
  if (call >= g_fake.succeedForCalls.load()) {
    return NCL_ErrorHardware; // simulate the driver going away
  }
  NCL_U32 idx = static_cast<NCL_U32>(call) % g_fake.bufferCount.load();
  *index = idx;
  *data = g_fakeBuffers[idx];
  *bytesUsed = g_fake.bytesPerFrame.load();
  return NCL_ErrorNone;
}

NCL_ERRORTYPE FakeQueueBuffer(NCL_HANDLETYPE /*device*/, NCL_U32 /*index*/) {
  g_fake.queueCalls.fetch_add(1);
  return NCL_ErrorNone;
}

} // namespace

TEST(V4L2_CaptureThread, RejectsNullOutParam) {
  NCL_HANDLETYPE frameQueue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&frameQueue, 4), NCL_ErrorNone);
  NCL_HANDLETYPE fakeDevice = reinterpret_cast<NCL_HANDLETYPE>(&g_fake);

  EXPECT_EQ(NCL_V4L2_CaptureThreadStartWithOps(nullptr, fakeDevice, frameQueue, 4,
                                               FakeDequeueBuffer, FakeQueueBuffer),
            NCL_ErrorBadParameter);

  NCL_OSAL_QueueTerminate(frameQueue);
}

// The fake fails on the very first call, so the loop should exit on its
// own — no real camera, no hardware error, but the same shutdown path a
// real "camera unplugged mid-stream" would take.
TEST(V4L2_CaptureThread, StopsCleanlyWhenDequeueFailsImmediately) {
  ResetFake(/*succeedForCalls=*/0);
  NCL_HANDLETYPE frameQueue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&frameQueue, 4), NCL_ErrorNone);
  NCL_HANDLETYPE fakeDevice = reinterpret_cast<NCL_HANDLETYPE>(&g_fake);

  NCL_HANDLETYPE capture = nullptr;
  ASSERT_EQ(NCL_V4L2_CaptureThreadStartWithOps(&capture, fakeDevice, frameQueue, 4,
                                               FakeDequeueBuffer, FakeQueueBuffer),
            NCL_ErrorNone);

  ASSERT_EQ(NCL_V4L2_CaptureThreadStop(capture), NCL_ErrorNone);
  EXPECT_GE(g_fake.dequeueCalls.load(), 1);

  NCL_OSAL_QueueTerminate(frameQueue);
}

// Drains the queue as frames arrive and checks their contents — this is
// the "happy path" the threaded demo exercises against a real camera,
// reproduced here deterministically.
TEST(V4L2_CaptureThread, PushesFramesUntilDeviceFails) {
  constexpr int kFramesToProduce = 5;
  ResetFake(kFramesToProduce, /*bytesPerFrame=*/123);
  NCL_HANDLETYPE frameQueue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&frameQueue, 8), NCL_ErrorNone);
  NCL_HANDLETYPE fakeDevice = reinterpret_cast<NCL_HANDLETYPE>(&g_fake);

  NCL_HANDLETYPE capture = nullptr;
  ASSERT_EQ(NCL_V4L2_CaptureThreadStartWithOps(&capture, fakeDevice, frameQueue, 4,
                                               FakeDequeueBuffer, FakeQueueBuffer),
            NCL_ErrorNone);

  int framesSeen = 0;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (framesSeen < kFramesToProduce &&
         std::chrono::steady_clock::now() < deadline) {
    NCL_PTR item = nullptr;
    if (NCL_OSAL_Dequeue(frameQueue, &item) == NCL_ErrorNone) {
      auto *frame = static_cast<NCL_V4L2_FRAME *>(item);
      EXPECT_EQ(frame->bytesUsed, 123u);
      framesSeen++;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }

  EXPECT_EQ(framesSeen, kFramesToProduce);
  ASSERT_EQ(NCL_V4L2_CaptureThreadStop(capture), NCL_ErrorNone);
  NCL_OSAL_QueueTerminate(frameQueue);
}

// Never drains the queue, so it fills up (capacity 1) almost immediately.
// This is the one path a real camera can't reliably reproduce on demand:
// the capture thread must notice OSAL_Enqueue failing and requeue the
// buffer itself via queueFn, instead of blocking or leaking it.
TEST(V4L2_CaptureThread, DropsAndRequeuesWhenFrameQueueIsFull) {
  ResetFake(/*succeedForCalls=*/50);
  NCL_HANDLETYPE frameQueue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&frameQueue, 1), NCL_ErrorNone);
  NCL_HANDLETYPE fakeDevice = reinterpret_cast<NCL_HANDLETYPE>(&g_fake);

  NCL_HANDLETYPE capture = nullptr;
  ASSERT_EQ(NCL_V4L2_CaptureThreadStartWithOps(&capture, fakeDevice, frameQueue, 4,
                                               FakeDequeueBuffer, FakeQueueBuffer),
            NCL_ErrorNone);

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (g_fake.queueCalls.load() == 0 &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  EXPECT_GT(g_fake.queueCalls.load(), 0)
      << "capture thread never returned a buffer on its own — the "
         "queue-full drop path did not run";

  ASSERT_EQ(NCL_V4L2_CaptureThreadStop(capture), NCL_ErrorNone);
  NCL_OSAL_QueueTerminate(frameQueue);
}
