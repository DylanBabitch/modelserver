#pragma once

#include <model/ModelRegistry.hpp>
#include <metrics/MetricsRegistry.hpp>
#include <scheduler/RequestQueue.hpp>

#include <vector>
#include <cstddef>
#include <thread>
#include <atomic>

class WorkerPool{
private:
    RequestQueue* reqQueue;
    ModelRegistry* modelReg;
    MetricsRegistry* metricsReg;

    std::size_t workerCount;
    std::vector<std::thread> workers;

    std::atomic<bool> shutdown = false;

    void workerLoop(int workerId);
public:
    WorkerPool(std::size_t numWorkers, RequestQueue* reqQueue, ModelRegistry* modelReg, MetricsRegistry* metricsReg);
    void start();
    void stop();

    bool isRunning() const;
    std::size_t size();
};