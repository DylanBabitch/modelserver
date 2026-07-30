cmake_minimum_required(VERSION 3.20)

project(modelserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CROW_BUILD_EXAMPLES OFF)
set(CROW_BUILD_TESTS OFF)

add_subdirectory(external/crow)

add_subdirectory(external/gtest)

add_executable(modelserver
    src/main.cpp

    src/metrics/MetricsRegistry.cpp
    src/metrics/Timer.cpp

    src/model/DummyRuntime.cpp
    src/model/ModelRegistry.cpp

    src/server/HttpServer.cpp

    src/scheduler/WorkerPool.cpp
    src/scheduler/RequestQueue.cpp
)

target_include_directories(modelserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(modelserver PRIVATE
    Crow::Crow
)

target_compile_options(modelserver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

add_executable(modelserver_tests
    tests/metrics_tests.cpp
    tests/model_registry_tests.cpp

    src/metrics/MetricsRegistry.cpp
    src/model/ModelRegistry.cpp
)

target_include_directories(modelserver_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(modelserver_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(modelserver_tests)