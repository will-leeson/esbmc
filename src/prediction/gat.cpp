#include <gat.h>
#include <iostream>

gat::gat(){
    loaded=false;
}

gat::gat(std::string path){
    try{
        model = torch::jit::load(path);
        loaded = true;
    }
    catch(const c10::Error &e){
        loaded = false;
    }
}

int gat::predict(std::vector<unsigned int> nodes, 
                 std::vector<unsigned int> inEdges,
                 std::vector<unsigned int> outEdges,  
                 std::vector<unsigned int> edge_attr){

    //Need to declare it as int type to avoid conversion issues
    auto opts = torch::TensorOptions().dtype(torch::kInt32);
    //Need to convert it to a Float for the model
    auto nodeTensor = torch::from_blob(nodes.data(), {(unsigned int)nodes.size()/67,67}, opts).to(torch::kFloat32);

    auto inEdgeTensor = torch::from_blob(inEdges.data(), (unsigned int)inEdges.size(), opts).to(torch::kI64);
    auto outEdgeTensor = torch::from_blob(outEdges.data(), (unsigned int)outEdges.size(), opts).to(torch::kI64);
    auto edgeTensor = torch::stack({inEdgeTensor, outEdgeTensor});

    auto edge_attrTensor = torch::from_blob(edge_attr.data(), edge_attr.size(), opts).to(torch::kFloat32);

    auto problemType = torch::zeros(1, opts).to(torch::kFloat32);

    auto batch = torch::zeros(nodeTensor.sizes()[0], opts).to(torch::kI64);

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(nodeTensor);
    inputs.push_back(edgeTensor);
    inputs.push_back(edge_attrTensor);
    inputs.push_back(problemType);
    inputs.push_back(batch);

    auto out = model.forward(inputs).toTensor();
    int choice = out.argmin().item<int>();

    return choice;
}

void gat::load_model(std::string path){
    try{
        model = torch::jit::load(path);
        loaded = true;
    }
    catch(const c10::Error &e){
        loaded = false;
    }
}

bool gat::is_loaded(){
    return loaded;
}

gat::~gat() {};