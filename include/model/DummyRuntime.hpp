#pragma once

#include "model/ModelRuntime.hpp"

class DummyRuntime : public ModelRuntime{
public:
    PredictionResponse predict(const PredictionRequest& request) override final;
};