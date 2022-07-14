/*******************************************************************\

Module: Command Line Parsing

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_ESBMC_PARSEOPTIONS_H
#define CPROVER_ESBMC_PARSEOPTIONS_H

#include <esbmc/bmc.h>
#include <goto-programs/goto_convert_functions.h>
#include <langapi/language_ui.h>
#include <util/cmdline.h>
#include <util/options.h>
#include <util/parseoptions.h>
#include <prediction/gat.h>

extern const struct group_opt_templ all_cmd_options[];

class esbmc_parseoptionst : public parseoptions_baset, public language_uit
{
public:
  int doit() override;
  void help() override;

  esbmc_parseoptionst(int argc, const char **argv, messaget &msg)
    : parseoptions_baset(all_cmd_options, argc, argv, msg),
      language_uit(cmdline, msg)
  {
  }

  ~esbmc_parseoptionst()
  {
    close_file(out);
    if(out != err)
      close_file(err);
  }

protected:
  virtual void get_command_line_options(optionst &options);
  virtual int do_bmc(bmct &bmc, gat &model);

  virtual bool
  get_goto_program(optionst &options, goto_functionst &goto_functions);

  virtual bool
  process_goto_program(optionst &options, goto_functionst &goto_functions);

  int doit_k_induction(gat model);
  int doit_k_induction_parallel(gat model);

  int doit_falsification(gat model);
  int doit_incremental(gat model);
  int doit_termination(gat model);

  int do_base_case(
    optionst &opts,
    goto_functionst &goto_functions,
    const BigInt &k_step,
    gat &model);

  int do_forward_condition(
    optionst &opts,
    goto_functionst &goto_functions,
    const BigInt &k_step,
    gat &model);

  int do_inductive_step(
    optionst &opts,
    goto_functionst &goto_functions,
    const BigInt &k_step,
    gat &model);

  bool read_goto_binary(goto_functionst &goto_functions);

  bool set_claims(goto_functionst &goto_functions);

  void set_verbosity_msg(messaget &message);

  uint64_t read_time_spec(const char *str);
  uint64_t read_mem_spec(const char *str);

  void preprocessing();

  void print_ileave_points(namespacet &ns, goto_functionst &goto_functions);

  FILE *out = stdout;
  FILE *err = stderr;

private:
  void close_file(FILE *f)
  {
    if(f != stdout && f != stderr)
    {
      fclose(f);
    }
  }

public:
  goto_functionst goto_functions;
};

#endif
