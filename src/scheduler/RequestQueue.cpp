#include "scheduler/RequestQueue.hpp"

#include <stdexcept>
#include <future>
#include <cstdint>
#include <optional>

std::future<PredictionResponse> RequestQueue::push(PredictionRequest request, std::chrono::steady_clock::time_point creationTime){
    //check shutdown
    //Make the Queued Request
    std::unique_lock<std::mutex> lock(mtx);

    if(shutdown){
        std::promise<PredictionResponse> failedProm;
        failedProm.set_exception(std::make_exception_ptr(std::runtime_error("Request queue is shutdown.")));
        return failedProm.get_future();
    }

    std::promise<PredictionResponse> prom;
    auto future = prom.get_future();

    requests.emplace(currId, std::move(request), creationTime, 
                    std::chrono::steady_clock::now(), 
                    std::nullopt, std::nullopt,QueuedRequest::RequestStatus::Queued, 
                    std::nullopt, std::move(prom));
    ++currId;
    lock.unlock();
    cv.notify_one();
    return future;
}

std::optional<QueuedRequest> RequestQueue::pop(){
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [this]{return (shutdown || !requests.empty());});

    if (shutdown && requests.empty()){
        return std::nullopt;
    }
    
    QueuedRequest request = std::move(requests.front());
    requests.pop();

    return request;
}

size_t RequestQueue::size() const{
    std::lock_guard<std::mutex> lock(mtx);
    return requests.size();

}

bool RequestQueue::empty() const{
    std::lock_guard<std::mutex> lock(mtx);
    return requests.empty();
}

void RequestQueue::setShutdown(){
    std::unique_lock<std::mutex> lock(mtx);
    if(shutdown) return;
    shutdown = true;
    lock.unlock();
    cv.notify_all();
}

bool RequestQueue::isShutdown() const{
    std::lock_guard<std::mutex> lock(mtx);
    return shutdown;
}

