#include "model/DummyRuntime.hpp"
#include "request/PredictionResponse.hpp"
#include "request/PredictionRequest.hpp"
#include "metrics/Timer.hpp"
#include <random>

PredictionResponse DummyRuntime::dummyRun(PredictionRequest& pReq){
    //actually run model

    //just make a fake response
    Timer t;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distr(0, 10);
    

    double confidence = distr(gen) / 10;
    double latency = t.end();

    PredictionResponse p{pReq.model, pReq.version, "dummy response", confidence, latency};

    return p;
}