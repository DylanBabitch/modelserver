#pragma once
#include "scheduler/QueuedRequest.hpp"
#include "request/PredictionRequest.hpp"
#include <queue>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class RequestQueue{
private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<QueuedRequest> requests;
    uint32_t currId = 0;
    bool shutdown = false;
public:
    void push(PredictionRequest request, std::chrono::steady_clock::time_point creationTime);
    std::optional<QueuedRequest> pop();
    size_t size() const;
    bool empty() const;
    void setShutdown();
    bool isShutdown() const;
};