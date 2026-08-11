#include <gtest/gtest.h>

#include "metrics/MetricsRegistry.hpp"

TEST(MetricsRegistryTest, StartsAtZero) {
    MetricsRegistry metrics;

    EXPECT_EQ(metrics.getRequestsTotal(), 0U);
    EXPECT_EQ(metrics.getRequestsSuccessful(), 0U);
    EXPECT_EQ(metrics.getRequestsFailed(), 0U);
    EXPECT_EQ(metrics.getActiveRequests(), 0U);
    EXPECT_EQ(metrics.getPredictionsTotal(), 0U);
    EXPECT_EQ(metrics.getModelsRegisteredTotal(), 0U);
    EXPECT_EQ(metrics.getBatchesTotal(), 0U);
    EXPECT_EQ(metrics.getRequestsBatchedTotal(), 0U);
    EXPECT_EQ(metrics.getMaxObservedBatchSize(), 0U);
    EXPECT_EQ(metrics.getValidationErrorsTotal(), 0U);
    EXPECT_EQ(metrics.getModelNotFoundErrorsTotal(), 0U);
    EXPECT_EQ(metrics.getVersionNotFoundErrorsTotal(), 0U);
    EXPECT_EQ(metrics.getRuntimeErrorsTotal(), 0U);
}

TEST(MetricsRegistryTest, SuccessfulAndFailedRequestsDetermineTotal) {
    MetricsRegistry metrics;

    metrics.recordRequestSuccess();
    metrics.recordRequestFailure();

    EXPECT_EQ(metrics.getRequestsSuccessful(), 1U);
    EXPECT_EQ(metrics.getRequestsFailed(), 1U);
    EXPECT_EQ(metrics.getRequestsTotal(), 2U);
}

TEST(MetricsRegistryTest, ActiveRequestsNeverBecomesNegative) {
    MetricsRegistry metrics;

    metrics.incrementActiveRequests();
    metrics.incrementActiveRequests();
    metrics.decrementActiveRequests();
    metrics.decrementActiveRequests();
    metrics.decrementActiveRequests();

    EXPECT_EQ(metrics.getActiveRequests(), 0U);
}

TEST(MetricsRegistryTest, RecordsPredictionAndModelTotals) {
    MetricsRegistry metrics;

    metrics.addPredictions(8);
    metrics.recordModelRegistration();
    metrics.recordModelRegistration();

    EXPECT_EQ(metrics.getPredictionsTotal(), 8U);
    EXPECT_EQ(metrics.getModelsRegisteredTotal(), 2U);
}

TEST(MetricsRegistryTest, RequestLatencyStatisticsUseAverageMedianAndP95) {
    MetricsRegistry metrics;
    for (double latency : {10.0, 20.0, 30.0, 40.0}) {
        metrics.recordRequestLatencyMs(latency);
    }

    EXPECT_DOUBLE_EQ(metrics.getAverageRequestLatencyMs(), 25.0);
    EXPECT_DOUBLE_EQ(metrics.getRequestP50LatencyMs(), 25.0);
    EXPECT_DOUBLE_EQ(metrics.getRequestP95LatencyMs(), 40.0);
}

TEST(MetricsRegistryTest, InferenceLatencyStatisticsUseAverageMedianAndP95) {
    MetricsRegistry metrics;
    for (double latency : {1.0, 2.0, 3.0, 4.0}) {
        metrics.recordInferenceLatencyMs(latency);
    }

    EXPECT_DOUBLE_EQ(metrics.getAverageInferenceLatencyMs(), 2.5);
    EXPECT_DOUBLE_EQ(metrics.getInferenceP50LatencyMs(), 2.5);
    EXPECT_DOUBLE_EQ(metrics.getInferenceP95LatencyMs(), 4.0);
}

TEST(MetricsRegistryTest, QueueWaitLatencyStatisticsUseAverageMedianAndP95) {
    MetricsRegistry metrics;
    for (double latency : {2.0, 4.0, 6.0, 8.0}) {
        metrics.recordQueueWaitMs(latency);
    }

    EXPECT_DOUBLE_EQ(metrics.getAverageQueueWaitMs(), 5.0);
    EXPECT_DOUBLE_EQ(metrics.getQueueWaitP50Ms(), 5.0);
    EXPECT_DOUBLE_EQ(metrics.getQueueWaitP95Ms(), 8.0);
}

TEST(MetricsRegistryTest, BatchMetricsTrackCountsAndWaitStatistics) {
    MetricsRegistry metrics;

    metrics.recordBatch(2);
    metrics.recordBatch(4);
    for (double latency : {3.0, 6.0, 9.0, 12.0}) {
        metrics.recordBatchWaitMs(latency);
    }

    EXPECT_EQ(metrics.getBatchesTotal(), 2U);
    EXPECT_EQ(metrics.getRequestsBatchedTotal(), 6U);
    EXPECT_DOUBLE_EQ(metrics.getAverageBatchSize(), 3.0);
    EXPECT_EQ(metrics.getMaxObservedBatchSize(), 4U);
    EXPECT_DOUBLE_EQ(metrics.getAverageBatchWaitMs(), 7.5);
    EXPECT_DOUBLE_EQ(metrics.getBatchWaitP50Ms(), 7.5);
    EXPECT_DOUBLE_EQ(metrics.getBatchWaitP95Ms(), 12.0);
}

TEST(MetricsRegistryTest, RecordsErrorCounters) {
    MetricsRegistry metrics;

    metrics.recordValidationError();
    metrics.recordModelNotFoundError();
    metrics.recordVersionNotFoundError();
    metrics.recordRuntimeError();

    EXPECT_EQ(metrics.getValidationErrorsTotal(), 1U);
    EXPECT_EQ(metrics.getModelNotFoundErrorsTotal(), 1U);
    EXPECT_EQ(metrics.getVersionNotFoundErrorsTotal(), 1U);
    EXPECT_EQ(metrics.getRuntimeErrorsTotal(), 1U);
}

TEST(MetricsRegistryTest, EmptyLatencyStatisticsReturnZero) {
    MetricsRegistry metrics;

    EXPECT_DOUBLE_EQ(metrics.getAverageRequestLatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getRequestP50LatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getRequestP95LatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getAverageInferenceLatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getInferenceP50LatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getInferenceP95LatencyMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getAverageQueueWaitMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getQueueWaitP50Ms(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getQueueWaitP95Ms(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getAverageBatchWaitMs(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getBatchWaitP50Ms(), 0.0);
    EXPECT_DOUBLE_EQ(metrics.getBatchWaitP95Ms(), 0.0);
}
