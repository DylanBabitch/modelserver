#include "model/DummyRuntime.hpp"
#include "request/PredictionResponse.hpp"
#include "request/PredictionRequest.hpp"
#include "metrics/Timer.hpp"
#include <random>
#include <sstream>


//Deprecated as of 8/13/2026, use real 
PredictionResponse DummyRuntime::predict(const PredictionRequest& request) {
    //actually run model

    //just make a fake response
    Timer t;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distr(0, 10);
    
    // std::istringstream iss(request.input);
    std::string word;

    bool pos = false, neg = false;
    // // while(iss >> word){
    //     if(pos && neg) break;
    //     if(word == "great") pos = true;
    //     else if(word == "bad") neg = true;
    // }

    std::string fakeOutput = ((pos && neg) || (!pos && !neg)) ? "neutral" : pos ? "positive" : "negative";
    
    double confidence = distr(gen) / 10;
    double latency = t.end();


    // PredictionResponse p{request.model, request.version, fakeOutput, confidence, latency};

    // return p;
}