#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

class MetricsRegistry {
private:
    mutable std::mutex mtx;

    std::size_t requests_successful = 0;
    std::size_t requests_failed = 0;
    std::size_t active_requests = 0;
    std::size_t predictions_total = 0;
    std::size_t models_registered_total = 0;

    double total_request_latency_ms = 0.0;
    std::vector<double> request_latencies_ms;
    double total_inference_latency_ms = 0.0;
    std::vector<double> inference_latencies_ms;
    double total_queue_wait_ms = 0.0;
    std::vector<double> queue_wait_latencies_ms;
    double total_batch_wait_ms = 0.0;
    std::vector<double> batch_wait_latencies_ms;

    std::size_t batches_total = 0;
    std::size_t requests_batched_total = 0;
    std::size_t max_observed_batch_size = 0;
    std::vector<std::size_t> batch_sizes;

    std::size_t validation_errors_total = 0;
    std::size_t model_not_found_errors_total = 0;
    std::size_t version_not_found_errors_total = 0;
    std::size_t runtime_errors_total = 0;

    static double median(std::vector<double> values);
    static double percentile95(std::vector<double> values);

public:
    std::size_t getRequestsTotal() const;
    std::size_t getRequestsSuccessful() const;
    std::size_t getRequestsFailed() const;
    std::size_t getActiveRequests() const;
    std::size_t getPredictionsTotal() const;
    std::size_t getModelsRegisteredTotal() const;

    double getAverageRequestLatencyMs() const;
    double getRequestP50LatencyMs() const;
    double getRequestP95LatencyMs() const;
    double getAverageInferenceLatencyMs() const;
    double getInferenceP50LatencyMs() const;
    double getInferenceP95LatencyMs() const;
    double getAverageQueueWaitMs() const;
    double getQueueWaitP50Ms() const;
    double getQueueWaitP95Ms() const;

    std::size_t getBatchesTotal() const;
    std::size_t getRequestsBatchedTotal() const;
    double getAverageBatchSize() const;
    std::size_t getMaxObservedBatchSize() const;
    double getAverageBatchWaitMs() const;
    double getBatchWaitP50Ms() const;
    double getBatchWaitP95Ms() const;

    std::size_t getValidationErrorsTotal() const;
    std::size_t getModelNotFoundErrorsTotal() const;
    std::size_t getVersionNotFoundErrorsTotal() const;
    std::size_t getRuntimeErrorsTotal() const;

    void recordRequestSuccess();
    void recordRequestFailure();
    void incrementActiveRequests();
    void decrementActiveRequests();
    void addPredictions(std::size_t count);
    void recordModelRegistration();
    void recordRequestLatencyMs(double latency_ms);
    void recordInferenceLatencyMs(double latency_ms);
    void recordQueueWaitMs(double latency_ms);
    void recordBatch(std::size_t batch_size);
    void recordBatchWaitMs(double latency_ms);
    void recordValidationError();
    void recordModelNotFoundError();
    void recordVersionNotFoundError();
    void recordRuntimeError();
};
