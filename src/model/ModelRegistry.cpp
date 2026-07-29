#include "model/ModelRegistry.hpp"
#include <string>
#include <unordered_map>

bool ModelRegistry::checkModel(std::string modelName){
    return this->availableModels.contains(modelName);
}

bool ModelRegistry::checkVersion(std::string modelName, std::string version){
    if(!this->availableModels.contains(modelName)) return false;

    for(std::string availableVersion : this->availableModels[modelName]){
        if(availableVersion == version) return true;
    }
    return false;
}

bool ModelRegistry::addModel(std::string modelName, std::string version){
    if(checkVersion(modelName, version)) return false;
    this->availableModels[modelName].push_back(version);
    return true;
}

std::unordered_map<std::string, std::vector<std::string>>& ModelRegistry::getAvailableModels(){
    return availableModels;
}