#include <sibyl_pred.h>
#include <iostream>

int predict(torch::jit::Module module, std::vector<int> nodes, std::vector<std::pair <int,int>> edges, std::vector<int> edge_attr){
    // std::vector<torch::jit::IValue> inputs;

    // auto intOptions = torch::TensorOptions().dtype(torch::kInt32);
    // auto longOptions = torch::TensorOptions().dtype(torch::kInt64);

    // torch::Tensor nodesTensor = torch::from_blob(nodes.data(), {nodes.size()}, intOptions);

    // std::cout << nodesTensor.slice(/*dim=*/1, /*start=*/0, /*end=*/5) << '\n';

    return 0;
}