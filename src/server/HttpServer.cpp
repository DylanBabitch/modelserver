#include "server/HttpServer.hpp"
#include "request/PredictionRequest.hpp"
#include "model/DummyRuntime.hpp"
#include "model/ModelRegistry.hpp"
#include "metrics/Timer.hpp"
#include <crow.h>
#include <string>
#include <unordered_map>
#include <vector>

HttpServer::HttpServer(ModelRegistry* modelRegistry, MetricsRegistry* metricsRegistry){
    this->modelRegistry = modelRegistry;
    this->metricsRegistry = metricsRegistry;
}

void HttpServer::run(){
    crow::SimpleApp app;

    this->registerHealthRoute(app);
    this->registerPredictRoute(app);
    this->registerModelRegisterRoute(app);
    this->registerMetricsRoute(app);
    this->registerModelRoute(app);
    
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
        Timer t;
        crow::json::rvalue req_data = crow::json::load(req.body); 
    
        metricsRegistry->addActiveRequest();

        //make sure request is well formatted
        crow::response valid_request = this->checkPrediction(req_data);
        if(valid_request.code != 200){
            res = std::move(valid_request);
            res.end();
            metricsRegistry->addFailedRequest();
            return;
        }
        
        //make sure model and version exists;
        std::string model = req_data["model"].s();
        std::string version = req_data["version"].s();
        if(!this->modelRegistry->checkModel(model)){
            res.code = 400;
            res.body = "Error: Model not registered\n";
            res.end();
            metricsRegistry->addFailedRequest();
            return;
        } else if(!this->modelRegistry->checkVersion(model, version)){
            res.code = 400;
            res.body = "Error: Version not registered\n";
            res.end();
            metricsRegistry->addFailedRequest();
            return;
        }

        PredictionRequest pReq = {model, version, req_data["input"].s()};
        PredictionResponse pResp = DummyRuntime::dummyRun(pReq);
        crow::json::wvalue x;
        x["model"] = pResp.model;
        x["version"] = pResp.version;
        x["prediction"] = pResp.prediciton;
        x["confidence"] = pResp.confidence;
        x["inference_latency_ms"] = pResp.latency_ms;

        metricsRegistry->addInferenceLatency(pResp.latency_ms);

        double req_latency = t.end();
        x["request_latency_ms"] = req_latency;
        metricsRegistry->addPrediction(req_latency);
        metricsRegistry->addSuccessfulRequest();

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.end();
        
    });
}

void HttpServer::registerModelRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/models")
    ([this](crow::response& res){
        crow::json::wvalue x;
        std::unordered_map<std::string, std::vector<std::string>>& availableModels = this->modelRegistry->getAvailableModels();
        std::vector<crow::json::wvalue> modelOutput;
        modelOutput.reserve(availableModels.size());
        for(auto& [model, versions] : availableModels){
            crow::json::wvalue temp;
            temp["name"] = model;
            temp["versions"] = versions;
            modelOutput.push_back(temp);
        }
        x["models"] = std::move(modelOutput);

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.end();
    });
}

void HttpServer::registerMetricsRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/metrics")
    ([this](crow::response& res){
        crow::json::wvalue x;
        x["total_reqs"] = this->metricsRegistry->getRequestsTotal();
        x["successful_reqs"] = this->metricsRegistry->getRequestsSuccessful();
        x["failed_reqs"] = this->metricsRegistry->getRequestsFailed();
        x["average_request_latency"] = this->metricsRegistry->getAverageRequestLatency();
        x["average_inference_latency"] = this->metricsRegistry->getAverageInferenceLatency();
        auto [req_p50_latency, infer_p50_latency] = this->metricsRegistry->getP50Latency();
        auto [req_p95_latency, infer_p95_latency] = this->metricsRegistry->getP95Latency();
        x["request_p50_latency"] = req_p50_latency;
        x["request_p95_latency"] = req_p95_latency;
        x["inference_p50_latency"] = infer_p50_latency;
        x["inference_p95_latency"] = infer_p95_latency;

        res.body = x.dump() + "\n";
        res.add_header("Content_Type", "application/json");
        res.end();
    });
}

void HttpServer::registerModelRegisterRoute(crow::SimpleApp& app){
    //TODO add path and runtime to request
    CROW_ROUTE(app, "/models/register").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res){
        crow::json::rvalue req_data = crow::json::load(req.body);

        //check the request formatting
        crow::response formatCheck = this->checkModelRegister(req_data);
        if(formatCheck.code != 200){
            res = std::move(formatCheck);
            res.end();
            return;
        }

        std::string name = req_data["name"].s();
        std::string version = req_data["version"].s();

        //register model
        if(!this->modelRegistry->addModel(name, version)){
            res.code = 400;
            res.body = "Model already added\n";
            res.end();
            return;
        }
        metricsRegistry->addRegisteredModel();
        res.code = 200;
        res.body = "Model Added Sucessfully\n";
        res.end();
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