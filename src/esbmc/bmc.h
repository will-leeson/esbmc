/*******************************************************************\

Module: Bounded Model Checking for ANSI-C + HDL

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_CBMC_BMC_H
#define CPROVER_CBMC_BMC_H

#define PY_SSIZE_T_CLEAN

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
#include <iostream>
#include <Python.h>

class CPythonEnv
{
private:
  PyObject *gat, *pModule;
  bool loadingError = false;
public:
  CPythonEnv()
  {
    //Start Python Enviornment
    Py_Initialize();
    PyObject *gatArgs, *passes, *inputLayerSize, *outputLayerSize, *numAttentionLayers;

    //Load GAT module
    PyObject *pName = PyUnicode_DecodeFSDefault("gat");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    //Get GAT class
    PyObject *dict = PyModule_GetDict(pModule);
    if (dict == nullptr) {
      PyErr_Print();
      loadingError = true;
    }

    PyObject *gatClass = PyDict_GetItemString(dict, "GAT");

    //Create GAT object
    gatArgs = PyTuple_New(4);
    passes = PyLong_FromLong(0);
    inputLayerSize = PyLong_FromLong(67);
    outputLayerSize = PyLong_FromLong(6);
    numAttentionLayers = PyLong_FromLong(5);
    PyTuple_SetItem(gatArgs, 0, passes);
    PyTuple_SetItem(gatArgs, 1, inputLayerSize);
    PyTuple_SetItem(gatArgs, 2, outputLayerSize);
    PyTuple_SetItem(gatArgs, 3, numAttentionLayers);

    if (PyCallable_Check(gatClass)) {
      gat = PyObject_CallObject(gatClass, gatArgs);
      Py_DECREF(gatClass);
      Py_DECREF(gatArgs);
      Py_DECREF(dict);
    } else {
      loadingError=true;
      Py_DECREF(gatClass);
    }
  }
  ~CPythonEnv()
  {
    //Close Python Enviornment
    Py_DECREF(pModule);
    Py_DECREF(gat);
    Py_Finalize();
  }

  void loadModel(std::string file){
    PyObject *fileObject, *funName;

    funName = PyUnicode_DecodeFSDefault("load_model");

    fileObject = PyUnicode_DecodeFSDefault(file.c_str());

    PyObject* res = PyObject_CallMethodOneArg(gat, funName, fileObject);

    Py_DECREF(funName);
    Py_DECREF(fileObject);
    if(res==nullptr){
      std::cout<<"Failed to load the model\n";
      PyErr_Print();
    }  
  }

  std::string predict(std::vector<unsigned int> nodes, std::vector<unsigned int> outEdges, std::vector<unsigned int> inEdges, std::vector<unsigned int> edge_attr){
    PyObject *fun, *nodesObject, *inEdgesObject, *outEdgesObject, *edge_attrObject, *args, *pyint;

    fun = PyObject_GetAttrString(pModule, (char*)"predict");
    nodesObject = PyList_New(nodes.size());
    outEdgesObject = PyList_New(outEdges.size());
    inEdgesObject = PyList_New(inEdges.size());
    edge_attrObject = PyList_New(edge_attr.size());

    assert(outEdges.size() == inEdges.size());
    assert(outEdges.size() == edge_attr.size());
    int i;
    for(i=0; i<nodes.size(); ++i){
      pyint = Py_BuildValue("i", nodes[i]);
      PyList_SetItem(nodesObject, i, pyint);
    }
    for(i=0; i<outEdges.size(); ++i){
      pyint = Py_BuildValue("i", outEdges[i]);
      PyList_SetItem(outEdgesObject, i, pyint);
      pyint = Py_BuildValue("i", inEdges[i]);
      PyList_SetItem(inEdgesObject, i, pyint);
      pyint = Py_BuildValue("i", edge_attr[i]);
      PyList_SetItem(edge_attrObject, i, pyint);
    }

    args = PyTuple_New(5);
    PyTuple_SetItem(args, 0, gat);
    PyTuple_SetItem(args, 1, nodesObject);
    PyTuple_SetItem(args, 2, outEdgesObject);
    PyTuple_SetItem(args, 3, inEdgesObject);
    PyTuple_SetItem(args, 4, edge_attrObject);

    PyObject *res = PyObject_CallObject(fun, args);
    
    if(res==nullptr){
      std::cout<<"Couldn't Call Model\n";
      PyErr_Print();
    }
    std::string choice = PyUnicode_AsUTF8(res);
    Py_DECREF(args);
    Py_DECREF(res);

    return choice;
  }
};

class bmct
{
public:
  bmct(
    goto_functionst &funcs,
    optionst &opts,
    contextt &_context,
    const messaget &_message_handler);

  optionst &options;

  BigInt interleaving_number;
  BigInt interleaving_failed;
  bool use_sibyl;

  virtual smt_convt::resultt start_bmc();
  virtual smt_convt::resultt run(std::shared_ptr<symex_target_equationt> &eq);
  virtual ~bmct() = default;

protected:
  const contextt &context;
  namespacet ns;
  const messaget &msg;
  std::shared_ptr<smt_convt> prediction_solver;
  std::shared_ptr<smt_convt> runtime_solver;
  std::shared_ptr<reachability_treet> symex;
  virtual smt_convt::resultt run_decision_procedure(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);

  virtual void do_cbmc(
    std::shared_ptr<smt_convt> &smt_conv,
    std::shared_ptr<symex_target_equationt> &eq);

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

  smt_convt::resultt run_thread(std::shared_ptr<symex_target_equationt> &eq, CPythonEnv *env);
};

#endif
