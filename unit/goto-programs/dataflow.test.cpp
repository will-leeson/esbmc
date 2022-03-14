/*******************************************************************
 Module: Dataflow algorithms Test

 Author: Rafael Sá Menezes

 Date: May 2021

 Test Plan:
   - Available expressions.
 \*******************************************************************/

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include "../testing-utils/goto_factory.h"
#include <util/dataflow/available_expressions.h>

// ** Bounded loop unroller
// Check whether the object converts a loop properly

SCENARIO("the available expressions can detect which expressions need recomputing", "[algorithms]")
{
  GIVEN("A C program with available expressions")
 {
    std::istringstream program(
      "int main() {"
      "int a = nondet_int();"
      "int b = nondet_int();"
      "int x = a + b;"
      "int y = a * b;"
      "while(y > a + b)"
      "{ a++; x = a + b; }"
      "return a;"
      "}");
    auto goto_functions = goto_factory::get_goto_functions(program);
    REQUIRE(goto_functions.function_map.size() > 0);
    available_expressions_dataflow dataflow_alg(goto_functions);
    dataflow_alg.run();

    REQUIRE(goto_functions.function_map.size() > 0);

    for(auto it = goto_functions.function_map.begin();
          it != goto_functions.function_map.end(); it++)
      for(auto itt = it->second.body.instructions.begin();
          itt !=  it->second.body.instructions.end(); itt++)
          {
          dataflow_alg.entry_instructions(*it, itt);
          //abort();

          }
 }

  
}
