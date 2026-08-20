extern "C" {
#include "NCL_Core.h"
#include "NCL_Types.h"
#include "OSAL_Queue.h"
}

#include <gtest/gtest.h>

#include <atomic>
#include <numeric>
#include <thread>
#include <vector>

TEST(OSAL_Queue, EnqueueDequeueSingleThread) {
  NCL_HANDLETYPE queue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&queue, 8), NCL_ErrorNone);

  int value = 42;
  ASSERT_EQ(NCL_OSAL_Enqueue(queue, &value), NCL_ErrorNone);

  NCL_PTR out = nullptr;
  ASSERT_EQ(NCL_OSAL_Dequeue(queue, &out), NCL_ErrorNone);
  EXPECT_EQ(*static_cast<int *>(out), 42);

  ASSERT_EQ(NCL_OSAL_QueueTerminate(queue), NCL_ErrorNone);
}

// Two producer threads and one consumer thread share a single queue.
// This is the exact usage pattern OSAL_Queue is meant for (e.g. a capture
// thread handing frames to a processing thread), so it doubles as a
// regression test for whatever fix you land.
TEST(OSAL_Queue, ConcurrentProducerConsumer) {
  NCL_HANDLETYPE queue = nullptr;
  ASSERT_EQ(NCL_OSAL_QueueCreate(&queue, 16), NCL_ErrorNone);

  constexpr int kProducers = 2;
  constexpr int kItemsPerProducer = 200;
  std::vector<int> items(kProducers * kItemsPerProducer);
  std::iota(items.begin(), items.end(), 0);

  std::atomic<int> consumedCount{0};
  std::atomic<long long> consumedSum{0};

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p]() {
      for (int i = 0; i < kItemsPerProducer; ++i) {
        int *item = &items[p * kItemsPerProducer + i];
        while (NCL_OSAL_Enqueue(queue, item) != NCL_ErrorNone) {
          std::this_thread::yield();  // queue full, backpressure
        }
      }
    });
  }

  std::thread consumer([&]() {
    while (consumedCount.load() < kProducers * kItemsPerProducer) {
      NCL_PTR out = nullptr;
      if (NCL_OSAL_Dequeue(queue, &out) == NCL_ErrorNone) {
        consumedSum += *static_cast<int *>(out);
        ++consumedCount;
      } else {
        std::this_thread::yield();  // queue empty, retry
      }
    }
  });

  for (auto &t : producers) t.join();
  consumer.join();

  long long expectedSum =
      std::accumulate(items.begin(), items.end(), 0LL);
  EXPECT_EQ(consumedSum.load(), expectedSum);

  ASSERT_EQ(NCL_OSAL_QueueTerminate(queue), NCL_ErrorNone);
}
