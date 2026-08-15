#pragma once

#include "model/ModelRuntime.hpp"

//Deprecated as of 8/13/2026, use real runtimes
class DummyRuntime : public ModelRuntime{
public:
    PredictionResponse predict(const PredictionRequest& request) override final;
};