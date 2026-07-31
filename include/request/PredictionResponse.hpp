#pragma once

#include <string>

struct PredictionResponse{
    std::string model;
    std::string version;
    std::string prediction;
    double confidence = 0.0;
    double latency_ms = 0.0;
};