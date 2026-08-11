#pragma once

#include <model/ModelRegistry.hpp>
#include <metrics/MetricsRegistry.hpp>
#include <scheduler/BatchManager.hpp>

#include <vector>
#include <cstddef>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

class WorkerPool{
private:
    BatchManager& batchManager;
    ModelRegistry& modelReg;
    MetricsRegistry& metricsReg;

    std::size_t workerCount;
    std::vector<std::thread> workers;

    std::atomic<bool> shutdown = false;
    bool started = false;

    std::mutex mtx;

    void workerLoop();
public:
    WorkerPool(std::size_t numWorkers, BatchManager& reqQueue, ModelRegistry& modelReg, MetricsRegistry& metricsReg);
    ~WorkerPool();
    void start();
    void stop();

    std::size_t totalWorkers() const;
};