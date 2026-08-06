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

/*
    Returns Request and Inference P50 Latencies in format {request latency, inference latency}. If there are no latencies for request and/or inference available, the function returns -1 for the respective latency.
 */
std::pair<double, double> MetricsRegistry::getP50Latency() const{
    //make copies of the latencies while locked
    std::unique_lock<std::mutex> guard(mtx);
    std::vector<double> req_lat(request_latencies);
    std::vector<double> infer_lat(inference_latencies);
    guard.unlock();

    //TODO make this more efficient
    std::pair<double, double> p50Latencies; //{req latency, inference latency}

    if(req_lat.size() != 0){
        size_t n = req_lat.size();
        size_t mid = n / 2;
        std::nth_element(req_lat.begin(), req_lat.begin() + mid, req_lat.end());
        //get request latency
        if(n % 2 == 1){
            p50Latencies.first = req_lat[mid];
        } else{
            auto max_prev = std::max_element(req_lat.begin(), req_lat.begin() + mid);
            p50Latencies.first =  req_lat[mid] - (req_lat[mid] - *max_prev) / 2;
        }
    } else {
        p50Latencies.first = -1;
    }
   
    if(infer_lat.size() != 0){
        size_t n = infer_lat.size();
        size_t mid = n /2;
        //get inference latency
        std::nth_element(infer_lat.begin(), infer_lat.begin() + mid, infer_lat.end());
        if(n % 2 == 1){
            p50Latencies.second = infer_lat[mid];
        } else{
            auto max_prev = std::max_element(infer_lat.begin(), infer_lat.begin() + mid);
            p50Latencies.second =  infer_lat[mid] - (infer_lat[mid] - *max_prev) / 2;
        }
    } else {
        p50Latencies.second = -1;
    }   
    
    return p50Latencies;

}   
/**
    Returns Request and Inference P95 Latencies in format {request latency, inference latency}. If there are no latencies for request and/or inference available, the function returns -1 for the respective latency.
 */
std::pair<double, double> MetricsRegistry::getP95Latency() const{
    //make copies of the latencies while locked
    std::unique_lock<std::mutex> guard(mtx);
    std::vector<double> req_lat(request_latencies);
    std::vector<double> infer_lat(inference_latencies);
    guard.unlock();


    std::pair<double, double> p95Latencies; //{req latency, inference latency}
    if(req_lat.size() != 0){
        std::size_t idx = static_cast<size_t>(std::ceil(0.95 * req_lat.size())) - 1;
        //get request latency
        std::nth_element(req_lat.begin(), req_lat.begin() + idx, req_lat.end());
        p95Latencies.first = req_lat[idx];
    } else{
        p95Latencies.first = -1;
    }

    //get inference latency
    if(infer_lat.size() != 0){
        std::size_t idx = static_cast<size_t>(std::ceil(0.95 * infer_lat.size())) - 1;
        std::nth_element(infer_lat.begin(), infer_lat.begin() + idx, infer_lat.end());
        p95Latencies.second = infer_lat[idx];
    }else{
        p95Latencies.second = -1;
    }

    return p95Latencies;
}