#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stdexcept>

#include "scheduler/RequestQueue.hpp"

using namespace std::chrono_literals;

namespace {

PredictionRequest makeRequest(const std::string& model, const std::string& version) {
    return PredictionRequest{model, version, {"input", {1}, {1.0f}}};
}

} // namespace

TEST(RequestQueueTest, PushAndPopTransfersRequestAndCompletesFuture) {
    RequestQueue queue;
    PredictionRequest request = makeRequest("sentiment", "v1");

    std::future<PredictionResponse> future = queue.push(request);
    std::optional<QueuedRequest> queued = queue.popBlocking();

    ASSERT_TRUE(queued.has_value());
    EXPECT_EQ(queued->requestId, 0U);
    EXPECT_EQ(queued->request.model, "sentiment");
    EXPECT_EQ(queued->request.version, "v1");
    EXPECT_EQ(queued->request.input.name, "input");
    EXPECT_EQ(queued->request.input.shape, std::vector<std::int64_t>({1}));
    EXPECT_EQ(queued->request.input.data, std::vector<float>({1.0f}));

    queued->resultPromise.set_value(PredictionResponse{"sentiment", "v1", {"output", {1}, {0.9f}}, 1.5});

    const PredictionResponse response = future.get();
    EXPECT_EQ(response.model, "sentiment");
    EXPECT_EQ(response.version, "v1");
    EXPECT_EQ(response.output.name, "output");
    EXPECT_EQ(response.output.data, std::vector<float>({0.9f}));
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
    std::future<PredictionResponse> future = queue.push(makeRequest("sentiment", "v1"));

    queue.setShutdown();

    std::optional<QueuedRequest> queued = queue.popBlocking();
    ASSERT_TRUE(queued.has_value());
    queued->resultPromise.set_value(PredictionResponse{"sentiment", "v1", {"output", {1}, {0.8f}}, 2.0});

    EXPECT_EQ(future.get().output.data, std::vector<float>({0.8f}));
    EXPECT_FALSE(queue.popBlocking().has_value());
}

TEST(RequestQueueTest, PushAfterShutdownReturnsExceptionalFuture) {
    RequestQueue queue;
    queue.setShutdown();

    std::future<PredictionResponse> future = queue.push(makeRequest("sentiment", "v1"));

    EXPECT_THROW(future.get(), std::runtime_error);
}
