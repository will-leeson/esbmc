/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_C_FINAL_H
#define CPROVER_C_FINAL_H

#include <context.h>
#include <iostream>
#include <message.h>

void c_finalize_expression(
  const contextt &context,
  exprt &expr,
  message_handlert &message_handler);

bool c_final(contextt &context, message_handlert &message_handler);

#endif
