#include "scheduler/WorkerPool.hpp"
#include "model/ModelRuntime.hpp"

#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>

WorkerPool::WorkerPool(std::size_t numWorkers, std::shared_ptr<RequestQueue> reqQueue, std::shared_ptr<ModelRegistry> modelReg){
    if(numWorkers == 0){
        throw std::runtime_error("numWorkers must be at least 1");
    } else if(!reqQueue){
        throw std::runtime_error("reqQueue must point to a valid RequestQueue");
    } else if(!modelReg){
        throw std::runtime_error("modelReg must point to a valid ModelRegistry");
    }


    workers.reserve(numWorkers);
    this->workerCount = numWorkers;
    this->reqQueue = reqQueue;
    this->modelReg = modelReg;
}

WorkerPool::~WorkerPool(){
    this->stop();
}

void WorkerPool::start(){
    std::lock_guard<std::mutex> lock(mtx);
    if(started || shutdown) return;
    //creating thread
    started = true;
    for(std::size_t i = 0; i < workerCount; ++i){
        workers.emplace_back(&WorkerPool::workerLoop, this);
    }
    
}

void WorkerPool::stop(){
    std::lock_guard<std::mutex> lock(mtx);
    if(shutdown || !started){
        return;
    }
    reqQueue->setShutdown();
    for(auto& thread : workers){
        thread.join();
    }
    shutdown = true;
}

void WorkerPool::workerLoop(){
    while(!shutdown){
        //check if the next req exists
        auto nextReq = reqQueue->pop();

        if(!nextReq){
            return;
        }

        nextReq->status = QueuedRequest::RequestStatus::Running;
        nextReq->processingStartTime = std::chrono::steady_clock::now();

        //process the request
        std::string model = nextReq->request.model;
        std::string version = nextReq->request.version;

        ModelRuntime* modelRuntime = modelReg->getRuntime(model, version);
        if(!modelRuntime){
            nextReq->status = QueuedRequest::RequestStatus::Failed;
            nextReq->errorMessage = "model/version runtime not found.";
            nextReq->resultPromise.set_exception(std::make_exception_ptr(std::runtime_error("model/version runtime not found.")));
            nextReq->finishTime = std::chrono::steady_clock::now();
            continue;
        }
        try{
            PredictionResponse resp = modelRuntime->predict(nextReq->request);
            nextReq->status = QueuedRequest::RequestStatus::Completed;
            nextReq->finishTime = std::chrono::steady_clock::now();
            nextReq->resultPromise.set_value(resp);
        } catch (...){
            nextReq->status = QueuedRequest::RequestStatus::Failed;
            nextReq->errorMessage = "runtime predict failed.";
            nextReq->finishTime = std::chrono::steady_clock::now();
            nextReq->resultPromise.set_exception(std::make_exception_ptr(std::runtime_error("runtime predict failed.")));
        }
        
    }
}


std::size_t WorkerPool::totalWorkers() const{
    return workerCount;
}