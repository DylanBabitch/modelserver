#pragma once
#include <vector>
#include <utility>
#include <mutex>

class MetricsRegistry{
    mutable std::mutex mtx;
    int requests_total = 0;
    int requests_successful = 0;
    int requests_failed = 0;
    int predictions_total = 0;
    int models_registered_total = 0;
    double total_request_latency = 0.0;
    double total_inference_latency = 0.0;
    int active_requests = 0;
    std::vector<double> request_latencies;
    std::vector<double> inference_latencies;
public:
    int getRequestsTotal() const;
    int getRequestsSuccessful() const;
    int getRequestsFailed() const;
    int getPredictionsTotal() const;
    int getModelsRegisteredTotal() const;
    int getActiveRequests() const;
    double getAverageRequestLatency() const;
    double getAverageInferenceLatency() const;
    void addFailedRequest();
    void addSuccessfulRequest();
    void addPrediction(double latency);
    void addInferenceLatency(double latency);
    void addRegisteredModel();
    void addActiveRequest();
    std::pair<double, double> getP50Latency() const;
    std::pair<double, double> getP95Latency() const;
};