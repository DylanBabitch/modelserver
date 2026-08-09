#pragma once

#include "scheduler/RequestQueue.hpp"
#include "scheduler/QueuedRequest.hpp"

#include <cstddef>
#include <queue>
#include <optional>
#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

#define Clock std::chrono::steady_clock

struct PredictionBatch {
    std::uint64_t batchId;

    std::string model;
    std::string version;

    std::vector<QueuedRequest> requests;

    Clock::time_point creationTime;
    Clock::time_point dispatchTime;
};

class BatchManager{
private:
    std::size_t maxBatchSize; //TODO test this param
    std::chrono::milliseconds batchTimeoutMs; //TODO test this param
    RequestQueue& reqQueue;
    bool isShutdown = false;
    void managerLoop();
    mutable std::mutex mtx;
    bool isCompatiable(PredictionBatch& batch, QueuedRequest& nextReq);
public:
    

    BatchManager(RequestQueue& reqQueue, std::size_t maxBatchSize = 8, std::chrono::milliseconds batchTimeoutMs = std::chrono::milliseconds(10));
    std::optional<PredictionBatch> getBatch();
    void shutdown();
    std::size_t getMaxBatchSize() const;
    std::chrono::milliseconds getBatchTimeout() const;
};