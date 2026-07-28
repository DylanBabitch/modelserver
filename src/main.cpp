#include <iostream>
#include "crow.h"
#include "server/HttpServer.hpp"

int main(){
    HttpServer h;
    h.run();
    return 0;
}