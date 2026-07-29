#include "metrics/MetricsRegistry.hpp"
#include <utility>
#include <algorithm>
#include <cmath>

int MetricsRegistry::getRequestsTotal(){
    return requests_total;
}
int MetricsRegistry::getRequestsSuccessful(){
    return requests_successful;
}
int MetricsRegistry::getRequestsFailed(){
    return requests_failed;
}
int MetricsRegistry::getPredictionsTotal(){
    return predictions_total;
}
int MetricsRegistry::getModelsRegisteredTotal(){
    return models_registered_total;
}

void MetricsRegistry::addFailedRequest(){
    ++requests_failed;
    --active_requests;
    ++requests_total;
}
void MetricsRegistry::addSuccessfulRequest(){
    ++requests_successful;
    --active_requests;
    ++requests_total;
}
void MetricsRegistry::addPrediction(double latency){
    request_latencies.push_back(latency);
    total_request_latency += latency;
    ++predictions_total;
}

void MetricsRegistry::addInferenceLatency(double latency){
    inference_latencies.push_back(latency);
    total_inference_latency += latency;
}

void MetricsRegistry::addRegisteredModel(){
    ++models_registered_total;
}

void MetricsRegistry::addActiveRequest(){
    ++active_requests;
}

double MetricsRegistry::getAverageRequestLatency(){
    if(request_latencies.empty()) return 0.0;
    return total_request_latency / request_latencies.size();
}
double MetricsRegistry::getAverageInferenceLatency(){
    if(inference_latencies.empty()) return 0.0;
    return total_inference_latency / inference_latencies.size();
}

/**
    Returns Request and Inference P50 Latencies in format {request latency, inference latency}
 */
std::pair<double, double> MetricsRegistry::getP50Latency(){
    if(request_latencies.empty() || inference_latencies.empty()) return {0, 0};
    //TODO make this more efficient
    std::pair<double, double> p50Latencies; //{req latency, inference latency}
    int n = request_latencies.size();
    int mid = n / 2;
    std::nth_element(request_latencies.begin(), request_latencies.begin() + mid, request_latencies.end());
    //get request latency
    if(n % 2 == 1){
        p50Latencies.first = request_latencies[mid];
    } else{
        auto max_prev = std::max_element(request_latencies.begin(), request_latencies.begin() + mid);
        p50Latencies.first =  request_latencies[mid] - (request_latencies[mid] - *max_prev) / 2;
    }

    //get inference latency
    std::nth_element(inference_latencies.begin(), inference_latencies.begin() + mid, inference_latencies.end());
    if(n % 2 == 1){
        p50Latencies.second = inference_latencies[mid];
    } else{
        auto max_prev = std::max_element(inference_latencies.begin(), inference_latencies.begin() + mid);
        p50Latencies.first =  inference_latencies[mid] - (inference_latencies[mid] - *max_prev) / 2;
    }

    return p50Latencies;

}   
/**
    Returns Request and Inference P95 Latencies in format {request latency, inference latency}
 */
std::pair<double, double> MetricsRegistry::getP95Latency(){
    if(request_latencies.empty() || inference_latencies.empty()) return {0, 0};
    if(request_latencies.size() == 1 || inference_latencies.size() == 1) return {request_latencies[0], inference_latencies[0]};

    std::pair<double, double> p95Latencies; //{req latency, inference latency}
    int idx = static_cast<size_t>(std::ceil(0.95 * request_latencies.size())) - 1;
    //get request latency
    std::nth_element(request_latencies.begin(), request_latencies.begin() + idx, request_latencies.end());
    p95Latencies.first = request_latencies[idx];

    //get inference latency
    std::nth_element(inference_latencies.begin(), inference_latencies.begin() + idx, inference_latencies.end());
    p95Latencies.second = inference_latencies[idx];

    return p95Latencies;
}