#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "model/ModelRegistry.hpp"
#include "scheduler/WorkerPool.hpp"

namespace {

class TestRuntime final : public ModelRuntime {
public:
    PredictionResponse predict(const PredictionRequest& request) override
    {
        return PredictionResponse{request.model, request.version, "positive", 0.95, 1.0};
    }
};

std::shared_ptr<ModelRegistry> makeRegistryWithRuntime()
{
    auto registry = std::make_shared<ModelRegistry>();
    const bool added = registry->addModel("sentiment", "v1", "test-path", std::make_unique<TestRuntime>());
    EXPECT_TRUE(added);
    return registry;
}

} // namespace

TEST(WorkerPoolTest, ProcessesQueuedPredictionRequest)
{
    auto queue = std::make_shared<RequestQueue>();
    auto registry = makeRegistryWithRuntime();
    WorkerPool pool(1, queue, registry);

    pool.start();
    std::future<PredictionResponse> future = queue->push(PredictionRequest{"sentiment", "v1", "great"});

    const PredictionResponse response = future.get();
    EXPECT_EQ(response.model, "sentiment");
    EXPECT_EQ(response.version, "v1");
    EXPECT_EQ(response.prediction, "positive");
    EXPECT_DOUBLE_EQ(response.confidence, 0.95);
    EXPECT_DOUBLE_EQ(response.latency_ms, 1.0);

    pool.stop();
}

TEST(WorkerPoolTest, MissingRuntimeCompletesFutureWithException)
{
    auto queue = std::make_shared<RequestQueue>();
    auto registry = std::make_shared<ModelRegistry>();
    WorkerPool pool(1, queue, registry);

    pool.start();
    std::future<PredictionResponse> future = queue->push(PredictionRequest{"missing", "v1", "input"});

    EXPECT_THROW(future.get(), std::runtime_error);

    pool.stop();
}
