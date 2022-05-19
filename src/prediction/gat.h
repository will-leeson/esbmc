#ifndef PREDICTION_GAT_H
#define PREDICTION_GAT_H

#include <vector>
#include <string>

#include <torch/script.h> // One-stop header.
// #include <torchsparse/sparse.h>
// #include <torchscatter/scatter.h>

class gat
{
public:
    gat();

    std::string predict(std::vector<int[64]> nodes, std::vector<std::pair <int,int>> edges, std::vector<int> edge_attr);

    torch::jit::script::Module module;

};

#endif