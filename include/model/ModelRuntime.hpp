#pragma once

#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>

#include <vector>

class ModelRuntime{
public:
    virtual PredictionResponse predict(const PredictionRequest& request) = 0;
    virtual std::vector<PredictionResponse> predictBatch(const std::vector<PredictionRequest>& requests) {
        std::vector<PredictionResponse> responses;
        responses.reserve(requests.size());
        for (const PredictionRequest& request : requests) {
            responses.push_back(predict(request));
        }
        return responses;
    }
    virtual ~ModelRuntime() = default;
};
