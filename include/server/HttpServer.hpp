#pragma once
#include <crow.h>
#include "model/ModelRegistry.hpp"

class HttpServer{
    ModelRegistry* registry;
public:
    HttpServer(ModelRegistry* registry);
    void run();
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    void registerModelRoute(crow::SimpleApp& app);
    void registerModelRegisterRoute(crow::SimpleApp& app);
    crow::response checkPrediction(crow::json::rvalue& req_data);
    crow::response checkModelRegister(crow::json::rvalue& req_data);
};