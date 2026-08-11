#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stdexcept>

#include "scheduler/RequestQueue.hpp"

using namespace std::chrono_literals;

TEST(RequestQueueTest, PushAndPopTransfersRequestAndCompletesFuture) {
    RequestQueue queue;
    PredictionRequest request{"sentiment", "v1", "great"};

    std::future<PredictionResponse> future = queue.push(request);
    std::optional<QueuedRequest> queued = queue.popBlocking();

    ASSERT_TRUE(queued.has_value());
    EXPECT_EQ(queued->requestId, 0U);
    EXPECT_EQ(queued->request.model, "sentiment");
    EXPECT_EQ(queued->request.version, "v1");
    EXPECT_EQ(queued->request.input, "great");

    queued->resultPromise.set_value(PredictionResponse{"sentiment", "v1", "positive", 0.9, 1.5});

    const PredictionResponse response = future.get();
    EXPECT_EQ(response.model, "sentiment");
    EXPECT_EQ(response.version, "v1");
    EXPECT_EQ(response.prediction, "positive");
    EXPECT_DOUBLE_EQ(response.confidence, 0.9);
    EXPECT_DOUBLE_EQ(response.inference_latency_ms, 1.5);
}

TEST(RequestQueueTest, ShutdownWakesBlockedPop) {
    RequestQueue queue;

    std::future<std::optional<QueuedRequest>> popFuture = std::async(std::launch::async, [&queue] {
        return queue.popBlocking();
    });

    queue.setShutdown();

    ASSERT_EQ(popFuture.wait_for(1s), std::future_status::ready);
    EXPECT_FALSE(popFuture.get().has_value());
}

TEST(RequestQueueTest, ShutdownDrainsQueuedRequestsThenStops) {
    RequestQueue queue;
    std::future<PredictionResponse> future = queue.push(PredictionRequest{"sentiment", "v1", "bad"});

    queue.setShutdown();

    std::optional<QueuedRequest> queued = queue.popBlocking();
    ASSERT_TRUE(queued.has_value());
    queued->resultPromise.set_value(PredictionResponse{"sentiment", "v1", "negative", 0.8, 2.0});

    EXPECT_EQ(future.get().prediction, "negative");
    EXPECT_FALSE(queue.popBlocking().has_value());
}

TEST(RequestQueueTest, PushAfterShutdownReturnsExceptionalFuture) {
    RequestQueue queue;
    queue.setShutdown();

    std::future<PredictionResponse> future = queue.push(PredictionRequest{"sentiment", "v1", "great"});

    EXPECT_THROW(future.get(), std::runtime_error);
}
