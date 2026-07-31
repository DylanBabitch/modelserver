#include "model/ModelRegistry.hpp"
#include <string>
#include <unordered_map>
#include <stdexcept>

bool ModelRegistry::checkModel(const std::string& modelName) const{
    return this->availableModels.contains(modelName);
}

bool ModelRegistry::checkVersion(const std::string& modelName, const std::string& version) const{
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()) return false;

    for(const ModelInfo& info : it->second){
        if(info.modelVersion == version) return true;
    }
    return false;
}

bool ModelRegistry::addModel(const std::string& modelName, const std::string& version, const std::string& path, std::unique_ptr<ModelRuntime>&& model){
    if(!model){ //model is nullptr
        return false;
    }
    if(checkVersion(modelName, version)) return false;
    this->availableModels[modelName].emplace_back(version, path, std::move(model));
    return true;
}

const std::unordered_map<std::string, std::vector<ModelInfo>>& ModelRegistry::getAvailableModels() const{
    return  availableModels;
}

const std::vector<ModelInfo>* ModelRegistry::getAvailableVersions(const std::string& modelName) const{
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        return nullptr;
    }
    return &it->second;
}

ModelRuntime* ModelRegistry::getRuntime(const std::string& modelName, const std::string& modelVersion) const {
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        return nullptr;
    }

    for(const ModelInfo& mInfo : it->second){
        if(mInfo.modelVersion == modelVersion){
            return mInfo.runtime.get();
        }
    }

    return nullptr;
    
    
}