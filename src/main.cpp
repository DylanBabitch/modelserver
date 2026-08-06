#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"
#include "model/ModelRegistry.hpp"
#include "metrics/MetricsRegistry.hpp"

int main(){
//     ModelRegistry modelReg;
//     MetricsRegistry metricReg;
    HttpServer h;
    h.run();
    return 0;
}