#include <model/OnnxRuntime.hpp>
#include <request/PredictionRequest.hpp>
#include <request/PredictionResponse.hpp>
#include <onnxruntime_cxx_api.h>

#include <vector>
#include <cstdint>
#include <exception>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <metrics/Timer.hpp>

namespace {

std::size_t getElementCount(const std::vector<std::int64_t>& shape) {
    std::size_t elementCount = 1;
    for (const std::int64_t dimension : shape) {
        if (dimension < 0) {
            throw std::runtime_error("Input shape dimensions must be non-negative");
        }
        if (dimension != 0 && elementCount > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(dimension)) {
            throw std::runtime_error("Input shape is too large");
        }
        elementCount *= static_cast<std::size_t>(dimension);
    }
    return elementCount;
}

} // namespace

PredictionResponse OnnxRuntime::predict(const PredictionRequest& request){
    Timer t;

    const std::vector<float>& inputData = request.input.data;
    const std::vector<std::int64_t>& inputShape = request.input.shape;
    std::vector<float> inputValues = inputData;

    if (request.input.name != inputName) {
        throw std::runtime_error("Input name does not match the ONNX model");
    }
    if (inputShape.size() != this->inputShape.size()) {
        throw std::runtime_error("Input rank does not match the ONNX model");
    }
    for (std::size_t i = 0; i < inputShape.size(); ++i) {
        if (this->inputShape[i] >= 0 && inputShape[i] != this->inputShape[i]) {
            throw std::runtime_error("Input shape does not match the ONNX model");
        }
    }
    if (inputData.size() != getElementCount(inputShape)) {
        throw std::runtime_error("Input data size does not match the input shape");
    }
    
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo, inputValues.data(), inputValues.size(), inputShape.data(), inputShape.size());
    const char* inputNames[] = {inputName.c_str()};
    const char* outputNames[] = {outputName.c_str()};

    std::vector<Ort::Value> outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
    if (outputTensors.size() != 1 || !outputTensors[0].IsTensor()) {
        throw std::runtime_error("ONNX model did not return one tensor output");
    }

    Ort::Value& outputTensor = outputTensors[0];
    Ort::TensorTypeAndShapeInfo outputInfo = outputTensor.GetTensorTypeAndShapeInfo();
    if (outputInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        throw std::runtime_error("ONNX model output must use float tensors");
    }
    std::vector<std::int64_t> outputShape = outputInfo.GetShape();
    const std::size_t outputElementCount = outputInfo.GetElementCount();
    std::vector<float> outputValues(outputElementCount);
    if (outputElementCount > 0) {
        float* outputData = outputTensor.GetTensorMutableData<float>();
        std::copy(outputData, outputData + outputElementCount, outputValues.begin());
    }

    double latency = t.end();

    PredictionResponse p{request.model, request.version, {outputName, outputShape, outputValues}, latency};
    return p;
}

OnnxRuntime::OnnxRuntime(const std::string& modelPath):
             env(ORT_LOGGING_LEVEL_WARNING, "log identifier"), session_options(), session(env, modelPath.c_str(), session_options){
    if (session.GetInputCount() != 1 || session.GetOutputCount() != 1) {
        throw std::runtime_error("Only ONNX models with one input and one output are supported");
    }

    std::vector<std::string> inputNames = session.GetInputNames();
    std::vector<std::string> outputNames = session.GetOutputNames();
    inputName = inputNames[0];
    outputName = outputNames[0];

    Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(0);
    Ort::TypeInfo outputTypeInfo = session.GetOutputTypeInfo(0);
    if (inputTypeInfo.GetONNXType() != ONNX_TYPE_TENSOR || outputTypeInfo.GetONNXType() != ONNX_TYPE_TENSOR) {
        throw std::runtime_error("ONNX model inputs and outputs must be tensors");
    }

    Ort::ConstTensorTypeAndShapeInfo inputInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    Ort::ConstTensorTypeAndShapeInfo outputInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
    if (inputInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT || outputInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        throw std::runtime_error("ONNX model inputs and outputs must use float tensors");
    }

    inputShape = inputInfo.GetShape();
}
