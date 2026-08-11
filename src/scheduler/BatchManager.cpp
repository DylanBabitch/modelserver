#include "scheduler/BatchManager.hpp"
#include "scheduler/RequestQueue.hpp"
#include "metrics/MetricsRegistry.hpp"

#include <cstddef>
#include <optional>
#include <mutex>

namespace {

double elapsedMilliseconds(
    const std::chrono::steady_clock::time_point start,
    const std::chrono::steady_clock::time_point finish)
{
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

} // namespace

BatchManager::BatchManager(RequestQueue& reqQueue, MetricsRegistry& metricsReg, std::size_t maxBatchSize, std::chrono::milliseconds batchTimeoutMs)
    : maxBatchSize(maxBatchSize), batchTimeoutMs(batchTimeoutMs), reqQueue(reqQueue), metricsReg(metricsReg) {}

bool BatchManager::isCompatible(const PredictionBatch& batch, const QueuedRequest& nextReq) const {
    return batch.model == nextReq.request.model && batch.version == nextReq.request.version;
}

std::optional<PredictionBatch> BatchManager::getBatch(){
    std::unique_lock<std::mutex> managerLock(mtx);
    PredictionBatch batch;

    auto firstReq = reqQueue.popBlocking();

    if(!firstReq.has_value()){
        return std::nullopt;
    }

    batch.creationTime = PredictionBatch::Clock::now();
    batch.model = firstReq->request.model;
    batch.version = firstReq->request.version;
    batch.requests.push_back(std::move(*firstReq));
    const auto deadline = batch.creationTime + batchTimeoutMs;

    while(batch.requests.size() < maxBatchSize){
        auto now = PredictionBatch::Clock::now();

        if(now >= deadline){
            break;
        }

        auto maybeNext = reqQueue.peakUntil(deadline);

        if(!maybeNext.has_value()){
            break;
        }

        if(!isCompatible(batch, maybeNext->get())){
            break;
        }

        auto nextRequest = reqQueue.popNonBlocking();

        if(!nextRequest.has_value()){
            break;
        }
        batch.requests.push_back(std::move(*nextRequest));
    }

    batch.dispatchTime = PredictionBatch::Clock::now();
    metricsReg.recordBatch(batch.requests.size());
    metricsReg.recordBatchWaitMs(elapsedMilliseconds(batch.creationTime, batch.dispatchTime));
    return batch;
}

void BatchManager::shutdown(){
    isShutdown = true;
    reqQueue.setShutdown();
}

std::size_t BatchManager::getMaxBatchSize() const{
    std::lock_guard<std::mutex> lock(mtx);
    return maxBatchSize;
}

std::chrono::milliseconds BatchManager::getBatchTimeout() const{
    std::lock_guard<std::mutex> lock(mtx);
    return batchTimeoutMs;
}
