#include <gat.h>
#include <iostream>

gat::gat(){
    loaded=false;
}

gat::gat(std::string path){
    std::cout<<"Creating model with "<<path<<std::endl;
    try{
        model = torch::jit::load(path);
        loaded = true;
    }
    catch(const c10::Error &e){
        std::cout<<e.what()<<std::endl;
        loaded = false;
    }
}

std::vector<std::string> gat::predict(
                std::vector<unsigned int> nodes, 
                std::vector<unsigned int> inEdges,
                std::vector<unsigned int> outEdges,  
                std::vector<unsigned int> edge_attr){

    //Need to declare it as int type to avoid conversion issues
    c10::InferenceMode guard(true);
    model.eval();

    std::vector<std::string> solvers = {"bitwuzla", "boolector", "cvc", "mathsat", "yices", "z3"};
    if(terminate){
        std::cout<< "Terminated before any tensor creation"<<std::endl;
        return solvers;
    }

    auto opts = torch::TensorOptions().dtype(torch::kInt32);
    //Need to convert it to a Float for the model
    auto nodeTensor = torch::zeros({(long)nodes.size(), 67},opts).to(torch::kFloat32);

    for(int i=0; i<nodes.size(); i++){
        int j = nodes[i];
        nodeTensor.index_put_({i, j},1);
        if(terminate){
            std::cout<< "Terminated during node tensor creation"<<std::endl;
            return solvers;
        }
    }

    auto outEdgeTensor = torch::from_blob(outEdges.data(), (unsigned int)outEdges.size(), opts).to(torch::kI64);
    auto inEdgeTensor = torch::from_blob(inEdges.data(), (unsigned int)inEdges.size(), opts).to(torch::kI64);
    auto edgeTensor = torch::stack({outEdgeTensor, inEdgeTensor});
    

    auto edge_attrTensor = torch::from_blob(edge_attr.data(), edge_attr.size(), opts).to(torch::kFloat32);
    if(terminate){
        std::cout<< "Terminated after tensor creation"<<std::endl;
        return solvers;
    }

    auto problemType = torch::zeros(1, opts).to(torch::kFloat32);

    auto batch = torch::zeros(nodeTensor.sizes()[0], opts).to(torch::kI64);

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(nodeTensor);
    inputs.push_back(edgeTensor);
    inputs.push_back(edge_attrTensor);
    inputs.push_back(problemType);
    inputs.push_back(batch);
    if(terminate){
        std::cout<< "Terminated after tensor stacking"<<std::endl;
        return solvers;
    }

    auto out = model.forward(inputs).toTensor();
    if(terminate){
        std::cout<< "Terminated after forward call"<<std::endl;
        return solvers;
    }
    out = out.argsort();
    out = out.contiguous().to(torch::kInt32);
    std::vector<int> v(out.data_ptr<int>(), out.data_ptr<int>() + out.numel());

    std::vector<std::string> outVector = {solvers[v[0]], solvers[v[1]],solvers[v[2]],solvers[v[3]],solvers[v[4]],solvers[v[5]]};

    std::cout<< "Made it through full call"<<std::endl;
    return outVector;
}

void gat::load_model(std::string path){
    std::cout<<"Loading "<<path<<std::endl;
    try{
        model = torch::jit::load(path);
        loaded = true;
    }
    catch(const c10::Error &e){
        std::cout<<e.what()<<std::endl;
        loaded = false;
    }
}

bool gat::is_loaded(){
    return loaded;
}

gat::~gat() {};

void gat::set_terminate(bool val){
    terminate = val;
}