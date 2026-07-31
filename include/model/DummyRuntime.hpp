#pragma once

#include "model/ModelRuntime.hpp"
#include "request/PredictionResponse.hpp"
#include "request/PredictionRequest.hpp"
#include <crow.h>

class DummyRuntime : public ModelRuntime{
public:
    PredictionResponse predict(const PredictionRequest& request) override;
};