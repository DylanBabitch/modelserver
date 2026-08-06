#include "model/ModelRegistry.hpp"
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <mutex>
#include <crow.h>

bool ModelRegistry::checkModel(const std::string& modelName) const{
    std::lock_guard<std::mutex> lock(mtx);
    return this->availableModels.contains(modelName);
}

bool ModelRegistry::checkVersion(const std::string& modelName, const std::string& version) const{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()) return false;

    for(const ModelInfo& info : it->second){
        if(info.modelVersion == version) return true;
    }
    return false;
}

bool ModelRegistry::checkVersionLocked(const std::string& modelName, const std::string& version) const{
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()) return false;

    for(const ModelInfo& info : it->second){
        if(info.modelVersion == version) return true;
    }
    return false;
}

bool ModelRegistry::addModel(const std::string& modelName, const std::string& version, const std::string& path, std::unique_ptr<ModelRuntime>&& model){
    std::lock_guard<std::mutex> lock(mtx);
    if(!model){ //model is nullptr
        return false;
    }
    if(checkVersionLocked(modelName, version)) return false;
    this->availableModels[modelName].emplace_back(version, path, std::move(model));
    return true;
}

const std::vector<crow::json::wvalue> ModelRegistry::getAvailableModels() const{
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<crow::json::wvalue> modelOutput;
    modelOutput.reserve(availableModels.size());
    for(auto& [modelName, models] : availableModels){
        crow::json::wvalue temp;
        temp["name"] = modelName;
        std::vector<std::string> modelVersions;
        modelVersions.reserve(models.size());
        for(const ModelInfo& model : models){
            modelVersions.push_back(model.modelVersion);
        }
        temp["versions"] = modelVersions;
        modelOutput.push_back(temp);
    }
    return modelOutput;
}

ModelRuntime* ModelRegistry::getRuntime(const std::string& modelName, const std::string& modelVersion) const {
    std::lock_guard<std::mutex> lock(mtx);
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