#pragma once

#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>

class ModelRuntime{
public:
    virtual PredictionResponse predict(const PredictionRequest& request) = 0;
    virtual ~ModelRuntime() = default;
};