add_library(prediction gat.cpp)

target_link_libraries(prediction PUBLIC "${TORCH_LIBRARIES}" TorchSparse::TorchSparse TorchScatter::TorchScatter ${Boost_LIBRARIES})
set_property(TARGET prediction PROPERTY CXX_STANDARD 14)