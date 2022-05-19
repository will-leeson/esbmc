cmake_minimum_required(VERSION 3.10)
project(torchscriptTest)


set(CMAKE_PREFIX_PATH "/home/wel2vw/Downloads/libtorch")
find_package(Torch REQUIRED)
find_package(TorchSparse REQUIRED)
find_package(TorchScatter REQUIRED)

add_library(gat gat.cpp)

target_link_libraries(gat "${TORCH_LIBRARIES}" TorchSparse::TorchSparse TorchScatter::TorchScatter)
set_property(TARGET gat PROPERTY CXX_STANDARD 14)


add_library(prediction INTERFACE)
target_link_libraries(prediction INTERFACE gat)