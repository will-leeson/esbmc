import torch
import torch.nn as nn
import torch.nn.functional as f
from torch_geometric.nn import GATv2Conv, JumpingKnowledge, GlobalAttention, global_add_pool
from torch_geometric.data import Data, Batch
import numpy as np

class GAT(torch.nn.Module):
    def __init__(self, passes, inputLayerSize, outputLayerSize, numAttentionLayers):
        super(GAT, self).__init__()
        self.passes = passes
            
        self.gats = nn.ModuleList([GATv2Conv(inputLayerSize,inputLayerSize, heads=numAttentionLayers, concat=False, dropout=0, edge_dim=1) for i in range(passes)])
        self.jump = JumpingKnowledge('cat', channels=inputLayerSize, num_layers=self.passes)
        fcInputLayerSize = ((self.passes+1)*inputLayerSize)+1

        self.pool = GlobalAttention(gate_nn=nn.Sequential(torch.nn.Linear(fcInputLayerSize-1, 1), nn.LeakyReLU()))

        self.fc1 = nn.Linear(fcInputLayerSize, fcInputLayerSize//2)
        self.fc2 = nn.Linear(fcInputLayerSize//2,fcInputLayerSize//2)
        self.fcLast = nn.Linear(fcInputLayerSize//2, outputLayerSize)
    
    def forward(self, x, edge_index, edge_attr, problemType, batch):
        xs = [x]
        for gat in self.gats:
            x = f.leaky_relu(gat(x, edge_index, edge_attr=edge_attr))
            xs += [x]

        x = self.jump(xs)
        x = self.pool(x, batch)

        x = torch.cat((x.reshape(1,x.size(0)*x.size(1)), problemType.unsqueeze(1)), dim=1)

        x = self.fc1(x)
        x = f.leaky_relu(x)
        x = self.fc2(x)
        x = f.leaky_relu(x)
        x = self.fcLast(x)

        return x
    
    def load_model(self, model_file):
        self.load_state_dict(torch.load(model_file, map_location='cpu')) 

def predict(model, nodes, outEdges, inEdges, edge_attrs):
    nodes = torch.tensor([[0]*x + [1] + [0]*(66-x) for x in nodes])
    edges = torch.tensor([outEdges,inEdges])
    edge_attrs = torch.tensor(edge_attrs)

    # graph = np.savez("graph.npz", nodes=nodes.numpy(), edges=edges.numpy(), edge_attrs=edge_attrs.numpy())

    graph = Data(x=nodes.float(), edge_index=edges, edge_attr=edge_attrs.float(), problemType=torch.FloatTensor([0]))
    graph = Batch.from_data_list([graph])
    model.eval()

    with torch.no_grad():
        print("about to predict")
        scores = model(graph.x, graph.edge_index, graph.edge_attr, graph.problemType, graph.batch)
        print("predicted")

    solvers = ["Bitwuzla", 'mathsat-5.6.6', 'Yices 2.6.2 for SMTCOMP 2021', 'z3-4.8.11', 'cvc5', 'STP 2021.0']
    solvers.sort()

    solverToSolvers = {"z3-4.8.11":"z3", "STP 2021.0":"boolector",  'mathsat-5.6.6':"mathsat", 'cvc5':"cvc", 'Yices 2.6.2 for SMTCOMP 2021' : "yices", "Bitwuzla": "bitwuzla"}
    return solverToSolvers[solvers[scores[0].argmin()]]

# def main(argv):
#     nodes = argv[1].split(",")[:-1]
#     edges = argv[2].split(",")[:-1]
#     edge_attrs = argv[3].split(",")[:-1]

#     assert len(edges) == 2*len(edge_attrs)

#     nodes = torch.tensor([[0]*int(x) + [1] + [0]*(66-int(x)) for x in nodes])
#     edges = torch.tensor([[int(edges[x]) for x in range(0,len(edges),2)],[int(edges[x]) for x in range(1,len(edges),2)]])
#     edge_attrs = torch.tensor([int(x) for x in edge_attrs])

#     graph = Data(x=nodes.float(), edge_index=edges, edge_attr=edge_attrs.float(), problemType=torch.FloatTensor([0]))
#     graph = Batch.from_data_list([graph])

#     model = GAT(2, 67, 6, 5).eval()

#     model.load_state_dict(torch.load("/home/wel2vw/Research/esbmc_project/esbmc/src/prediction/gnn.pt", map_location='cpu'))

#     with torch.no_grad():
#         scores = model(graph.x, graph.edge_index, graph.edge_attr, graph.problemType, graph.batch)

#     solvers = ["Bitwuzla", 'mathsat-5.6.6', 'Yices 2.6.2 for SMTCOMP 2021', 'z3-4.8.11', 'cvc5', 'STP 2021.0']
#     solvers.sort()

#     solverToSolvers = {"z3-4.8.11":"z3", "STP 2021.0":"boolector",  'mathsat-5.6.6':"mathsat", 'cvc5':"cvc", 'Yices 2.6.2 for SMTCOMP 2021' : "yices", "Bitwuzla": "bitwuzla"}

#     choice = solvers[scores[0].argmin()]

#     print(solverToSolvers[choice],end="")

# main(argv)
