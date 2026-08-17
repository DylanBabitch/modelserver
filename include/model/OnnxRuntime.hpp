#pragma once

#include <model/ModelRuntime.hpp>
#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>
#include <onnxruntime_cxx_api.h>

#include <cstdint>
#include <string>
#include <vector>

class OnnxRuntime : public ModelRuntime{
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    std::string inputName;
    std::string outputName;
    std::vector<std::int64_t> inputShape;
    
public:
    explicit OnnxRuntime(const std::string& modelPath);
    PredictionResponse predict(const PredictionRequest& request) override final;

};
