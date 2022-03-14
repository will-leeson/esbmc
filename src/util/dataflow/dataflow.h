/*******************************************************************\
 Module: Dataflow Interface
 Author: Rafael Sá Menezes
 Date: March 2022

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
 * behaves through the program flow (using a CFG). With the dataflow results at hand,
 * we can apply more algorithms. In special, this would enable optimizations and 
 * simplifications such as:
 * - Caching expressions
 * - Remove uneeded variables
 * - Remove computations that does not affect the property
 * 
 * The basic idea is:
 * 
 * - Propagate analysis information along the edges of a CFG
 * - Compute the analysis at each program point
 * - For each statement, define how it affect the program state
 * - For loops: iterate until fix-point is found
 *
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

// Just some helper definitions
namespace cfg_types
{
typedef goto_programt::instructionst::iterator cfg_index;
}

template <class D>
class dataflow : public goto_functions_algorithm
{
public:
  dataflow(DataflowDirection direction, goto_functionst &goto_functions)
    : goto_functions_algorithm(goto_functions, true), direction(direction)
  {
  }

  // Dump/Output
  virtual void dump() = 0;

  // Const Properties
  const DataflowDirection direction;

  // Gen/Kill (use/def) functions
  virtual std::set<D> gen(size_t) = 0;
  virtual std::set<D> kill(size_t) = 0;

  // Transfer functions
  virtual std::set<D> df_exit(size_t) = 0;
  virtual std::set<D> df_entry(size_t) = 0;

  // Meet operators
  virtual std::set<D> merge_operator(std::set<D>, std::set<D>) = 0;
  virtual std::set<D> branch_operator(std::set<D>, std::set<D>) = 0;

  // Goto instructions as CFG
  /**
   * @brief This will look into a specific instruction of a function CFG and
   *        will look at which instructions happens before the node.
   * 
   *        Note: To make implementation easier, this will always use forward
   *              (from ENTRY to EXIT). For backwards analysis the user can use
   *              the other function (exit_instructions) as the entry_instructions
   * 
   *             ENTRY
   *               |
   *               1
   *              / \
   *             2   3
   *              \ /
   *               4
   *               |
   *              EXIT
   * 
   *        Calling this function with index 1 would return {} (empty)
   *        Calling this function with index 4 would return {2,3}
   * 
   * @param F goto-function
   * @param index index of the instruction
   * @return std::set<int> set of index that come before the current instruction
   */
  std::set<cfg_types::cfg_index> entry_instructions(
    std::pair<const dstring, goto_functiont> &F,
    cfg_types::cfg_index &index)

  {
    std::set<cfg_types::cfg_index> result;

    // First function should be empty
    if(index == F.second.body.instructions.begin())
      return result;

    // Add Previous Instruction
    auto bck = index;
    bck--;
    result.insert(bck);
    
    if(index->is_goto() || index->is_backwards_goto())
    {
      index->dump();
    }
    return result;
  }
  /**
   * @brief This will look into a specific instruction of a function CFG and
   *        will look at which instructions happens before the node
   * 
   *        Note: To make implementation easier, this will always use forward
   *              (from ENTRY to EXIT). For backwards analysis the user can use
   *              the other function (entry_instructions) as the exit_instructions
   * 
   *             ENTRY
   *               |
   *               1
   *              / \
   *             2   3
   *              \ /
   *               4
   *               |
   *              EXIT
   * 
   *        Calling this function with index 1 would return {2,3}
   *        Calling this function with index 4 would return {} (empty)
   * 
   * @param F goto-function
   * @param index index of the instruction
   * @return std::set<int> set of index that come before the current instruction (in the flow)
   */
  std::set<cfg_types::cfg_index> exit_instructions(
    std::pair<const dstring, goto_functiont> &F,
    cfg_types::cfg_index &index)

  {
    std::set<cfg_types::cfg_index> result;
    return result;
  }
};
