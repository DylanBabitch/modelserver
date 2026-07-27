#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"

int main(){
    HttpServer::run();
    return 0;
}