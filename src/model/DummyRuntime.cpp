#include "model/DummyRuntime.hpp"
#include "request/PredictionResponse.hpp"
#include "request/PredictionRequest.hpp"
#include "metrics/Timer.hpp"
#include <random>


//Deprecated as of 8/13/2026, use real 
PredictionResponse DummyRuntime::predict(const PredictionRequest& request) {
    //actually run model

    //just make a fake response
    Timer t;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distr(0, 10);
    
    double confidence = distr(gen) / 10;
    double latency = t.end();

    return PredictionResponse{request.model, request.version, {"output", {1}, {static_cast<float>(confidence)}}, latency};
}
