#pragma once
#include <crow.h>
#include "model/ModelRegistry.hpp"
#include "metrics/MetricsRegistry.hpp"

class HttpServer{
    ModelRegistry* modelReg;
    MetricsRegistry* metricsReg;
    RequestQueue* reqQueue;
    WorkerPool* workerPool;
    std::uint8_t numThreads;
    crow::response checkPrediction(crow::json::rvalue& req_data);
    crow::response checkModelRegister(crow::json::rvalue& req_data);
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    void registerModelRoute(crow::SimpleApp& app);
    void registerModelRegisterRoute(crow::SimpleApp& app);
    void registerMetricsRoute(crow::SimpleApp& app);
public:
    HttpServer(uint8_t numThreads = std::thread::hardware_concurrency());
    ~HttpServer();
    void run();
};