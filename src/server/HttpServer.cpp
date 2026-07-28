#include "server/HttpServer.hpp"
#include "request/PredictionRequest.hpp"
#include "model/DummyRuntime.hpp"
#include <crow.h>
#include <string>

void HttpServer::run(){
    crow::SimpleApp app;

    this->registerHealthRoute(app);
    this->registerPredictRoute(app);
    
    app.port(8080).run();
}

void HttpServer::registerHealthRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/health")
    ([](crow::response& res){
        crow::json::wvalue x;
        x["status"] = "ok";

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.add_header("Cache-Control", "no-store");
        res.end();
    });
}

void HttpServer::registerPredictRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/predict").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res){
        //add this logic later
        // try{

        // }catch(std::runtime_error){
            
        // }

        //for now assume well formed request
        crow::json::rvalue req_data = crow::json::load(req.body); 

        crow::response valid_request = this->checkPrediction(req_data);
        if(valid_request.code != 200){
            return valid_request;
        }


        PredictionRequest pReq = {req_data["model"].s(), req_data["version"].s(), req_data["input"].s()};
        PredictionResponse pResp = DummyRuntime::dummyRun(pReq);
        crow::json::wvalue x;
        x["model"] = pResp.model;
        x["version"] = pResp.version;
        x["prediction"] = pResp.prediciton;
        x["confidence"] = pResp.confidence;

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.end();
    });
}

crow::response HttpServer::checkPrediction(crow::json::rvalue& req_data){
    if(!req_data){
        return crow::response(400, "Invalid JSON Body");
    } else if(!req_data.has("model")){
        return crow::response(400, "Missing required field: model");
    } else if(!req_data.has("version")){
        return crow::response(400, "Missing required field: version");
    } else if(!req_data.has("input")){
        return crow::response(400, "Missing required field: input");
    }
}