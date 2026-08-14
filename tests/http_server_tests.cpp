#include <gtest/gtest.h>

#include "metrics/MetricsRegistry.hpp"
#include "model/ModelRegistry.hpp"
#include "scheduler/RequestQueue.hpp"
#include "server/HttpServer.hpp"

TEST(HttpServerTest, MetricsResponseUsesTheNestedMetricsShape) {
    ModelRegistry modelRegistry;
    MetricsRegistry metrics;
    RequestQueue queue;
    HttpServer server(modelRegistry, metrics, queue);

    metrics.incrementActiveRequests();
    metrics.recordRequestSuccess();
    metrics.recordRequestFailure();
    metrics.addPredictions(2);
    metrics.recordModelRegistration();
    metrics.recordRequestLatencyMs(10.0);
    metrics.recordInferenceLatencyMs(4.0);
    metrics.recordQueueWaitMs(3.0);
    metrics.recordBatchWaitMs(2.0);
    metrics.recordBatch(2);
    metrics.recordValidationError();
    metrics.recordModelNotFoundError();
    metrics.recordVersionNotFoundError();
    metrics.recordRuntimeError();

    const crow::json::rvalue response = crow::json::load(server.buildMetricsResponse().dump());
    ASSERT_TRUE(response);

    EXPECT_EQ(response["requests"]["total"].i(), 2);
    EXPECT_EQ(response["requests"]["successful"].i(), 1);
    EXPECT_EQ(response["requests"]["failed"].i(), 1);
    EXPECT_EQ(response["requests"]["active"].i(), 1);
    EXPECT_EQ(response["predictions"]["total"].i(), 2);
    EXPECT_EQ(response["models"]["registered_total"].i(), 1);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["request"]["avg"].d(), 10.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["request"]["p50"].d(), 10.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["request"]["p95"].d(), 10.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["inference"]["avg"].d(), 4.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["inference"]["p50"].d(), 4.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["inference"]["p95"].d(), 4.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["queue_wait"]["avg"].d(), 3.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["queue_wait"]["p50"].d(), 3.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["queue_wait"]["p95"].d(), 3.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["batch_wait"]["avg"].d(), 2.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["batch_wait"]["p50"].d(), 2.0);
    EXPECT_DOUBLE_EQ(response["latency_ms"]["batch_wait"]["p95"].d(), 2.0);
    EXPECT_EQ(response["batching"]["batches_total"].i(), 1);
    EXPECT_EQ(response["batching"]["requests_batched_total"].i(), 2);
    EXPECT_DOUBLE_EQ(response["batching"]["average_batch_size"].d(), 2.0);
    EXPECT_EQ(response["batching"]["max_observed_batch_size"].i(), 2);
    EXPECT_EQ(response["errors"]["validation_errors_total"].i(), 1);
    EXPECT_EQ(response["errors"]["model_not_found_errors_total"].i(), 1);
    EXPECT_EQ(response["errors"]["version_not_found_errors_total"].i(), 1);
    EXPECT_EQ(response["errors"]["runtime_errors_total"].i(), 1);
}
