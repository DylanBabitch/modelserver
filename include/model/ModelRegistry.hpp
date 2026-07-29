#pragma once
#include <unordered_map>
#include <string>
#include <vector>

class ModelRegistry{
private:
    std::unordered_map<std::string, std::vector<std::string>> availableModels;
public:
    bool addModel(std::string modelName, std::string version);
    bool checkModel(std::string modelName);
    bool checkVersion(std::string modelName, std::string version);
    std::unordered_map<std::string, std::vector<std::string>>& availableModels();
};