/*******************************************************************\
 Module: Dataflow Interface
 Author: Rafael Sá Menezes
 Date: May 2021

 Description: The dataflow interface is to be used as
              a base for any type of dataflow analysis inside
              ESBMC
\*******************************************************************/

#pragma once

#include <util/algorithms.h>
#include <vector>
#include <set>
#include <irep2/irep2.h>

/**
 * @brief This class will be the base for applying dataflow analysis in goto-programs
 * 
 * The dataflow is a framework where we can reason about any kind of data and how it
 * behaves through the program flow (using a CFG). In special, this can enable 
 * optimizations and simplifications such as:
 * - Caching expressions
 * - Remove uneeded variables
 * - Remove computations that does not affect the property
 *  * 
 * To define a dataflow analysis, one needs to define the following properties:
 * 
 * 1. Domain: This will be the target data for the analysis. Are we checking for
 *            expressions, statements affected, asserts, ...?
 * 2. Direction: Is it a forward analysis or backwards? Which flow are we interested?
 * 3. Transfer Function: How the current state affects the data
 * 4. Meet operator: How to merge/branch the current data (if/else)
 * 5. Boundary Condition: What are the conditions before/after the flow?
 * 6. Initial Values: What are the values of the intermediate nodes?
 * 
 * @tparam D the data domain
 */

enum class DataflowDirection
{
  FORWARD,
  BACKWARD
};

template <class D>
class dataflow : goto_functions_algorithm
{
public:
  dataflow(DataflowDirection direction, goto_functionst &goto_functions)
    : goto_functions_algorithm(goto_functions, true), direction(direction)
  {
  }
  const DataflowDirection direction;
  // Gen/Kill functions
  virtual std::set<D> gen(size_t) = 0;
  virtual std::set<D> kill(size_t) = 0;

  // Transfer functions
  virtual std::set<D> df_exit(size_t) = 0;
  virtual std::set<D> df_entry(size_t) = 0;

  // Meet operators
  virtual std::set<D> merge_operator(std::set<D>, std::set<D>) = 0;
  virtual std::set<D> branch_operator(std::set<D>, std::set<D>) = 0;
};

// Live variable (for assertions)
class liveness_dataflow : dataflow<symbol2tc>
{
public:
  liveness_dataflow(goto_functionst &goto_functions)
    : dataflow(DataflowDirection::BACKWARD, goto_functions)
  {
  }
};