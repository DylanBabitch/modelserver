#pragma once

#include <model/ModelRuntime.hpp>
#include <crow.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

struct ModelInfo{
    std::string modelVersion; //Name of model version 
    std::string path; //File path for model
    std::unique_ptr<ModelRuntime> runtime; //Runtime for model
};

class ModelRegistry{
private:
    //TODO maybe change availableModel impl if there's a lot of versions per model
    mutable std::mutex mtx;
    std::unordered_map<std::string, std::vector<ModelInfo>> availableModels; //Map from Model Name -> Vector of ModelInfo (one for each version within a single model)
    bool checkVersionLocked(const std::string& modelName, const std::string& version) const; //MUTEX SAFE Checks if model modelName with version versionName exists
public:
    bool addModel(const std::string& modelName, const std::string& version, const std::string& path, std::unique_ptr<ModelRuntime>&& runtime); //Returns true if model gets added, false if it already exists
    bool checkModel(const std::string& modelName) const; //Checks if model modelName exists
    bool checkVersion(const std::string& modelName, const std::string& version) const; //Checks if model modelName with version versionName exists
    const std::vector<crow::json::wvalue> getAvailableModels() const; //returns all 
    ModelRuntime* getRuntime(const std::string& modelName, const std::string& modelVersion) const; //TODO maybe use const ModelRuntime* depending on final ModelRuntime impl
};