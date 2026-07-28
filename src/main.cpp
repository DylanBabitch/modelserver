#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"
#include "model/ModelRegistry.hpp"

int main(){
    ModelRegistry reg;
    HttpServer h(&reg);
    h.run();
    return 0;
}