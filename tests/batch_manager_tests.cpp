#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

#include "metrics/MetricsRegistry.hpp"
#include "scheduler/BatchManager.hpp"

namespace {

PredictionRequest makeRequest(const std::string& model, const std::string& version) {
    return PredictionRequest{model, version, {"input", {1}, {1.0f}}};
}

} // namespace

TEST(BatchManagerTest, OneRequestFormsBatchAfterTimeout) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(10));

    queue.push(makeRequest("sentiment", "v1"));
    std::optional<PredictionBatch> batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 1U);
    EXPECT_EQ(batch->requests[0].requestId, 0U);
    EXPECT_EQ(batch->model, "sentiment");
    EXPECT_EQ(batch->version, "v1");
    EXPECT_EQ(metrics.getBatchesTotal(), 1U);
    EXPECT_EQ(metrics.getRequestsBatchedTotal(), 1U);
    EXPECT_EQ(metrics.getMaxObservedBatchSize(), 1U);
}

TEST(BatchManagerTest, MaxBatchSizeFormsFullBatch) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 3, std::chrono::milliseconds(100));

    queue.push(makeRequest("sentiment", "v1"));
    queue.push(makeRequest("sentiment", "v1"));
    queue.push(makeRequest("sentiment", "v1"));
    queue.push(makeRequest("sentiment", "v1"));

    std::optional<PredictionBatch> batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 3U);
    EXPECT_EQ(batch->requests[0].requestId, 0U);
    EXPECT_EQ(batch->requests[1].requestId, 1U);
    EXPECT_EQ(batch->requests[2].requestId, 2U);
    EXPECT_EQ(queue.size(), 1U);
}

TEST(BatchManagerTest, IncompatibleRequestStaysInRequestQueue) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(100));

    queue.push(makeRequest("sentiment", "v1"));
    queue.push(makeRequest("sentiment", "v1"));
    queue.push(makeRequest("fraud", "v1"));

    std::optional<PredictionBatch> batch = batchManager.getBatch();

    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->requests.size(), 2U);
    EXPECT_EQ(batch->model, "sentiment");
    EXPECT_EQ(batch->version, "v1");
    EXPECT_EQ(queue.size(), 1U);

    std::optional<QueuedRequest> remaining = queue.popNonBlocking();
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining->request.model, "fraud");
    EXPECT_EQ(remaining->request.version, "v1");
}

TEST(BatchManagerTest, ShutdownReturnsNoBatchAndRecordsNoBatchMetrics) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(10));

    batchManager.shutdown();
    std::optional<PredictionBatch> batch = batchManager.getBatch();

    EXPECT_FALSE(batch.has_value());
    EXPECT_EQ(metrics.getBatchesTotal(), 0U);
    EXPECT_EQ(metrics.getRequestsBatchedTotal(), 0U);
}

TEST(BatchManagerTest, ShutdownWakesBlockedGetBatch) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(100));

    std::future<std::optional<PredictionBatch>> future = std::async(std::launch::async, [&batchManager] {
        return batchManager.getBatch();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    batchManager.shutdown();

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(200)), std::future_status::ready);
    EXPECT_FALSE(future.get().has_value());
}
