#pragma once

#include <string>

struct PredicitionRequest {
    std::string model;
    std::string version;
    std::string input;
};