#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <stdexcept>

#include "metrics/MetricsRegistry.hpp"
#include "model/ModelRegistry.hpp"
#include "scheduler/BatchManager.hpp"
#include "scheduler/WorkerPool.hpp"

namespace {

class TestRuntime final : public ModelRuntime {
public:
    PredictionResponse predict(const PredictionRequest& request) override {
        return PredictionResponse{request.model, request.version, {"output", {1}, {0.95f}}};
    }
};

class ThrowingRuntime final : public ModelRuntime {
public:
    PredictionResponse predict(const PredictionRequest&) override {
        throw std::runtime_error("prediction failed");
    }
};

PredictionRequest makeRequest(const std::string& model, const std::string& version) {
    return PredictionRequest{model, version, {"input", {1}, {1.0f}}};
}

} // namespace

TEST(WorkerPoolTest, ProcessesQueuedPredictionRequestAndRecordsWorkerMetrics) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(1));
    ModelRegistry registry;
    ASSERT_TRUE(registry.addModel("sentiment", "v1", "test-path", std::make_unique<TestRuntime>()));
    WorkerPool pool(1, batchManager, registry, metrics);

    pool.start();
    std::future<PredictionResponse> future =
        queue.push(makeRequest("sentiment", "v1"));

    const PredictionResponse response = future.get();
    EXPECT_EQ(response.model, "sentiment");
    EXPECT_EQ(response.version, "v1");
    EXPECT_EQ(response.output.name, "output");
    EXPECT_EQ(response.output.data, std::vector<float>({0.95f}));
    EXPECT_EQ(metrics.getPredictionsTotal(), 1U);
    EXPECT_EQ(metrics.getBatchesTotal(), 1U);
    EXPECT_EQ(metrics.getRequestsBatchedTotal(), 1U);
    EXPECT_GE(metrics.getAverageInferenceLatencyMs(), 0.0);
    EXPECT_GE(metrics.getAverageQueueWaitMs(), 0.0);

    pool.stop();
}

TEST(WorkerPoolTest, RuntimeFailureCompletesEveryFutureAndRecordsOneRuntimeError) {
    RequestQueue queue;
    MetricsRegistry metrics;
    BatchManager batchManager(queue, metrics, 8, std::chrono::milliseconds(1));
    ModelRegistry registry;
    ASSERT_TRUE(registry.addModel("sentiment", "v1", "test-path", std::make_unique<ThrowingRuntime>()));
    WorkerPool pool(1, batchManager, registry, metrics);

    pool.start();
    std::future<PredictionResponse> future =
        queue.push(makeRequest("sentiment", "v1"));

    EXPECT_THROW(future.get(), std::runtime_error);
    EXPECT_EQ(metrics.getRuntimeErrorsTotal(), 1U);
    EXPECT_EQ(metrics.getPredictionsTotal(), 0U);

    pool.stop();
}
