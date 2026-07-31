#include "metrics/Timer.hpp"
Timer::Timer(){
    start_time = std::chrono::steady_clock::now();
}

double Timer::end() const{
    std::chrono::duration<double, std::milli> duration = std::chrono::steady_clock::now() - start_time;
    return duration.count();
}