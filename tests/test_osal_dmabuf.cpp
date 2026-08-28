extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_DmaBuf.h"
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
}

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr int kHeapFd = 50;      // fd from opening the heap device node
constexpr int kAllocatedFd = 77; // fd DMA_HEAP_IOCTL_ALLOC "returns"
constexpr int kImportedFd = 123; // an fd handed in from outside (e.g. V4L2)
constexpr int kDupedFd = 456;    // whatever fake_dup() hands back
constexpr NCL_U32 kBufSize = 4096;
constexpr char kHeapPath[] = "/fake/dma_heap/test";

// Mutable state for the fake dma-buf backend, reset before every test.
struct FakeState {
  bool heapOpenShouldFail = false;
  bool allocIoctlShouldFail = false;
  bool dupShouldFail = false;

  int closeCallCount = 0;
  int mmapCallCount = 0;
  int munmapCallCount = 0;
  int dupCallCount = 0;

  std::vector<int> closedFds;
  std::vector<__u64> syncFlagsSeen;
  void *mmapBuffer = nullptr;
};

FakeState g_state;

extern "C" int fake_open(const char *path, int flags) {
  (void)flags;
  EXPECT_STREQ(path, kHeapPath);
  if (g_state.heapOpenShouldFail) {
    errno = ENOENT;
    return -1;
  }
  return kHeapFd;
}

extern "C" int fake_ioctl(int fd, unsigned long request, void *arg) {
  switch (request) {
    case DMA_HEAP_IOCTL_ALLOC: {
      EXPECT_EQ(fd, kHeapFd);
      if (g_state.allocIoctlShouldFail) {
        errno = EINVAL;
        return -1;
      }
      auto *data = static_cast<struct dma_heap_allocation_data *>(arg);
      data->fd = kAllocatedFd;
      return 0;
    }
    case DMA_BUF_IOCTL_SYNC: {
      auto *sync = static_cast<struct dma_buf_sync *>(arg);
      g_state.syncFlagsSeen.push_back(sync->flags);
      return 0;
    }
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
  (void)fd;
  (void)offset;
  g_state.mmapCallCount++;
  EXPECT_EQ(length, kBufSize);
  g_state.mmapBuffer = std::malloc(length);
  return g_state.mmapBuffer;
}

extern "C" int fake_munmap(void *addr, size_t length) {
  (void)length;
  g_state.munmapCallCount++;
  std::free(addr);
  if (addr == g_state.mmapBuffer) {
    g_state.mmapBuffer = nullptr;
  }
  return 0;
}

extern "C" int fake_close(int fd) {
  g_state.closeCallCount++;
  g_state.closedFds.push_back(fd);
  return 0;
}

extern "C" int fake_dup(int fd) {
  (void)fd;
  g_state.dupCallCount++;
  if (g_state.dupShouldFail) {
    errno = EMFILE;
    return -1;
  }
  return kDupedFd;
}

const NCL_DMABUF_SYSCALLS kFakeSyscalls = {
    fake_open, fake_ioctl, fake_mmap, fake_munmap, fake_close, fake_dup,
};

class DmaBufTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_state = FakeState{};
    NCL_OSAL_DmaBuf_SetSyscalls(&kFakeSyscalls);
  }
  void TearDown() override { NCL_OSAL_DmaBuf_SetSyscalls(nullptr); }
};

}  // namespace

TEST_F(DmaBufTest, AllocSucceeds) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufAlloc(&buf, kHeapPath, kBufSize), NCL_ErrorNone);
  ASSERT_NE(buf, nullptr);
  // The heap device fd is only needed for the ioctl itself -- must be
  // closed right away, independent of the allocated dma-buf fd.
  ASSERT_EQ(g_state.closeCallCount, 1);
  EXPECT_EQ(g_state.closedFds[0], kHeapFd);

  NCL_U32 size = 0;
  ASSERT_EQ(NCL_OSAL_DmaBufGetSize(buf, &size), NCL_ErrorNone);
  EXPECT_EQ(size, kBufSize);

  int fd = -1;
  ASSERT_EQ(NCL_OSAL_DmaBufGetFd(buf, &fd), NCL_ErrorNone);
  EXPECT_EQ(fd, kDupedFd); // GetFd always hands back a dup, never the original

  ASSERT_EQ(NCL_OSAL_DmaBufTerminate(buf), NCL_ErrorNone);
  ASSERT_EQ(g_state.closeCallCount, 2);
  EXPECT_EQ(g_state.closedFds[1], kAllocatedFd);
}

TEST_F(DmaBufTest, AllocFailsWhenHeapOpenFails) {
  g_state.heapOpenShouldFail = true;
  NCL_HANDLETYPE buf = nullptr;
  EXPECT_EQ(NCL_OSAL_DmaBufAlloc(&buf, kHeapPath, kBufSize), NCL_ErrorHardware);
  EXPECT_EQ(buf, nullptr);
  EXPECT_EQ(g_state.closeCallCount, 0); // never got an fd to close
}

TEST_F(DmaBufTest, AllocFailsWhenIoctlFailsAndStillClosesHeapFd) {
  g_state.allocIoctlShouldFail = true;
  NCL_HANDLETYPE buf = nullptr;
  EXPECT_EQ(NCL_OSAL_DmaBufAlloc(&buf, kHeapPath, kBufSize), NCL_ErrorHardware);
  ASSERT_EQ(g_state.closeCallCount, 1); // no fd leak on the error path
  EXPECT_EQ(g_state.closedFds[0], kHeapFd);
}

TEST_F(DmaBufTest, ImportTakesOwnershipWithoutDup) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);
  EXPECT_EQ(g_state.dupCallCount, 0);

  ASSERT_EQ(NCL_OSAL_DmaBufTerminate(buf), NCL_ErrorNone);
  ASSERT_EQ(g_state.closeCallCount, 1);
  EXPECT_EQ(g_state.closedFds[0], kImportedFd);
}

TEST_F(DmaBufTest, ImportWithoutOwnershipDupsAndClosesTheDup) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_FALSE),
            NCL_ErrorNone);
  EXPECT_EQ(g_state.dupCallCount, 1);

  ASSERT_EQ(NCL_OSAL_DmaBufTerminate(buf), NCL_ErrorNone);
  ASSERT_EQ(g_state.closeCallCount, 1);
  // Closes its own dup'd copy, never the caller's original fd.
  EXPECT_EQ(g_state.closedFds[0], kDupedFd);
}

TEST_F(DmaBufTest, MapReturnsMmapPointer) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);

  NCL_PTR addr = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufMap(buf, &addr), NCL_ErrorNone);
  ASSERT_NE(addr, nullptr);
  EXPECT_EQ(g_state.mmapCallCount, 1);

  NCL_OSAL_DmaBufTerminate(buf);
}

TEST_F(DmaBufTest, MapIsIdempotent) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);

  NCL_PTR addr1 = nullptr;
  NCL_PTR addr2 = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufMap(buf, &addr1), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufMap(buf, &addr2), NCL_ErrorNone);
  EXPECT_EQ(addr1, addr2);
  EXPECT_EQ(g_state.mmapCallCount, 1); // second Map didn't call mmap again

  NCL_OSAL_DmaBufTerminate(buf);
}

TEST_F(DmaBufTest, UnmapWithoutMapIsNoop) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);
  EXPECT_EQ(NCL_OSAL_DmaBufUnmap(buf), NCL_ErrorNone);
  EXPECT_EQ(g_state.munmapCallCount, 0);
  NCL_OSAL_DmaBufTerminate(buf);
}

TEST_F(DmaBufTest, TerminateUnmapsIfStillMapped) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);
  NCL_PTR addr = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufMap(buf, &addr), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_DmaBufTerminate(buf), NCL_ErrorNone);
  EXPECT_EQ(g_state.munmapCallCount, 1);
}

TEST_F(DmaBufTest, SyncFlagsMatchKernelBitsForEveryMode) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);
  NCL_PTR addr = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufMap(buf, &addr), NCL_ErrorNone);

  ASSERT_EQ(NCL_OSAL_DmaBufSyncStart(buf, NCL_DMABUF_SYNC_READ), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufSyncEnd(buf, NCL_DMABUF_SYNC_READ), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufSyncStart(buf, NCL_DMABUF_SYNC_WRITE), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufSyncEnd(buf, NCL_DMABUF_SYNC_WRITE), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufSyncStart(buf, NCL_DMABUF_SYNC_RW), NCL_ErrorNone);
  ASSERT_EQ(NCL_OSAL_DmaBufSyncEnd(buf, NCL_DMABUF_SYNC_RW), NCL_ErrorNone);

  ASSERT_EQ(g_state.syncFlagsSeen.size(), 6u);
  EXPECT_EQ(g_state.syncFlagsSeen[0], DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ);
  EXPECT_EQ(g_state.syncFlagsSeen[1], DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ);
  EXPECT_EQ(g_state.syncFlagsSeen[2], DMA_BUF_SYNC_START | DMA_BUF_SYNC_WRITE);
  EXPECT_EQ(g_state.syncFlagsSeen[3], DMA_BUF_SYNC_END | DMA_BUF_SYNC_WRITE);
  EXPECT_EQ(g_state.syncFlagsSeen[4], DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW);
  EXPECT_EQ(g_state.syncFlagsSeen[5], DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW);

  NCL_OSAL_DmaBufTerminate(buf);
}

TEST_F(DmaBufTest, SyncBeforeMapFailsWithoutTouchingTheDevice) {
  NCL_HANDLETYPE buf = nullptr;
  ASSERT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_TRUE),
            NCL_ErrorNone);

  EXPECT_EQ(NCL_OSAL_DmaBufSyncStart(buf, NCL_DMABUF_SYNC_RW),
            NCL_ErrorInvalidState);
  EXPECT_TRUE(g_state.syncFlagsSeen.empty());

  NCL_OSAL_DmaBufTerminate(buf);
}

TEST_F(DmaBufTest, ImportFailsWhenDupFails) {
  g_state.dupShouldFail = true;
  NCL_HANDLETYPE buf = nullptr;
  EXPECT_EQ(NCL_OSAL_DmaBufImport(&buf, kImportedFd, kBufSize, NCL_FALSE),
            NCL_ErrorInsufficientResources);
  EXPECT_EQ(buf, nullptr);
}

TEST_F(DmaBufTest, TerminateNullHandleIsBadParameter) {
  EXPECT_EQ(NCL_OSAL_DmaBufTerminate(nullptr), NCL_ErrorBadParameter);
}
