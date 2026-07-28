#include "model/DummyRuntime.hpp"
#include "request/PredictionResponse.hpp"
#include "request/PredictionRequest.hpp"
#include <random>

PredictionResponse DummyRuntime::dummyRun(PredictionRequest& pReq){
    //actually run model

    //just make a fake response

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distr(0, 1);

    double confidence = distr(gen);

    PredictionResponse p{pReq.model, pReq.version, "dummy response", confidence, 0.0};

    return p;
}