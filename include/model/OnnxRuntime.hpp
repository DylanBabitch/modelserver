#pragma once

#include <model/ModelRuntime.hpp>
#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>
#include <onnxruntime_cxx_api.h>


class OnnxRuntime : public ModelRuntime{
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    
public:
    explicit OnnxRuntime(const std::string& modelPath);
    PredictionResponse predict(PredictionRequest& request) override final;

};