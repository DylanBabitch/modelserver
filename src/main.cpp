#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"
#include "model/ModelRegistry.hpp"
#include "metrics/MetricsRegistry.hpp"
#include "scheduler/BatchManager.hpp"
#include "scheduler/RequestQueue.hpp"
#include "scheduler/WorkerPool.hpp"

#include <algorithm>
#include <cstdint>
#include <thread>

int main(){
    ModelRegistry modelReg;
    MetricsRegistry metricsReg;
    RequestQueue reqQueue;
    BatchManager batchManager(reqQueue, metricsReg);

    std::size_t numThreads = std::max(std::thread::hardware_concurrency(), 1u);
    WorkerPool workerPool(numThreads, batchManager, modelReg, metricsReg);
    workerPool.start();
    
    HttpServer h(modelReg, metricsReg, reqQueue);
    h.run();
    workerPool.stop();
    return 0;
}
