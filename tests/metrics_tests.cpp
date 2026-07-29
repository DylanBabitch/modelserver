#include <gtest/gtest.h>

#include "metrics/MetricsRegistry.hpp"

TEST(MetricsRegistryTest, StartsAtZero)
{
    MetricsRegistry metrics;

    EXPECT_EQ(metrics.getRequestsTotal(), 0);
    EXPECT_EQ(metrics.getRequestsSuccessful(), 0);
    EXPECT_EQ(metrics.getRequestsFailed(), 0);
    EXPECT_EQ(metrics.getPredictionsTotal(), 0);
    EXPECT_EQ(metrics.getModelsRegisteredTotal(), 0);

    EXPECT_DOUBLE_EQ(metrics.getAverageRequestLatency(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getAverageInferenceLatency(), 0.0);

    auto [request_p50, inference_p50] = metrics.getP50Latency();
    auto [request_p95, inference_p95] = metrics.getP95Latency();

    EXPECT_DOUBLE_EQ(request_p50, 0.0);
    EXPECT_DOUBLE_EQ(inference_p50, 0.0);
    EXPECT_DOUBLE_EQ(request_p95, 0.0);
    EXPECT_DOUBLE_EQ(inference_p95, 0.0);
}

TEST(MetricsRegistryTest, RecordsSuccessfulRequest)
{
    MetricsRegistry metrics;

    metrics.addActiveRequest();
    metrics.addSuccessfulRequest();

    EXPECT_EQ(metrics.getRequestsTotal(), 1);
    EXPECT_EQ(metrics.getRequestsSuccessful(), 1);
    EXPECT_EQ(metrics.getRequestsFailed(), 0);
}

TEST(MetricsRegistryTest, RecordsFailedRequest)
{
    MetricsRegistry metrics;

    metrics.addActiveRequest();
    metrics.addFailedRequest();

    EXPECT_EQ(metrics.getRequestsTotal(), 1);
    EXPECT_EQ(metrics.getRequestsSuccessful(), 0);
    EXPECT_EQ(metrics.getRequestsFailed(), 1);
}

TEST(MetricsRegistryTest, RecordsPrediction)
{
    MetricsRegistry metrics;

    metrics.addPrediction(10.0);

    EXPECT_EQ(metrics.getPredictionsTotal(), 1);
    EXPECT_DOUBLE_EQ(metrics.getAverageRequestLatency(), 10.0);
}

TEST(MetricsRegistryTest, ComputesAverageRequestLatency)
{
    MetricsRegistry metrics;

    metrics.addPrediction(10.0);
    metrics.addPrediction(20.0);
    metrics.addPrediction(30.0);

    EXPECT_EQ(metrics.getPredictionsTotal(), 3);
    EXPECT_DOUBLE_EQ(metrics.getAverageRequestLatency(), 20.0);
}

TEST(MetricsRegistryTest, ComputesAverageInferenceLatency)
{
    MetricsRegistry metrics;

    metrics.addInferenceLatency(2.0);
    metrics.addInferenceLatency(4.0);
    metrics.addInferenceLatency(6.0);

    EXPECT_DOUBLE_EQ(metrics.getAverageInferenceLatency(), 4.0);
}

TEST(MetricsRegistryTest, RecordsRegisteredModels)
{
    MetricsRegistry metrics;

    metrics.addRegisteredModel();
    metrics.addRegisteredModel();

    EXPECT_EQ(metrics.getModelsRegisteredTotal(), 2);
}

TEST(MetricsRegistryTest, ComputesP50LatencyForOddCount)
{
    MetricsRegistry metrics;

    metrics.addPrediction(10.0);
    metrics.addPrediction(20.0);
    metrics.addPrediction(30.0);

    metrics.addInferenceLatency(1.0);
    metrics.addInferenceLatency(2.0);
    metrics.addInferenceLatency(3.0);

    auto [request_p50, inference_p50] = metrics.getP50Latency();

    EXPECT_DOUBLE_EQ(request_p50, 20.0);
    EXPECT_DOUBLE_EQ(inference_p50, 2.0);
}

TEST(MetricsRegistryTest, ComputesP95Latency)
{
    MetricsRegistry metrics;

    metrics.addPrediction(10.0);
    metrics.addPrediction(20.0);
    metrics.addPrediction(30.0);
    metrics.addPrediction(40.0);
    metrics.addPrediction(50.0);

    metrics.addInferenceLatency(1.0);
    metrics.addInferenceLatency(2.0);
    metrics.addInferenceLatency(3.0);
    metrics.addInferenceLatency(4.0);
    metrics.addInferenceLatency(5.0);

    auto [request_p95, inference_p95] = metrics.getP95Latency();

    EXPECT_DOUBLE_EQ(request_p95, 50.0);
    EXPECT_DOUBLE_EQ(inference_p95, 5.0);
}