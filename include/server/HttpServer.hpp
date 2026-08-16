#pragma once
#include <crow.h>
#include "model/ModelRegistry.hpp"
#include "metrics/MetricsRegistry.hpp"
#include "scheduler/RequestQueue.hpp"
#include "scheduler/WorkerPool.hpp"

#include <thread>
#include <cstdint>
#include <memory>

class HttpServer{
    ModelRegistry& modelReg;
    MetricsRegistry& metricsReg;
    RequestQueue& reqQueue;
    crow::response checkPrediction(crow::json::rvalue& req_data);
    crow::response checkModelRegister(crow::json::rvalue& req_data);
    crow::response checkModelLoad(crow::json::rvalue& req_data);
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    void registerModelRoute(crow::SimpleApp& app);
    void registerModelRegisterRoute(crow::SimpleApp& app);
    void registerMetricsRoute(crow::SimpleApp& app);
    void registerModelLoadRoute(crow::SimpleApp& app);
    void registerModelUnloadRoute(crow::SimpleApp& app);
public:
    HttpServer(ModelRegistry& modelReg, MetricsRegistry& metricsReg, RequestQueue& reqQueue);
    crow::json::wvalue buildMetricsResponse() const;
    void run();
};
