#ifndef PREDICTION_GAT_H
#define PREDICTION_GAT_H

#include <torch/script.h>
#include <torchscatter/scatter.h>
#include <torchsparse/sparse.h>

class gat
{
private:
    torch::jit::script::Module model;
    bool loaded;
public:
    gat();
    gat(std::string path);
    ~gat();

    std::vector<std::string> predict(
                std::vector<unsigned int> nodes, 
                std::vector<unsigned int> inEdges,
                std::vector<unsigned int> outEdges, std::vector<unsigned int> edge_attr);
    void load_model(std::string path);
    bool is_loaded();
    void set_terminate(bool val);

    bool terminate=false;
};

#endif