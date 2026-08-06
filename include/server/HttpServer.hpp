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
    std::shared_ptr<ModelRegistry> modelReg;
    std::shared_ptr<MetricsRegistry> metricsReg;
    std::shared_ptr<RequestQueue> reqQueue;
    std::shared_ptr<WorkerPool> workerPool;
    std::size_t numThreads;
    crow::response checkPrediction(crow::json::rvalue& req_data);
    crow::response checkModelRegister(crow::json::rvalue& req_data);
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    void registerModelRoute(crow::SimpleApp& app);
    void registerModelRegisterRoute(crow::SimpleApp& app);
    void registerMetricsRoute(crow::SimpleApp& app);
public:
    HttpServer(std::size_t numThreads = std::thread::hardware_concurrency());
    void run();
};