/*******************************************************************\
 Module: Available expressions Algorithm
 Author: Rafael Sá Menezes
 Date: March 2022

 Description: For each program point, compute which expressions must
              have already been computed (and that does not need to again)
\*******************************************************************/

#pragma once

#include <util/dataflow/dataflow.h>

class available_expressions_dataflow : public dataflow<expr2tc>
{
public:
  available_expressions_dataflow(goto_functionst &goto_functions)
    : dataflow<expr2tc>(DataflowDirection::FORWARD, goto_functions)
  {
  }

  virtual void dump() override
  {
  }

  // Gen/Kill (use/def) functions
  virtual std::set<expr2tc> gen(size_t) override
  {
    std::set<expr2tc> result;
    return result;
  }
  virtual std::set<expr2tc> kill(size_t) override
  {
    std::set<expr2tc> result;
    return result;
  }

  // Transfer functions
  virtual std::set<expr2tc> df_exit(size_t) override
  {
    std::set<expr2tc> result;
    return result;
  }
  virtual std::set<expr2tc> df_entry(size_t) override
  {
    std::set<expr2tc> result;
    return result;
  }
  // Meet operators
  virtual std::set<expr2tc>
    merge_operator(std::set<expr2tc>, std::set<expr2tc>) override
  {
    std::set<expr2tc> result;
    return result;
  }
  virtual std::set<expr2tc>
    branch_operator(std::set<expr2tc>, std::set<expr2tc>) override
  {
    std::set<expr2tc> result;
    return result;
  }
};