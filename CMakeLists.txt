cmake_minimum_required(VERSION 3.20)

project(modelserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CROW_BUILD_EXAMPLES OFF)
set(CROW_BUILD_TESTS OFF)

add_subdirectory(external/crow)

add_subdirectory(external/gtest)

set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/external/onnx")

add_library(onnx SHARED IMPORTED)

set_target_properties(onnx PROPERTIES
    IMPORTED_LOCATION
        "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so"

    INTERFACE_INCLUDE_DIRECTORIES
        "${ONNXRUNTIME_ROOT}/include"
)

add_executable(modelserver
    src/main.cpp

    src/metrics/MetricsRegistry.cpp
    src/metrics/Timer.cpp

    src/model/DummyRuntime.cpp
    src/model/ModelRegistry.cpp
    src/model/OnnxRuntime.cpp

    src/server/HttpServer.cpp

    src/scheduler/WorkerPool.cpp
    src/scheduler/RequestQueue.cpp
    src/scheduler/BatchManager.cpp
)

target_include_directories(modelserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(modelserver PRIVATE
    Crow::Crow
    onnx
)

target_compile_options(modelserver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

set_target_properties(modelserver PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_ROOT}/lib"
)

enable_testing()

add_executable(modelserver_tests
    tests/metrics_tests.cpp
    tests/model_registry_tests.cpp
    tests/request_queue_tests.cpp
    tests/batch_manager_tests.cpp
    tests/worker_pool_tests.cpp
    tests/http_server_tests.cpp

    src/metrics/MetricsRegistry.cpp
    src/metrics/Timer.cpp
    src/model/DummyRuntime.cpp
    src/model/ModelRegistry.cpp
    src/model/OnnxRuntime.cpp
    src/server/HttpServer.cpp
    src/scheduler/RequestQueue.cpp
    src/scheduler/WorkerPool.cpp
    src/scheduler/BatchManager.cpp
)

target_include_directories(modelserver_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(modelserver_tests PRIVATE
    GTest::gtest_main
    Crow::Crow
    onnx
)

set_target_properties(modelserver_tests PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_ROOT}/lib"
)

include(GoogleTest)
gtest_discover_tests(modelserver_tests)
