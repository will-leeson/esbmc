/*******************************************************************\

Module: Bounded Model Checking for ANSI-C + HDL

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_CBMC_BMC_H
#define CPROVER_CBMC_BMC_H

#include <goto-symex/reachability_tree.h>
#include <goto-symex/symex_target_equation.h>
#include <langapi/language_ui.h>
#include <list>
#include <map>
#include <solvers/smt/smt_conv.h>
#include <solvers/smtlib/smtlib_conv.h>
#include <solvers/sibyl/sibyl_conv.h>
#include <solvers/solve.h>
#include <util/options.h>
#include <util/time_stopping.h>
#include <iostream>
#include <prediction/gat.h>


class bmct
{
public:
  bmct(
    goto_functionst &funcs,
    optionst &opts,
    contextt &_context,
    const messaget &_message_handler,
    std::string &_last_winner);

  bmct(const bmct& rhs);

  optionst &options;

  BigInt interleaving_number;
  BigInt interleaving_failed;
  bool use_sibyl;

  virtual smt_convt::resultt start_bmc(gat &model);
  virtual smt_convt::resultt run(std::shared_ptr<symex_target_equationt> &eq, gat &model);
  virtual ~bmct() = default;
  virtual smt_convt::resultt run_decision_procedure(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);

  virtual smt_convt::resultt run_top_k_decision_procedure(
    std::shared_ptr<smt_convt> &smt_conv1,
    std::shared_ptr<smt_convt> &smt_conv2,
    std::shared_ptr<symex_target_equationt> &eq,
    gat &model
  );
  
  virtual smt_convt::resultt run_static_parallel_decision_procedure(
    std::shared_ptr<smt_convt> &smt_conv1,
    std::shared_ptr<smt_convt> &smt_conv2,
    std::shared_ptr<symex_target_equationt> &eq,
    gat &model
  );

  virtual smt_convt::resultt run_prediction_with_choice_decision_procedure(
    std::shared_ptr<smt_convt> &smt_conv1,
    std::shared_ptr<smt_convt> &smt_conv2,
    std::shared_ptr<symex_target_equationt> &eq,
    gat &model
  );

  virtual void do_cbmc(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);
  
  std::vector<std::string> run_sibyl(std::shared_ptr<symex_target_equationt> &eq, gat &model);
  fine_timet get_solve_time();

  std::vector<std::string> choose_parallel_solvers();

protected:
  const contextt &context;
  namespacet ns;
  const messaget &msg;
  std::shared_ptr<smt_convt> prediction_solver;
  std::shared_ptr<smt_convt> runtime_solver;
  std::shared_ptr<smt_convt> solver1;
  std::shared_ptr<smt_convt> solver2;;
  std::shared_ptr<reachability_treet> symex;

  virtual void show_program(std::shared_ptr<symex_target_equationt> &eq);
  virtual void report_success();
  virtual void report_failure();

  virtual void error_trace(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);

  virtual void successful_trace();

  virtual void show_vcc(std::shared_ptr<symex_target_equationt> &eq);

  virtual void
  show_vcc(std::ostream &out, std::shared_ptr<symex_target_equationt> &eq);

  virtual void report_trace(
    smt_convt::resultt &res,
    std::shared_ptr<symex_target_equationt> &eq);

  virtual void report_result(smt_convt::resultt &res);

  virtual void bidirectional_search(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);

  smt_convt::resultt run_thread(std::shared_ptr<symex_target_equationt> &eq, gat &model);


  std::string &last_winner;
  fine_timet solve_time = 0;
};

#endif
