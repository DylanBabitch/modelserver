#include <gtest/gtest.h>

#include "scheduler/BatchManager.hpp"
#include "scheduler/RequestQueue.hpp"
#include "scheduler/QueuedRequest.hpp"
#include "request/PredictionRequest.hpp"

#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <thread>

namespace {

QueuedRequest makeQueuedRequest(
    std::uint64_t id,
    const std::string& model,
    const std::string& version,
    const std::string& input
)
{
    QueuedRequest queuedRequest;

    queuedRequest.requestId = id;
    queuedRequest.request = PredictionRequest{
        model,
        version,
        input
    };
    queuedRequest.creationTime = QueuedRequest::Clock::now();
    queuedRequest.status = RequestStatus::Queued;

    return queuedRequest;
}

} // namespace

TEST(BatchManagerTest, OneRequestFormsBatchAfterTimeout)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 8, std::chrono::milliseconds(10));

    queue.push(makeQueuedRequest(1, "sentiment", "v1", "this movie was great"));

    auto batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->requests.size(), 1);
    EXPECT_EQ(batch->requests[0].requestId, 1);
    EXPECT_EQ(batch->model, "sentiment");
    EXPECT_EQ(batch->version, "v1");
}

TEST(BatchManagerTest, MaxBatchSizeFormsFullBatch)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 3, std::chrono::milliseconds(100));

    queue.push(makeQueuedRequest(1, "sentiment", "v1", "request 1"));
    queue.push(makeQueuedRequest(2, "sentiment", "v1", "request 2"));
    queue.push(makeQueuedRequest(3, "sentiment", "v1", "request 3"));
    queue.push(makeQueuedRequest(4, "sentiment", "v1", "request 4"));

    auto batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->requests.size(), 3);

    EXPECT_EQ(batch->requests[0].requestId, 1);
    EXPECT_EQ(batch->requests[1].requestId, 2);
    EXPECT_EQ(batch->requests[2].requestId, 3);

    EXPECT_EQ(queue.size(), 1);
}

TEST(BatchManagerTest, BatchPreservesFifoOrder)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 4, std::chrono::milliseconds(100));

    queue.push(makeQueuedRequest(10, "sentiment", "v1", "first"));
    queue.push(makeQueuedRequest(20, "sentiment", "v1", "second"));
    queue.push(makeQueuedRequest(30, "sentiment", "v1", "third"));

    auto batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 3);

    EXPECT_EQ(batch->requests[0].requestId, 10);
    EXPECT_EQ(batch->requests[1].requestId, 20);
    EXPECT_EQ(batch->requests[2].requestId, 30);
}

TEST(BatchManagerTest, IncompatibleRequestStaysInRequestQueue)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 8, std::chrono::milliseconds(100));

    queue.push(makeQueuedRequest(1, "sentiment", "v1", "a1"));
    queue.push(makeQueuedRequest(2, "sentiment", "v1", "a2"));
    queue.push(makeQueuedRequest(3, "fraud", "v1", "b1"));

    auto batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 2);

    EXPECT_EQ(batch->requests[0].requestId, 1);
    EXPECT_EQ(batch->requests[1].requestId, 2);

    EXPECT_EQ(batch->model, "sentiment");
    EXPECT_EQ(batch->version, "v1");

    EXPECT_EQ(queue.size(), 1);

    auto remaining = queue.popNonBlocking();

    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining->requestId, 3);
    EXPECT_EQ(remaining->request.model, "fraud");
    EXPECT_EQ(remaining->request.version, "v1");
}

TEST(BatchManagerTest, BatchOnlyIncludesCompatibleModelVersionPairs)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 8, std::chrono::milliseconds(100));

    queue.push(makeQueuedRequest(1, "sentiment", "v1", "a1"));
    queue.push(makeQueuedRequest(2, "sentiment", "v2", "a2"));
    queue.push(makeQueuedRequest(3, "sentiment", "v1", "a3"));

    auto batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 1);

    EXPECT_EQ(batch->requests[0].requestId, 1);
    EXPECT_EQ(batch->model, "sentiment");
    EXPECT_EQ(batch->version, "v1");

    EXPECT_EQ(queue.size(), 2);

    auto next = queue.popNonBlocking();

    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->requestId, 2);
    EXPECT_EQ(next->request.model, "sentiment");
    EXPECT_EQ(next->request.version, "v2");
}

TEST(BatchManagerTest, ShutdownReturnsNoBatchWhenQueueIsEmpty)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 8, std::chrono::milliseconds(10));

    batchManager.shutdown();

    auto batch = batchManager.getBatch();

    EXPECT_FALSE(batch.has_value());
}

TEST(BatchManagerTest, ShutdownWakesBlockedGetBatch)
{
    RequestQueue queue;
    BatchManager batchManager(queue, 8, std::chrono::milliseconds(100));

    auto future = std::async(std::launch::async, [&batchManager]() {
        return batchManager.getBatch();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    batchManager.shutdown();

    auto status = future.wait_for(std::chrono::milliseconds(200));

    ASSERT_EQ(status, std::future_status::ready);

    auto batch = future.get();

    EXPECT_FALSE(batch.has_value());
}