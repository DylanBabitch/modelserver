#pragma once

#include <string>

struct PredictionResponse{
    std::string model;
    std::string version;
    std::string prediciton;
    double confidence = 0.0;
    double latency_ms = 0.0;
};