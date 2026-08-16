#include "server/HttpServer.hpp"

#include "metrics/Timer.hpp"
#include "model/OnnxRuntime.hpp"
#include "request/PredictionRequest.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class ActiveRequestGuard {
private:
    MetricsRegistry& metricsReg;

public:
    explicit ActiveRequestGuard(MetricsRegistry& metricsReg) : metricsReg(metricsReg) {
        metricsReg.incrementActiveRequests();
    }

    ~ActiveRequestGuard() {
        metricsReg.decrementActiveRequests();
    }
};

double elapsedMilliseconds(
    const QueuedRequest::Clock::time_point start,
    const QueuedRequest::Clock::time_point finish)
{
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

} // namespace

HttpServer::HttpServer(ModelRegistry& modelReg, MetricsRegistry& metricsReg, RequestQueue& reqQueue)
    : modelReg(modelReg), metricsReg(metricsReg), reqQueue(reqQueue) {}

void HttpServer::run() {
    crow::SimpleApp app;

    registerHealthRoute(app);
    registerPredictRoute(app);
    registerModelRegisterRoute(app);
    registerMetricsRoute(app);
    registerModelRoute(app);
    registerModelLoadRoute(app);
    registerModelUnloadRoute(app);

    app.port(8080).run();
}

void HttpServer::registerHealthRoute(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/health")
    ([](crow::response& res) {
        crow::json::wvalue response;
        response["status"] = "ok";

        res.body = response.dump() + "\n";
        res.add_header("Content-Type", "application/json");
        res.add_header("Cache-Control", "no-store");
        res.end();
    });
}

void HttpServer::registerPredictRoute(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/predict").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res) {
        Timer requestTimer;
        ActiveRequestGuard activeRequest(metricsReg);
        crow::json::rvalue reqData = crow::json::load(req.body);

        crow::response validationResponse = checkPrediction(reqData);
        if (validationResponse.code != 200) {
            metricsReg.recordValidationError();
            metricsReg.recordRequestFailure();

            crow::json::wvalue error;
            error["error"] = validationResponse.body;
            error["request_latency_ms"] = requestTimer.end();
            error["inference_latency_ms"] = nullptr;
            validationResponse.body = error.dump() + "\n";
            validationResponse.add_header("Content-Type", "application/json");

            res = std::move(validationResponse);
            res.end();
            return;
        }

        const std::string model = reqData["model"].s();
        const std::string version = reqData["version"].s();

        crow::json::wvalue response;

        if (!modelReg.checkModel(model)) {
            metricsReg.recordModelNotFoundError();
            metricsReg.recordRequestFailure();
            response["error"] = "Model does not exist";
            response["request_latency_ms"] = requestTimer.end();
            response["inference_latency_ms"] = nullptr;
            res.code = 404;
        } else if (!modelReg.checkVersion(model, version)) {
            metricsReg.recordVersionNotFoundError();
            metricsReg.recordRequestFailure();
            response["error"] = "Model version does not exist";
            response["request_latency_ms"] = requestTimer.end();
            response["inference_latency_ms"] = nullptr;
            res.code = 404;
        } else {
            //TODO add support for models with multiple inputs
            //they pass in a vector of inputs in crow req
            PredictionRequest::TensorInput input;
            input.name = reqData["inputs"]["name"].s();
            for(const auto& shapeVal : reqData["inputs"]["shape"]){
                input.shape.push_back(shapeVal.u());
            }
            for(const auto& dataVal : reqData["inputs"]["data"]){
                input.shape.push_back(dataVal.d());
            }
            const auto creationTime = QueuedRequest::Clock::now();
            PredictionRequest predictionRequest{model, version, input};

            try {
                std::future<PredictionResponse> predictionFuture =
                    reqQueue.push(std::move(predictionRequest), creationTime);
                PredictionResponse predictionResponse = predictionFuture.get();

                response["model"] = predictionResponse.model;
                response["version"] = predictionResponse.version;
                response["output"]["shape"] = predictionResponse.output.shape;
                response["output"]["data"] = predictionResponse.output.data;
                response["inference_latency_ms"] = predictionResponse.inference_latency_ms;
                response["queue_wait_ms"] = predictionResponse.queue_wait_ms;
                response["batch_wait_ms"] = predictionResponse.batch_wait_ms;

                const double requestLatencyMs =
                    elapsedMilliseconds(creationTime, QueuedRequest::Clock::now());
                response["request_latency_ms"] = requestLatencyMs;
                metricsReg.recordRequestLatencyMs(requestLatencyMs);
                metricsReg.recordRequestSuccess();
                res.code = 200;
            } catch (...) {
                metricsReg.recordRequestFailure();
                response["error"] = "Prediction Failed";
                response["request_latency_ms"] =
                    elapsedMilliseconds(creationTime, QueuedRequest::Clock::now());
                response["inference_latency_ms"] = nullptr;
                res.code = 500;
            }
        }

        res.body = response.dump() + "\n";
        res.add_header("Content-Type", "application/json");
        res.end();
    });
}

void HttpServer::registerModelRoute(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/models")
    ([this](crow::response& res) {
        const std::vector<ModelSummary> models = modelReg.getAvailableModels();

        crow::json::wvalue::list modelsJson;
        modelsJson.reserve(models.size());
        for (const ModelSummary& model : models) {
            crow::json::wvalue modelJson;
            modelJson["name"] = model.name;
            modelJson["versions"] = model.versions;
            modelsJson.emplace_back(std::move(modelJson));
        }

        crow::json::wvalue response;
        response["models"] = std::move(modelsJson);
        res.body = response.dump() + "\n";
        res.add_header("Content-Type", "application/json");
        res.end();
    });
}

crow::json::wvalue HttpServer::buildMetricsResponse() const {
    crow::json::wvalue response;

    response["requests"]["total"] = metricsReg.getRequestsTotal();
    response["requests"]["successful"] = metricsReg.getRequestsSuccessful();
    response["requests"]["failed"] = metricsReg.getRequestsFailed();
    response["requests"]["active"] = metricsReg.getActiveRequests();
    response["predictions"]["total"] = metricsReg.getPredictionsTotal();
    response["models"]["registered_total"] = metricsReg.getModelsRegisteredTotal();

    response["latency_ms"]["request"]["avg"] = metricsReg.getAverageRequestLatencyMs();
    response["latency_ms"]["request"]["p50"] = metricsReg.getRequestP50LatencyMs();
    response["latency_ms"]["request"]["p95"] = metricsReg.getRequestP95LatencyMs();
    response["latency_ms"]["inference"]["avg"] = metricsReg.getAverageInferenceLatencyMs();
    response["latency_ms"]["inference"]["p50"] = metricsReg.getInferenceP50LatencyMs();
    response["latency_ms"]["inference"]["p95"] = metricsReg.getInferenceP95LatencyMs();
    response["latency_ms"]["queue_wait"]["avg"] = metricsReg.getAverageQueueWaitMs();
    response["latency_ms"]["queue_wait"]["p50"] = metricsReg.getQueueWaitP50Ms();
    response["latency_ms"]["queue_wait"]["p95"] = metricsReg.getQueueWaitP95Ms();
    response["latency_ms"]["batch_wait"]["avg"] = metricsReg.getAverageBatchWaitMs();
    response["latency_ms"]["batch_wait"]["p50"] = metricsReg.getBatchWaitP50Ms();
    response["latency_ms"]["batch_wait"]["p95"] = metricsReg.getBatchWaitP95Ms();

    response["batching"]["batches_total"] = metricsReg.getBatchesTotal();
    response["batching"]["requests_batched_total"] = metricsReg.getRequestsBatchedTotal();
    response["batching"]["average_batch_size"] = metricsReg.getAverageBatchSize();
    response["batching"]["max_observed_batch_size"] = metricsReg.getMaxObservedBatchSize();

    response["errors"]["validation_errors_total"] = metricsReg.getValidationErrorsTotal();
    response["errors"]["model_not_found_errors_total"] = metricsReg.getModelNotFoundErrorsTotal();
    response["errors"]["version_not_found_errors_total"] = metricsReg.getVersionNotFoundErrorsTotal();
    response["errors"]["runtime_errors_total"] = metricsReg.getRuntimeErrorsTotal();

    return response;
}

void HttpServer::registerMetricsRoute(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/metrics")
    ([this](crow::response& res) {
        crow::json::wvalue response = buildMetricsResponse();
        res.body = response.dump() + "\n";
        res.add_header("Content-Type", "application/json");
        res.end();
    });
}

void HttpServer::registerModelRegisterRoute(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/models/register").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res) {
        crow::json::rvalue reqData = crow::json::load(req.body);
        crow::response formatCheck = checkModelRegister(reqData);
        if (formatCheck.code != 200) {
            res = std::move(formatCheck);
            res.end();
            return;
        }

        const std::string name = reqData["name"].s();
        const std::string version = reqData["version"].s();
        
        const std::string path = reqData["path"].s();
        auto runtime = std::make_unique<OnnxRuntime>(path);

        //make the TensorInput
        std::vector<PredictionRequest::TensorInput> tensors;
        tensors.reserve(reqData["inputs"].size());
        for(const auto& input : reqData["inputs"]){
            std::string name = input["name"].s();

            std::vector<std::uint64_t> inputShapes;
            for(const auto& val : input["shape"]){
                inputShapes.push_back(val.u());
            }

            std::vector<float> inputData;
            for(const auto& val : input["data"]){
                inputData.push_back(val.d());
            }


            tensors.emplace_back(name, inputShapes, inputData);
        }
        

        if (!modelReg.addModel(name, version, path, std::move(runtime), tensors)) {
            res.code = 400;
            res.body = "Model already added\n";
            res.end();
            return;
        }

        metricsReg.recordModelRegistration();
        res.code = 200;
        res.body = "Model Added Successfully\n";
        res.end();
    });
}

void HttpServer::registerModelLoadRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/models/load").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res){
        crow::json::rvalue reqData = crow::json::load(req.body);
        crow::response formatCheck = checkModelLoad(reqData); //requires same fields as load
        if (formatCheck.code != 200) {
            res = std::move(formatCheck);
            res.end();
            return;
        }

        const std::string name = reqData["name"].s();
        const std::string version = reqData["version"].s();
        std::string loadResponse;
        if(!this->modelReg.loadModel(name, version, loadResponse)){
            //error occured
            res = crow::response(400, loadResponse);
            res.end();
            return;
        }
        res = crow::response(200, "Model successfully loaded.");
        res.end();
        return;
    });
}

void HttpServer::registerModelUnloadRoute(crow::SimpleApp& app){
    CROW_ROUTE(app, "/models/unload").methods(crow::HTTPMethod::POST)
    ([this](crow::request& req, crow::response& res){
        crow::json::rvalue reqData = crow::json::load(req.body);
        crow::response formatCheck = checkModelLoad(reqData); //requires same fields as load
        if (formatCheck.code != 200) {
            res = std::move(formatCheck);
            res.end();
            return;
        }

        const std::string name = reqData["name"].s();
        const std::string version = reqData["version"].s();
        std::string loadResponse;
        if(!this->modelReg.loadModel(name, version, loadResponse)){
            //error occured
            res = crow::response(400, loadResponse);
            res.end();
            return;
        }
        res = crow::response(200, "Model successfully unloaded.");
        res.end();
        return;
    });
}

crow::response HttpServer::checkPrediction(crow::json::rvalue& reqData) {
    if (!reqData) {
        return crow::response(400, "Invalid JSON Body");
    }

    if (!reqData.has("model")) {
        return crow::response(400, "Missing required field: model");
    }
    if (!reqData.has("version")) {
        return crow::response(400, "Missing required field: version");
    }
    //TODO add support for multiple inputs
    if (!reqData.has("input")) {
        return crow::response(400, "Missing required field: input");
    }

    if (reqData["model"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'model' must be a string");
    }
    if (reqData["version"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'version' must be a string");
    }
    if (reqData["input"].t() != crow::json::type::List) {
        return crow::response(400, "Field 'input' must be a list");
    }

    //TODO clean this up
    if (!reqData["input"].has("name")) {
        return crow::response(400, "Missing required field in input: name");
    }
    if (!reqData["input"].has("shape")) {
        return crow::response(400, "Missing required field in input: shape");
    }
    if (!reqData.has("input")) {
        return crow::response(400, "Missing required field in input: data");
    }

    if (reqData["input"]["name"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'input/name' must be a string");
    }if (reqData["input"]["shape"].t() != crow::json::type::List) {
        return crow::response(400, "Field 'input/shape' must be a list");
    }if (reqData["input"]["data"].t() != crow::json::type::List) {
        return crow::response(400, "Field 'input/data' must be a list");
    }

    //TODO check for list type




    return crow::response(200, "Well Formatted");
}

crow::response HttpServer::checkModelRegister(crow::json::rvalue& reqData) {
    if (!reqData) {
        return crow::response(400, "Invalid JSON Body");
    }
    if (!reqData.has("name")) {
        return crow::response(400, "Missing required field: name");
    }
    if (!reqData.has("version")) {
        return crow::response(400, "Missing required field: version");
    }
     if (!reqData.has("path")) {
        return crow::response(400, "Missing required field: path");
    }
    if (!reqData.has("inputs")) {
        return crow::response(400, "Missing required field: inputs");
    }
    if (!reqData.has("outputs")) {
        return crow::response(400, "Missing required field: outputs");
    }
    if (reqData["name"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'name' must be a string");
    }
    if (reqData["version"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'version' must be a string");
    }
    if (reqData["path"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'path' must be a string");
    }
    if (reqData["inputs"].t() != crow::json::type::List) {
        return crow::response(400, "Field 'inputs' must be a list");
    }
    if (reqData["outputs"].t() != crow::json::type::List) {
        return crow::response(400, "Field 'outputs' must be a list");
    }
    if (reqData["name"].size() == 0) {
        return crow::response(400, "Field name must be non-empty");
    }if (reqData["version"].size() == 0) {
        return crow::response(400, "Field version must be non-empty");
    }if (reqData["path"].size() == 0) {
        return crow::response(400, "Field path must be non-empty");
    } if(reqData["inputs"].size() == 0) {
        return crow::response(400, "Field inputs must be non-empty");
    }if(reqData["outputs"].size() == 0) {
        return crow::response(400, "Field outputs must be non-empty");
    }

    //go through inputs
    for(const auto& input : reqData["inputs"]){
        if (!input.has("name")) {
            return crow::response(400, "Missing required field: inputs/name");
        }if (!input.has("shape")) {
            return crow::response(400, "Missing required field: inputs/shape");
        }if (!input.has("data")) {
            return crow::response(400, "Missing required field: inputs/data");
        }

        if(input["name"].t() != crow::json::type::String) {
            return crow::response(400, "Field 'inputs/name' must be a string");
        } if(input["shape"].t() != crow::json::type::List) {
            return crow::response(400, "Field 'inputs/shape' must be a list");
        }

        if(input["name"].size() == 0) {
            return crow::response(400, "Field inputs/name must be non-empty");
        } if(input["shape"].size() == 0) {
            return crow::response(400, "Field inputs/shape must be non-empty");
        }
    } 
    //go through outputs
    for(const auto& output : reqData["outputs"]){
        if (!output.has("name")) {
            return crow::response(400, "Missing required field: inputs/name");
        }if (!output.has("shape")) {
            return crow::response(400, "Missing required field: inputs/shape");
        }

        if(input["name"].t() != crow::json::type::String) {
            return crow::response(400, "Field 'inputs/name' must be a string");
        } if(input["shape"].t() != crow::json::type::List) {
            return crow::response(400, "Field 'inputs/shape' must be a list");
        } if(input["data"].t() != crow::json::type::List) {
            return crow::response(400, "Field 'inputs/data' must be a list");
        }

        if(input["name"].size() == 0) {
            return crow::response(400, "Field inputs/name must be non-empty");
        } if(input["shape"].size() == 0) {
            return crow::response(400, "Field inputs/shape must be non-empty");
        } if(input["data"].size() == 0) {
            return crow::response(400, "Field inputs/data must be non-empty");
        }
    }


    return crow::response(200, "Well formatted");
}