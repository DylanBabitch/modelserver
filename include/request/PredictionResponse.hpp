#pragma once

#include <string>

struct PredictionResponse{
    std::string model;
    std::string version;
    std::string prediction;
    double confidence = 0.0;
    double inference_latency_ms = 0.0;
    double queue_wait_ms = 0.0;
    double batch_wait_ms = 0.0;
};