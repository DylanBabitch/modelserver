#pragma once
#include <vector>
#include <utility>

class MetricsRegistry{
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
    int getRequestsTotal();
    int getRequestsSuccessful();
    int getRequestsFailed();
    int getPredictionsTotal();
    int getModelsRegisteredTotal();
    double getAverageRequestLatency();
    double getAverageInferenceLatency();
    void addFailedRequest();
    void addSuccessfulRequest();
    void addPrediction(double latency);
    void addInferenceLatency(double latency);
    void addRegisteredModel();
    void addActiveRequest();
    std::pair<double, double> getP50Latency();
    std::pair<double, double> getP95Latency();
};