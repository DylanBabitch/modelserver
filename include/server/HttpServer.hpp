#pragma once
#include <crow.h>

class HttpServer{
public:
    void run();
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    void registerModelRoute(crow::SimpleApp& app);
    void registerModelRegisterRoute(crow::SimpleApp& app, ModelRegistry& registry);
    crow::response checkPrediction(crow::json::rvalue& req_data);
    crow::response checkModelRegister(crow::json::rvalue& req_data);
};