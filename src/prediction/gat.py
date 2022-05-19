import torch
import torch.nn as nn
import torch.nn.functional as f
from torch_geometric.nn import GATv2Conv, JumpingKnowledge, GlobalAttention

from sys import argv

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
            out = gat(x, edge_index, edge_attr=edge_attr)
            x = f.leaky_relu(out)
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

def main(argv):
    for item in argv:
        print(item)

main(argv)