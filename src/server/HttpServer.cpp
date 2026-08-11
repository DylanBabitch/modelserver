#include "server/HttpServer.hpp"

#include "metrics/Timer.hpp"
#include "model/DummyRuntime.hpp"
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
            const auto creationTime = QueuedRequest::Clock::now();
            PredictionRequest predictionRequest{model, version, reqData["input"].s()};

            try {
                std::future<PredictionResponse> predictionFuture =
                    reqQueue.push(std::move(predictionRequest), creationTime);
                PredictionResponse predictionResponse = predictionFuture.get();

                response["model"] = predictionResponse.model;
                response["version"] = predictionResponse.version;
                response["prediction"] = predictionResponse.prediction;
                response["confidence"] = predictionResponse.confidence;
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
        auto runtime = std::make_unique<DummyRuntime>();

        if (!modelReg.addModel(name, version, "dummypath/", std::move(runtime))) {
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
        crow::response formatCheck = checkModelRegister(reqData); //requires same fields as load
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
        crow::response formatCheck = checkModelRegister(reqData); //requires same fields as load
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
    if (!reqData.has("input")) {
        return crow::response(400, "Missing required field: input");
    }

    if (reqData["model"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'model' must be a string");
    }
    if (reqData["version"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'version' must be a string");
    }
    if (reqData["input"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'input' must be a string");
    }

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
    if (reqData["name"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'name' must be a string");
    }
    if (reqData["version"].t() != crow::json::type::String) {
        return crow::response(400, "Field 'version' must be a string");
    }
    if (reqData["name"].size() == 0) {
        return crow::response(400, "Field name must be non-empty");
    }
    if (reqData["version"].size() == 0) {
        return crow::response(400, "Field version must be non-empty");
    }

    return crow::response(200, "Well formatted");
}