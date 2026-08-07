#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"
#include "model/ModelRegistry.hpp"
#include "metrics/MetricsRegistry.hpp"

#include <cstdint>
#include <thread>

int main(){
//     ModelRegistry modelReg;
//     MetricsRegistry metricReg;
    std::size_t numThreads = std::max(std::thread::hardware_concurrency(), 1u);
    HttpServer h(numThreads);
    h.run();
    return 0;
}