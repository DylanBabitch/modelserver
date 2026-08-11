#include "scheduler/WorkerPool.hpp"

#include "model/ModelRuntime.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

double elapsedMilliseconds(
    const QueuedRequest::Clock::time_point start,
    const QueuedRequest::Clock::time_point finish)
{
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

void failBatch(PredictionBatch& batch, const std::string& error_message) {
    const std::exception_ptr error = std::make_exception_ptr(std::runtime_error(error_message));
    const auto finish_time = QueuedRequest::Clock::now();

    for (QueuedRequest& request : batch.requests) {
        request.status = QueuedRequest::RequestStatus::Failed;
        request.errorMessage = error_message;
        request.finishTime = finish_time;
        request.resultPromise.set_exception(error);
    }
}

} // namespace

WorkerPool::WorkerPool(
    std::size_t numWorkers,
    BatchManager& batchManager,
    ModelRegistry& modelReg,
    MetricsRegistry& metricsReg)
    : batchManager(batchManager), modelReg(modelReg), metricsReg(metricsReg), workerCount(numWorkers)
{
    if (numWorkers == 0) {
        throw std::runtime_error("numWorkers must be at least 1");
    }
    workers.reserve(numWorkers);
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::start() {
    std::unique_lock<std::mutex> lock(mtx);

    if (started || shutdown) {
        return;
    }

    try {
        for (std::size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back(&WorkerPool::workerLoop, this);
        }
        started = true;
    } catch (...) {
        shutdown = true;
        lock.unlock();
        batchManager.shutdown();

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        throw;
    }
}

void WorkerPool::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    if (shutdown || !started) {
        return;
    }

    batchManager.shutdown();
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    shutdown = true;
}

void WorkerPool::workerLoop() {
    while (!shutdown) {
        std::optional<PredictionBatch> next_batch = batchManager.getBatch();
        if (!next_batch.has_value()) {
            return;
        }

        PredictionBatch& batch = *next_batch;
        std::vector<PredictionRequest> requests;
        std::vector<double> queue_waits_ms;
        requests.reserve(batch.requests.size());
        queue_waits_ms.reserve(batch.requests.size());

        for (QueuedRequest& request : batch.requests) {
            const auto start_time = QueuedRequest::Clock::now();
            request.startTime = start_time;
            request.status = QueuedRequest::RequestStatus::Running;

            const double queue_wait_ms = elapsedMilliseconds(request.creationTime, start_time);
            queue_waits_ms.push_back(queue_wait_ms);
            metricsReg.recordQueueWaitMs(queue_wait_ms);
            requests.push_back(request.request);
        }

        ModelRuntime* runtime = modelReg.getRuntime(batch.model, batch.version);
        if (runtime == nullptr) {
            failBatch(batch, "model/version runtime not found");
            continue;
        }

        try {
            const auto inference_start = QueuedRequest::Clock::now();
            std::vector<PredictionResponse> responses = runtime->predictBatch(requests);
            const auto inference_finish = QueuedRequest::Clock::now();
            const double inference_latency_ms = elapsedMilliseconds(inference_start, inference_finish);

            if (responses.size() != batch.requests.size()) {
                throw std::runtime_error("runtime returned a response count that does not match the batch size");
            }

            const double batch_wait_ms = elapsedMilliseconds(batch.creationTime, batch.dispatchTime);
            for (std::size_t i = 0; i < batch.requests.size(); ++i) {
                PredictionResponse& response = responses[i];
                QueuedRequest& request = batch.requests[i];

                response.inference_latency_ms = inference_latency_ms;
                response.queue_wait_ms = queue_waits_ms[i];
                response.batch_wait_ms = batch_wait_ms;
                request.status = QueuedRequest::RequestStatus::Completed;
                request.finishTime = QueuedRequest::Clock::now();
                request.resultPromise.set_value(std::move(response));
                metricsReg.recordInferenceLatencyMs(inference_latency_ms);
            }
            metricsReg.addPredictions(batch.requests.size());
        } catch (...) {
            metricsReg.recordRuntimeError();
            failBatch(batch, "runtime predictBatch failed");
        }
    }
}

std::size_t WorkerPool::totalWorkers() const {
    return workerCount;
}
