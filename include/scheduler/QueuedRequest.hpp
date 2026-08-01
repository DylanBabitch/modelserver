#pragma once
#include "request/PredictionRequest.hpp"
#include "request/PredictionResponse.hpp"
#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <optional>



struct QueuedRequest {
    enum struct RequestStatus{
        Queued,
        Running,
        Completed,
        Failed,
        Cancelled
    };

    using Clock = std::chrono::steady_clock;
    std::uint64_t requestId = 0;
    PredictionRequest request;
    Clock::time_point creationTime;
    Clock::time_point queueTime;
    std::optional<Clock::time_point> processingStartTime;
    std::optional<Clock::time_point> finishTime;
    RequestStatus status = RequestStatus::Queued;
    std::optional<std::string> errorMessage;
    std::promise<PredictionResponse> resultPromise;
};