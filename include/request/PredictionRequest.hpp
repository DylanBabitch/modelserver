#pragma once

#include <string>

struct PredictionRequest {
    std::string model;
    std::string version;
    std::string input;
};