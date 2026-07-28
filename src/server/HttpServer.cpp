#include "server/HttpServer.hpp"
#include "request/PredictionRequest.hpp"
#include "model/DummyRuntime.hpp"
#include "model/ModelRegistry.hpp"
#include <crow.h>
#include <string>

void HttpServer::run(){
    crow::SimpleApp app;
    ModelRegistry registry;

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
        x["latency_ms"] = pResp.latency_ms;

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.end();
    });
}

void HttpServer::registerModelRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/models")
    ([this](crow::response& res){
        return crow::response(404, "Not added yet");
    });
}

void HttpServer::registerModelRegisterRoute(crow::SimpleApp& app, ModelRegistry& registry){
    //TODO add path and runtime to request
    CROW_ROUTE(app, "models/register").methods(crow::HTTPMethod::POST)
    ([this, registry](crow::request& req, crow::response& res){
        crow::json::rvalue req_data = crow::json::load(req.body);

        //check the request formatting
        crow::response formatCheck = this->checkModelRegister(req_data);
        if(formatCheck.code != 200) return formatCheck;

        std::string name = req_data["name"].s();
        std::string version = req_data["version"].s();

        //check if model exists
        if(registry.checkVersion(name, version)){
            
        }

        //register model
    });
}

crow::response HttpServer::checkPrediction(crow::json::rvalue& req_data){
    //Make sure request is valid JSON
    if(!req_data){
        return crow::response(400, "Invalid JSON Body");
    } 

    //Check if req has all required fields
    if(!req_data.has("model")){
        return crow::response(400, "Missing required field: model");
    } else if(!req_data.has("version")){
        return crow::response(400, "Missing required field: version");
    } else if(!req_data.has("input")){
        return crow::response(400, "Missing required field: input");
    }

    //Check field types
    if(req_data["model"].t() != crow::json::type::String){
        return crow::response(400, "Field 'model' must be a string");
    } else if(req_data["version"].t() != crow::json::type::String){
        return crow::response(400, "Field 'model' must be a string");
    } else if(req_data["input"].t() != crow::json::type::String){
        return crow::response(400, "Field 'model' must be a string");
    }

    return crow::response(200, "Well Formatted");
}

crow::response HttpServer::checkModelRegister(crow::json::rvalue& req_data){
    //TODO add checks 
    if(!req_data){
        return crow::response(400, "Invalid JSON Body");
    }

    if(!req_data.has("name")){
        return crow::response(400, "Missing required field: name");
    } else if(!req_data.has("version")){
        return crow::response(400, "Missing required field: version");
    }

    if(req_data["name"].t() != crow::json::type::String){
        return crow::response(400, "Field 'name' must be a string");
    } else if(req_data["version"].t() != crow::json::type::String){
        return crow::response(400, "Field 'version' must be a string");
    }

    return crow::response(200, "Well formatted");
}