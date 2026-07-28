#pragma once
#include <crow.h>

class HttpServer{
public:
    void run();
    void registerHealthRoute(crow::SimpleApp& app);
    void registerPredictRoute(crow::SimpleApp& app);
    crow::response checkPrediction(crow::json::rvalue& req_data);
};