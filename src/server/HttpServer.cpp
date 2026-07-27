#include "server/HttpServer.hpp"
#include "crow.h"

void HttpServer::run(){
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([](){
        return crow::json()
    })
}