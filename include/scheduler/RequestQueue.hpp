#pragma once
#include "scheduler/QueuedRequest.hpp"
#include "request/PredictionRequest.hpp"

#include <cstddef>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <optional>
#include <chrono>
#include <cstdint>

class RequestQueue{
private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<QueuedRequest> requests;
    std::uint64_t currId = 0;
    bool shutdown = false;
public:
    std::future<PredictionResponse> push(PredictionRequest request);
    std::optional<QueuedRequest> pop();
    std::size_t size() const;
    bool empty() const;
    void setShutdown();
    bool isShutdown() const;
};