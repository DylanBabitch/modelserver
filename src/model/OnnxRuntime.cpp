#include <model/OnnxRuntime.hpp>
#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>
#include <onnxruntime_cxx_api.h>

#include <vector>
#include <cstdint>
#include <exception>

PredictionResponse OnnxRuntime::predict(PredictionRequest& request){
    //TODO add support for multiple 
    std::vector<float>& inputData = request.input.data;
    std::vector<std::int64_t>& inputShape = request.input.shape;

    //enfore shape == 1x1x28x28 for minst 
    if(inputShape.size() != 4){
        throw std::runtime_error("Input Shape must be 1x1x28x28 for MINST");
    }else if(inputShape[0] != 1.0 || inputShape[1] != 1.0 || inputShape[2] != 28.0 || inputShape[3] != 28.0){
        throw std::runtime_error("Input Shape must be 1x1x28x28 for MINST");
    }

    //get from model registry in the future
    std::vector<float> outputValues(1 * 10, 0.0f);
    std::vector<int64_t> outputShape = {1, 10};
    
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo, inputData.data(), inputData.size(), inputShape.data(), inputShape.size());
    //get from model registry in future
    Ort::Value outputTensor = Ort::Value::CreateTensor<float>(memInfo, outputValues.data(), outputValues.size(), outputShape.data(), outputShape.size());
    const char* inputNames[] = {"Input3"};
    const char* outputNames[] = {"Plus214_Output_0"};

    session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, &outputTensor, 1);


}
