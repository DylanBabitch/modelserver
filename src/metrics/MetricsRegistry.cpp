#include "metrics/MetricsRegistry.hpp"
#include <utility>
#include <algorithm>
#include <cmath>
#include <mutex>

int MetricsRegistry::getRequestsTotal() const{
    std::lock_guard<std::mutex> guard(mtx);
    return requests_total;
}
int MetricsRegistry::getRequestsSuccessful() const{
    std::lock_guard<std::mutex> guard(mtx);
    return requests_successful;
}
int MetricsRegistry::getRequestsFailed() const{
    std::lock_guard<std::mutex> guard(mtx);
    return requests_failed;
}
int MetricsRegistry::getPredictionsTotal() const{
    std::lock_guard<std::mutex> guard(mtx);
    return predictions_total;
}
int MetricsRegistry::getModelsRegisteredTotal() const{
    std::lock_guard<std::mutex> guard(mtx);
    return models_registered_total;
}

int MetricsRegistry::getActiveRequests() const{
    std::lock_guard<std::mutex> guard(mtx);
    return active_requests;
}

void MetricsRegistry::addFailedRequest(){
    std::lock_guard<std::mutex> guard(mtx);
    ++requests_failed;
    --active_requests;
    ++requests_total;
}
void MetricsRegistry::addSuccessfulRequest(){
    std::lock_guard<std::mutex> guard(mtx);
    ++requests_successful;
    --active_requests;
    ++requests_total;
}
void MetricsRegistry::addPrediction(double latency){
    std::lock_guard<std::mutex> guard(mtx);
    request_latencies.push_back(latency);
    total_request_latency += latency;
    ++predictions_total;
}

void MetricsRegistry::addInferenceLatency(double latency){
    std::lock_guard<std::mutex> guard(mtx);
    inference_latencies.push_back(latency);
    total_inference_latency += latency;
}

void MetricsRegistry::addRegisteredModel(){
    std::lock_guard<std::mutex> guard(mtx);
    ++models_registered_total;
}

void MetricsRegistry::addActiveRequest(){
    std::lock_guard<std::mutex> guard(mtx);
    ++active_requests;
}

double MetricsRegistry::getAverageRequestLatency() const {
    std::lock_guard<std::mutex> guard(mtx);
    if(request_latencies.empty()) return 0.0;
    return total_request_latency / request_latencies.size();
}
double MetricsRegistry::getAverageInferenceLatency() const{
    std::lock_guard<std::mutex> guard(mtx);
    if(inference_latencies.empty()) return 0.0;
    return total_inference_latency / inference_latencies.size();
}

int MetricsRegistry::getQueuedRequests() const{
    std::lock_guard<std::mutex> lock(mtx);
    return queued_requests;
}

int MetricsRegistry::getTotalBatches() const{
    std::lock_guard<std::mutex> lock(mtx);
    return batches_total;
}

int MetricsRegistry::getTotalRuntimeErrors() const{
    std::lock_guard<std::mutex> lock(mtx);
    return runtime_errors_total;
}

int MetricsRegistry::getTotalValidationErrors() const{
    std::lock_guard<std::mutex> lock(mtx);
    return validation_errors_total;
}

int MetricsRegistry::getTotalMNFErrors() const{
    std::lock_guard<std::mutex> lock(mtx);
    return model_not_found_errors_total;
}

int MetricsRegistry::getTotalVNFErrors() const{
    std::lock_guard<std::mutex> lock(mtx);
    return version_not_found_errors_total;
}

double MetricsRegistry::getAverageQueueWaitTime() const{
    std::lock_guard<std::mutex> lock(mtx);
    if(queued_wait_latencies.empty()) return 0.0;
    return total_queued_wait / queued_wait_latencies.size();
}

double MetricsRegistry::getAverageBatchSize() const{
    std::lock_guard<std::mutex> lock(mtx);
    if(batch_sizes.empty()) return 0.0;
    return total_batched_requests / batch_sizes.size();
}

double MetricsRegistry::getAverageBatchWaitTime() const{
    std::lock_guard<std::mutex> lock(mtx);
    if(batch_wait_latencies.empty()) return 0.0;
    return total_batch_wait_latnecy / batch_wait_latencies.size();
}

void MetricsRegistry::addQueuedRequest(){
    std::lock_guard<std::mutex> lock(mtx);
    queued_requests++;
}

void MetricsRegistry::addQueueWaitLatency(double latency){
    std::lock_guard<std::mutex> lock(mtx);
    total_queued_wait += latency;
    queued_wait_latencies.push_back(latency);
}

void MetricsRegistry::addBatch(int batchSize){
    std::lock_guard<std::mutex> lock(mtx);
    total_batched_requests += batchSize;
    batches_total++;
}

void MetricsRegistry::addBatchLatency(double latency){
    std::lock_guard<std::mutex> lock(mtx);
    total_batch_wait_latnecy += latency;
    batch_wait_latencies.push_back(latency);
}

void MetricsRegistry::addRuntimeError(){
    std::lock_guard<std::mutex> lock(mtx);
    runtime_errors_total++;
}

void MetricsRegistry::addValidationError(){
    std::lock_guard<std::mutex> lock(mtx);
    validation_errors_total++;
}

void MetricsRegistry::addMNFError(){
    std::lock_guard<std::mutex> lock(mtx);
    model_not_found_errors_total++;
}

void MetricsRegistry::addVNFError(){
    std::lock_guard<std::mutex> lock(mtx);
    version_not_found_errors_total++;
}

double MetricsRegistry::getP50Latency(std::vector<double>& latencies){
    //asume caller has lock on mutex
    if(latencies.size() != 0){
        std::size_t n = latencies.size();
        std::size_t mid = n /2;
        //get inference latency
        std::nth_element(latencies.begin(), latencies.begin() + mid, latencies.end());
        if(n % 2 == 1){
            return latencies[mid];
        } else{
            auto max_prev = std::max_element(latencies.begin(), latencies.begin() + mid);
            return latencies[mid] - (latencies[mid] - *max_prev) / 2;
        }
    }

    return -1; 
}

/*
    Returns RequestP50 Latency}. If there are no latencies for request and/or inference available, the function returns -1 for the respective latency.
*/
double MetricsRegistry::getRequestP50Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP50Latency(request_latencies);
}   

double MetricsRegistry::getInferenceP50Latency() {
    std::lock_guard<std::mutex> guard(mtx);
    return getP50Latency(inference_latencies);
}

double MetricsRegistry::getBatchP50Latency() {
    std::lock_guard<std::mutex> guard(mtx);
    return getP50Latency(batch_wait_latencies);
}

double MetricsRegistry::getQueueP50Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP50Latency(queued_wait_latencies);
}


double MetricsRegistry::getP95Latency(std::vector<double>& latencies){
    if(latencies.size() != 0){
        std::size_t idx = static_cast<std::size_t>(std::ceil(0.95 * latencies.size())) - 1;
        //get request latency
        std::nth_element(latencies.begin(), latencies.begin() + idx, latencies.end());
        return latencies[idx];
    } 
    return -1;
}

double MetricsRegistry::getRequestP95Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP95Latency(request_latencies);
}

double MetricsRegistry::getInferenceP95Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP95Latency(inference_latencies);
}

double MetricsRegistry::getBatchP95Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP95Latency(batch_wait_latencies);
}

double MetricsRegistry::getQueueP95Latency(){
    std::lock_guard<std::mutex> guard(mtx);
    return getP95Latency(queued_wait_latencies);
}