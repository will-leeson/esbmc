#include "gat.h"

gat::gat(){
    module = torch::jit::load("/home/wel2vw/Research/sandbox/GNN.pt");
}

std::string gat::predict(std::vector<int[64]> nodes, std::vector<std::pair <int,int>> edges, std::vector<int> edge_attr){
    std::vector<torch::jit::IValue> inputs;
    torch::jit::IValue x = torch::ones({25,300});
    return "";
}