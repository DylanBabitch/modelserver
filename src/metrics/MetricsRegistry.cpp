#include "metrics/MetricsRegistry.hpp"

#include <algorithm>
#include <cmath>

double MetricsRegistry::median(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }

    const std::size_t middle = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + middle, values.end());
    const double upper_middle = values[middle];

    if (values.size() % 2 != 0) {
        return upper_middle;
    }

    const double lower_middle = *std::max_element(values.begin(), values.begin() + middle);
    return (lower_middle + upper_middle) / 2.0;
}

double MetricsRegistry::percentile95(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }

    const std::size_t index = static_cast<std::size_t>(std::ceil(values.size() * 0.95)) - 1;
    std::nth_element(values.begin(), values.begin() + index, values.end());
    return values[index];
}

std::size_t MetricsRegistry::getRequestsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return requests_successful + requests_failed;
}

std::size_t MetricsRegistry::getRequestsSuccessful() const {
    std::lock_guard<std::mutex> lock(mtx);
    return requests_successful;
}

std::size_t MetricsRegistry::getRequestsFailed() const {
    std::lock_guard<std::mutex> lock(mtx);
    return requests_failed;
}

std::size_t MetricsRegistry::getActiveRequests() const {
    std::lock_guard<std::mutex> lock(mtx);
    return active_requests;
}

std::size_t MetricsRegistry::getPredictionsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return predictions_total;
}

std::size_t MetricsRegistry::getModelsRegisteredTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return models_registered_total;
}

double MetricsRegistry::getAverageRequestLatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return request_latencies_ms.empty() ? 0.0 : total_request_latency_ms / request_latencies_ms.size();
}

double MetricsRegistry::getRequestP50LatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return median(request_latencies_ms);
}

double MetricsRegistry::getRequestP95LatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return percentile95(request_latencies_ms);
}

double MetricsRegistry::getAverageInferenceLatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return inference_latencies_ms.empty() ? 0.0 : total_inference_latency_ms / inference_latencies_ms.size();
}

double MetricsRegistry::getInferenceP50LatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return median(inference_latencies_ms);
}

double MetricsRegistry::getInferenceP95LatencyMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return percentile95(inference_latencies_ms);
}

double MetricsRegistry::getAverageQueueWaitMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue_wait_latencies_ms.empty() ? 0.0 : total_queue_wait_ms / queue_wait_latencies_ms.size();
}

double MetricsRegistry::getQueueWaitP50Ms() const {
    std::lock_guard<std::mutex> lock(mtx);
    return median(queue_wait_latencies_ms);
}

double MetricsRegistry::getQueueWaitP95Ms() const {
    std::lock_guard<std::mutex> lock(mtx);
    return percentile95(queue_wait_latencies_ms);
}

std::size_t MetricsRegistry::getBatchesTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return batches_total;
}

std::size_t MetricsRegistry::getRequestsBatchedTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return requests_batched_total;
}

double MetricsRegistry::getAverageBatchSize() const {
    std::lock_guard<std::mutex> lock(mtx);
    return batch_sizes.empty() ? 0.0 : static_cast<double>(requests_batched_total) / batch_sizes.size();
}

std::size_t MetricsRegistry::getMaxObservedBatchSize() const {
    std::lock_guard<std::mutex> lock(mtx);
    return max_observed_batch_size;
}

double MetricsRegistry::getAverageBatchWaitMs() const {
    std::lock_guard<std::mutex> lock(mtx);
    return batch_wait_latencies_ms.empty() ? 0.0 : total_batch_wait_ms / batch_wait_latencies_ms.size();
}

double MetricsRegistry::getBatchWaitP50Ms() const {
    std::lock_guard<std::mutex> lock(mtx);
    return median(batch_wait_latencies_ms);
}

double MetricsRegistry::getBatchWaitP95Ms() const {
    std::lock_guard<std::mutex> lock(mtx);
    return percentile95(batch_wait_latencies_ms);
}

std::size_t MetricsRegistry::getValidationErrorsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return validation_errors_total;
}

std::size_t MetricsRegistry::getModelNotFoundErrorsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return model_not_found_errors_total;
}

std::size_t MetricsRegistry::getVersionNotFoundErrorsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return version_not_found_errors_total;
}

std::size_t MetricsRegistry::getRuntimeErrorsTotal() const {
    std::lock_guard<std::mutex> lock(mtx);
    return runtime_errors_total;
}

void MetricsRegistry::recordRequestSuccess() {
    std::lock_guard<std::mutex> lock(mtx);
    ++requests_successful;
}

void MetricsRegistry::recordRequestFailure() {
    std::lock_guard<std::mutex> lock(mtx);
    ++requests_failed;
}

void MetricsRegistry::incrementActiveRequests() {
    std::lock_guard<std::mutex> lock(mtx);
    ++active_requests;
}

void MetricsRegistry::decrementActiveRequests() {
    std::lock_guard<std::mutex> lock(mtx);
    if (active_requests > 0) {
        --active_requests;
    }
}

void MetricsRegistry::addPredictions(std::size_t count) {
    std::lock_guard<std::mutex> lock(mtx);
    predictions_total += count;
}

void MetricsRegistry::recordModelRegistration() {
    std::lock_guard<std::mutex> lock(mtx);
    ++models_registered_total;
}

void MetricsRegistry::recordRequestLatencyMs(double latency_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    request_latencies_ms.push_back(latency_ms);
    total_request_latency_ms += latency_ms;
}

void MetricsRegistry::recordInferenceLatencyMs(double latency_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    inference_latencies_ms.push_back(latency_ms);
    total_inference_latency_ms += latency_ms;
}

void MetricsRegistry::recordQueueWaitMs(double latency_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    queue_wait_latencies_ms.push_back(latency_ms);
    total_queue_wait_ms += latency_ms;
}

void MetricsRegistry::recordBatch(std::size_t batch_size) {
    std::lock_guard<std::mutex> lock(mtx);
    ++batches_total;
    requests_batched_total += batch_size;
    max_observed_batch_size = std::max(max_observed_batch_size, batch_size);
    batch_sizes.push_back(batch_size);
}

void MetricsRegistry::recordBatchWaitMs(double latency_ms) {
    std::lock_guard<std::mutex> lock(mtx);
    batch_wait_latencies_ms.push_back(latency_ms);
    total_batch_wait_ms += latency_ms;
}

void MetricsRegistry::recordValidationError() {
    std::lock_guard<std::mutex> lock(mtx);
    ++validation_errors_total;
}

void MetricsRegistry::recordModelNotFoundError() {
    std::lock_guard<std::mutex> lock(mtx);
    ++model_not_found_errors_total;
}

void MetricsRegistry::recordVersionNotFoundError() {
    std::lock_guard<std::mutex> lock(mtx);
    ++version_not_found_errors_total;
}

void MetricsRegistry::recordRuntimeError() {
    std::lock_guard<std::mutex> lock(mtx);
    ++runtime_errors_total;
}
