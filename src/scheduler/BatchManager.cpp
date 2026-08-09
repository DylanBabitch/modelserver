#include "scheduler/BatchManager.hpp"
#include "scheduler/RequestQueue.hpp"

#include <cstddef>
#include <optional>
#include <mutex>

BatchManager::BatchManager(RequestQueue& reqQueue, std::size_t maxBatchSize = 8, std::chrono::milliseconds batchTimeoutMs = std::chrono::milliseconds(10)) 
                            : reqQueue(reqQueue), maxBatchSize(maxBatchSize), batchTimeoutMs(batchTimeoutMs) {}

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
}

void BatchManager::shutdown(){
    isShutdown = true;
}

std::size_t BatchManager::getMaxBatchSize() const{
    std::lock_guard<std::mutex> lock(mtx);
    return maxBatchSize;
}

std::chrono::milliseconds BatchManager::getBatchTimeout() const{
    std::lock_guard<std::mutex> lock(mtx);
    return batchTimeoutMs;
}