#include "server/HttpServer.hpp"
#include <crow.h>

void HttpServer::run(){
    crow::SimpleApp app;
    //const crow::request &req
    CROW_ROUTE(app, "/health")
    ([](){
        crow::json::wvalue x;
        x["status"] = "ok";
        return x;
    });
    
    app.port(8080).run();
}