#include "model/ModelRegistry.hpp"

#include <chrono>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <mutex>
#include <vector>

bool ModelRegistry::checkModel(const std::string& modelName) const{
    std::lock_guard<std::mutex> lock(mtx);
    return this->availableModels.contains(modelName);
}

bool ModelRegistry::checkVersion(const std::string& modelName, const std::string& version) const{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()) return false;

    for(const ModelInfo& info : it->second){
        if(info.version == version) return true;
    }
    return false;
}

bool ModelRegistry::checkVersionLocked(const std::string& modelName, const std::string& version) const{
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()) return false;

    for(const ModelInfo& info : it->second){
        if(info.version == version) return true;
    }
    return false;
}

bool ModelRegistry::addModel(const std::string& modelName, const std::string& version, const std::string& path, std::unique_ptr<ModelRuntime>&& model){
    std::lock_guard<std::mutex> lock(mtx);
    if(!model){ //model is nullptr
        return false;
    }
    if(checkVersionLocked(modelName, version)) return false;
    this->availableModels[modelName].emplace_back(version, path, std::move(model), true, std::chrono::steady_clock::now());
    return true;
}

std::vector<ModelSummary> ModelRegistry::getAvailableModels() const{
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<ModelSummary> snapshot;
    snapshot.reserve(availableModels.size());

    for(const auto& [modelName, models] : availableModels){
        ModelSummary summary;
        summary.name = modelName;
        summary.versions.reserve(models.size());

        for (const ModelInfo& model : models){
            summary.versions.push_back(model.version);
        }

        snapshot.push_back(std::move(summary));
    }

    return snapshot;
}

ModelRuntime* ModelRegistry::getRuntime(const std::string& modelName, const std::string& modelVersion) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        return nullptr;
    }

    for(const ModelInfo& mInfo : it->second){
        if(mInfo.version == modelVersion && mInfo.loaded){
            return mInfo.runtime.get();
        }
    }

    return nullptr;
}

bool ModelRegistry::loadModel(const std::string& modelName, const std::string& modelVersion, std::string& errorResponse){
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        errorResponse = "Model: " + modelName + " does not exist.";
        return false;
    }

    for(ModelInfo& info : it->second){
        if(info.version == modelVersion){
            //model version found
            if(info.loaded == true){
                errorResponse = "Model: " + modelName + " Version: " + modelVersion + " is already loaded.";
                return false;
            }
            info.loaded = true;
            return true;
        }
    }
    errorResponse = "Version: " + modelVersion + " does not exist for Model: " + modelName + ".";
    return false;
}

bool ModelRegistry::unloadModel(const std::string& modelName, const std::string& modelVersion, std::string& errorResponse){
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        errorResponse = "Model: " + modelName + " does not exist.";
        return false;
    }

    for(ModelInfo& info : it->second){
        if(info.version == modelVersion){
            //model version found
            if(info.loaded == false){
                errorResponse = "Model: " + modelName + " Version: " + modelVersion + " is already unloaded.";
                return false;
            }
            info.loaded = false;
            return true;
        }
    }
    errorResponse = "Version: " + modelVersion + " does not exist for Model: " + modelName + ".";
    return false;
}

const ModelInfo& ModelRegistry::getModelInfo(const std::string& modelName, const std::string& modelVersion) const{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = availableModels.find(modelName);
    if(it == availableModels.end()){
        throw std::runtime_error("Model " + modelName + " does not exist.");
    }

    for(const ModelInfo& mInfo : it->second){
        if(mInfo.version == modelVersion){
            return mInfo;
        }
    }
    throw std::runtime_error("Version " + modelVersion + " does not exist for model " + modelName);
}
