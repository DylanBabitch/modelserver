#include "metrics/Timer.hpp"
#include <chrono>

Timer::Timer(){
    start_time = std::chrono::high_resolution_clock::now();
}

double Timer::end(){
    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start_time;
    return duration.count();
}