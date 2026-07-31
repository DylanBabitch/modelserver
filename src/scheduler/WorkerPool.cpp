#include "scheduler/WorkerPool.hpp"
#include "model/ModelRuntime.hpp"

WorkerPool::WorkerPool(std::size_t numWorkers, RequestQueue* reqQueue, ModelRegistry* modelReg, MetricsRegistry* metricsReg){
    workers.reserve(numWorkers);
    this->workerCount = numWorkers;
    this->reqQueue = reqQueue;
    this->modelReg = modelReg;
    this->metricsReg = metricsReg;
}

void WorkerPool::start(){
    //creating thread
    for(std::size_t i = 0; i < workerCount; ++i){
        workers.emplace_back(&WorkerPool::workerLoop, this, i);
    }
}

void WorkerPool::stop(){
    shutdown = true;
    reqQueue->setShutdown();
    for(auto& thread : workers){
        thread.join();
    }
}

void WorkerPool::workerLoop(int workerId){
    while(true){
        //check if the next req exists
        auto nextReq = reqQueue->pop();

        if(!nextReq){
            return;
        }

        nextReq->status = QueuedRequest::RequestStatus::Running;

        //process the request
        std::string model = nextReq->request.model;
        std::string version = nextReq->request.version;
        std::string input = nextReq->request.input;
        
        if(!modelReg->checkVersion(model, version)){
            //model doesn't exist, idk waht to do yet
        }

        ModelRuntime* modelRuntime = modelReg->getRuntime(model, version);
        
        nextReq->resultPromise.set_value(modelRuntime->predict(nextReq->request));
    }
}