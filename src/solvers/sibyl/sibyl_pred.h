#ifndef _ESBMC_SOLVERS_SIYBL_PRED_H_
#define _ESBMC_SOLVERS_SIYBL_PRED_H_

#include <torch/script.h>
#include <torchsparse/sparse.h>
#include <torchscatter/scatter.h>
/** @file sibyl.h
 * Sibyl predictor and utilities
 * This file deals with creating a sibyl predictor and
 * making actual predictions.
 */

int predict(torch::jit::Module module, std::vector<int> nodes, std::vector<std::pair <int,int>> edges, std::vector<int> edge_attr);


#endif /*_ESBMC_SOLVERS_SIYBL_PRED_H_ */