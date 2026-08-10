#pragma once
#include <vector>
#include <mutex>

class MetricsRegistry{
    mutable std::mutex mtx;
    //prediction counters
    int predictions_total = 0;
    //model counters
    int models_registered_total = 0;
    //request metrics
    int requests_total = 0;
    int requests_successful = 0;
    int requests_failed = 0;
    int active_requests = 0;
    double total_request_latency = 0.0; //used to calcultae average request latency in O(1)
    std::vector<double> request_latencies;
    //inference metrics
    double total_inference_latency = 0.0; //used to calculate average inference wait in O(1)
    std::vector<double> inference_latencies;
    //queue metrics
    double queued_requests = 0;
    std::vector<double> queued_wait_latencies;
    double total_queued_wait = 0.0; //used to calculate average queue wait in O(1)
    //batch metrics
    int batches_total; // number of batches formed
    int total_batched_requests; // number of requests that have been batched
    double total_batch_wait_latnecy; //used to calculate average batch wait latency in O(1)
    std::vector<int> batch_sizes; //array of batch sizes across all batches
    std::vector<double> batch_wait_latencies; //array of all wait times for batches
    //error metrics
    int runtime_errors_total; //Model runtime fails
    int validation_errors_total; //Input fails (bad Json, missing fields, etc)
    int model_not_found_errors_total;
    int version_not_found_errors_total;

    double getP50Latency(std::vector<double>& latencies);
    double getP95Latency(std::vector<double>& latencies);
public:
    int getRequestsTotal() const;
    int getRequestsSuccessful() const;
    int getRequestsFailed() const;
    int getPredictionsTotal() const;
    int getModelsRegisteredTotal() const;
    int getActiveRequests() const;
    int getQueuedRequests() const;
    int getTotalBatches() const;
    int getTotalRuntimeErrors() const;
    int getTotalValidationErrors() const;
    int getTotalMNFErrors() const; //total model not found errors
    int getTotalVNFErrors() const; //total version not found errors
    double getAverageRequestLatency() const;
    double getAverageInferenceLatency() const;
    double getAverageQueueWaitTime() const;
    double getAverageBatchSize() const;
    double getAverageBatchWaitTime() const;
    void addFailedRequest();
    void addSuccessfulRequest();
    void addPrediction(double latency);
    void addInferenceLatency(double latency);
    void addRegisteredModel();
    void addActiveRequest();
    void addQueuedRequest();
    void addQueueWaitLatency(double latency);
    void addBatch(int batchSize);
    void addBatchLatency(double latency);
    void addRuntimeError();
    void addValidationError();
    void addMNFError();
    void addVNFError();
    double getRequestP50Latency();
    double getInferenceP50Latency();
    double getQueueP50Latency();
    double getBatchP50Latency();
    double getRequestP95Latency();
    double getInferenceP95Latency();
    double getQueueP95Latency();
    double getBatchP95Latency();
};