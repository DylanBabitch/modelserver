#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct PredictionRequest {
    struct TensorInput {
        std::string name;
        std::vector<std::int64_t> shape;
        std::vector<float> data;
    };

    std::string model;
    std::string version;
    TensorInput input;
};
