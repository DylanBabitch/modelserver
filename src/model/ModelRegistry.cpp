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

 std::vector<ModelSummary> ModelRegistry::getAvailableModels() const
  {
      std::lock_guard<std::mutex> lock(mtx);

      std::vector<ModelSummary> snapshot;
      snapshot.reserve(availableModels.size());

      for (const auto& [modelName, models] : availableModels) {
          ModelSummary summary;
          summary.name = modelName;
          summary.versions.reserve(models.size());

          for (const ModelInfo& model : models) {
              summary.versions.push_back(model.modelVersion);
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
        if(mInfo.modelVersion == modelVersion){
            return mInfo.runtime.get();
        }
    }

    return nullptr;
    
    
}