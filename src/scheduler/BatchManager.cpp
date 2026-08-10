#include "scheduler/BatchManager.hpp"
#include "scheduler/RequestQueue.hpp"

#include <cstddef>
#include <optional>
#include <mutex>

BatchManager::BatchManager(RequestQueue& reqQueue, std::size_t maxBatchSize = 8, std::chrono::milliseconds batchTimeoutMs = std::chrono::milliseconds(10)) 
                            : reqQueue(reqQueue), maxBatchSize(maxBatchSize), batchTimeoutMs(batchTimeoutMs) {}

bool BatchManager::isCompatiable(PredictionBatch& batch, QueuedRequest& nextReq){
    return batch.model == nextReq.request.model && batch.version == nextReq.request.version;
}

std::optional<PredictionBatch> BatchManager::getBatch(){
    PredictionBatch batch;

    auto firstReq = reqQueue.popBlocking();

    if(!firstReq.has_value()){
        return std::nullopt;
    }

    batch.creationTime = Clock::now();
    batch.model = firstReq->request.model;
    batch.version = firstReq->request.version;
    batch.requests.push_back(std::move(*firstReq));

    const auto deadline = batch.creationTime + batchTimeoutMs;

    while(batch.requests.size() < maxBatchSize){
        auto now = Clock::now();

        if(now >= deadline){
            break;
        }

        auto maybeNext = reqQueue.peakUntil(deadline);

        if(!maybeNext.has_value()){
            break;
        }

        if(!isCompatiable(batch, *maybeNext)){
            break;
        }

        auto nextRequest = reqQueue.popNonBlocking();

        if(!nextRequest.has_value()){
            break;
        }

        batch.requests.push_back(std::move(*nextRequest));
    }
    batch.dispatchTime = Clock::now();
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