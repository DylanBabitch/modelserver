#pragma once

#include "scheduler/RequestQueue.hpp"
#include "scheduler/QueuedRequest.hpp"
#include "metrics/MetricsRegistry.hpp"

#include <cstddef>
#include <queue>
#include <optional>
#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

struct PredictionBatch {
    using Clock = std::chrono::steady_clock;

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
    MetricsRegistry& metricsReg;
    bool isShutdown = false;
    mutable std::mutex mtx;
    bool isCompatible(const PredictionBatch& batch, const QueuedRequest& nextReq) const;
public:
    

    BatchManager(RequestQueue& reqQueue, MetricsRegistry& metricsReg, std::size_t maxBatchSize = 8, std::chrono::milliseconds batchTimeoutMs = std::chrono::milliseconds(10));
    std::optional<PredictionBatch> getBatch();
    void shutdown();
    std::size_t getMaxBatchSize() const;
    std::chrono::milliseconds getBatchTimeout() const;
};
