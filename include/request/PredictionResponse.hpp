#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct PredictionResponse{
    struct TensorOutput {
        std::vector<std::int64_t> shape;
        std::vector<float> data;
    };
    std::string model;
    std::string version;
    TensorOutput output;
    double inference_latency_ms = 0.0;
    double queue_wait_ms = 0.0;
    double batch_wait_ms = 0.0;
};