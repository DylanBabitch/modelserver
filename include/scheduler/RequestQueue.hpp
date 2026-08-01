#pragma once
#include "scheduler/QueuedRequest.hpp"
#include "request/PredictionRequest.hpp"

#include <cstddef>
#include <queue>

class RequestQueue{
private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<QueuedRequest> requests;
    std::uint64_t currId = 0;
    bool shutdown = false;
public:
    std::future<PredictionResponse> push(PredictionRequest request, std::chrono::steady_clock::time_point creationTime);
    std::optional<QueuedRequest> pop();
    std::size_t size() const;
    bool empty() const;
    void setShutdown();
    bool isShutdown() const;
};