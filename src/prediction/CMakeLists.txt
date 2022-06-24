add_library(prediction gat.cpp)

target_include_directories(prediction
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(prediction
    TorchScatter::TorchScatter TorchSparse::TorchSparse
)

set_property(TARGET prediction PROPERTY CXX_STANDARD 14)