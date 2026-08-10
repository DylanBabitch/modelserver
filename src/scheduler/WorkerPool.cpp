#include "scheduler/WorkerPool.hpp"
#include "model/ModelRuntime.hpp"
#include <scheduler/BatchManager.hpp>

#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>

WorkerPool::WorkerPool(std::size_t numWorkers, BatchManager& batchManager, ModelRegistry& modelReg) : batchManager(batchManager), modelReg(modelReg){
    if(numWorkers == 0){
        throw std::runtime_error("numWorkers must be at least 1");
    }
    workers.reserve(numWorkers);
    this->workerCount = numWorkers;
}

WorkerPool::~WorkerPool(){
    this->stop();
}

void WorkerPool::start(){
    std::unique_lock<std::mutex> lock(mtx);

    if(started || shutdown) return;

    //creating thread
    try{
        for(std::size_t i = 0; i < workerCount; ++i){
            workers.emplace_back(&WorkerPool::workerLoop, this);
        }
        started = true;
    } catch (...){
        shutdown = true;
        lock.unlock();
        batchManager.shutdown();

        for(std::thread& worker : workers){
            if(worker.joinable()){
                worker.join();
            }
        }
        throw;
    }
    
    
}

void WorkerPool::stop(){
    std::lock_guard<std::mutex> lock(mtx);
    if(shutdown || !started){
        return;
    }
    batchManager.shutdown();
    for(auto& thread : workers){
        thread.join();
    }
    shutdown = true;
}

void WorkerPool::workerLoop(){
    while(!shutdown){
        //check if the next req exists
        auto nextBatch = batchManager.getBatch();

        if(!nextBatch){
            return;
        }
        for(auto& req : nextBatch->requests){
            req.status = QueuedRequest::RequestStatus::Running;
            req.processingStartTime = std::chrono::steady_clock::now();

            //process the request
            std::string model = req.request.model;
            std::string version = req.request.version;

            ModelRuntime* modelRuntime = modelReg.getRuntime(model, version);
            if(!modelRuntime){
                req.status = QueuedRequest::RequestStatus::Failed;
                req.errorMessage = "model/version runtime not found.";
                req.resultPromise.set_exception(std::make_exception_ptr(std::runtime_error("model/version runtime not found.")));
                req.finishTime = std::chrono::steady_clock::now();
                continue;
            }
            try{
                PredictionResponse resp = modelRuntime->predict(req.request);
                req.status = QueuedRequest::RequestStatus::Completed;
                req.finishTime = std::chrono::steady_clock::now();
                req.resultPromise.set_value(resp);
            } catch (...){
                req.status = QueuedRequest::RequestStatus::Failed;
                req.errorMessage = "runtime predict failed.";
                req.finishTime = std::chrono::steady_clock::now();
                req.resultPromise.set_exception(std::make_exception_ptr(std::runtime_error("runtime predict failed.")));
            }
        }
    }
}


std::size_t WorkerPool::totalWorkers() const{
    return workerCount;
}