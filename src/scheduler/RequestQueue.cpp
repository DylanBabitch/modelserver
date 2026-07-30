#include "scheduler/RequestQueue.hpp"
#include "scheduler/QueuedRequest.hpp"
#include <chrono>
#include <optional>
#include <mutex>

void RequestQueue::push(PredictionRequest request, std::chrono::steady_clock::time_point creationTime){
    //Make the Queued Request
    std::unique_lock<std::mutex> lock(mtx);

    requests.emplace(currId, request, creationTime, 
                    std::chrono::steady_clock::now(), 
                    std::nullopt, std::nullopt,RequestStatus::Queued, 
                    std::nullopt, std::promise<PredictionResponse>{});
    ++currId;
    lock.unlock();
    cv.notify_one();
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
    shutdown = true;
    lock.unlock();
    cv.notify_all();
}

bool RequestQueue::isShutdown() const{
    std::lock_guard<std::mutex> lock(mtx);
    return shutdown;
}

