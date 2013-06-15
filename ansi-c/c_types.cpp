/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <std_types.h>

#include "c_types.h"
#include "config.h"

typet build_float_type(unsigned width)
{
  if(config.ansi_c.use_fixed_for_float)
  {
    fixedbv_typet result;
    result.set_width(width);
    result.set_integer_bits(width/2);
    return result;
  }
  else
  {
    floatbv_typet result=floatbv_typet();
    result.set_width(width);

    switch(width)
    {
    case 32: result.set_f(23); break;
    case 64: result.set_f(52); break;
    default: assert(false);
    }

    return result;
  }
}

type2tc build_float_type2(unsigned width)
{
  if(config.ansi_c.use_fixed_for_float)
  {
    fixedbv_type2tc result(width, width/2);
    return result;
  }
  else
  {
    std::cerr << "New intermediate representation doesn't have floatbvs yet, "
              << "sorry" << std::endl;
    abort();
#if 0
    floatbv_typet result=floatbv_typet();
    result.set_width(width);

    switch(width)
    {
    case 32: result.set_f(23); break;
    case 64: result.set_f(52); break;
    default: assert(false);
    }

    return result;
#endif
  }
}

typet index_type()
{
  return signedbv_typet(config.ansi_c.int_width);  
}

type2tc index_type2(void)
{
  return type_pool.get_int(config.ansi_c.int_width);
}

typet enum_type()
{
  return signedbv_typet(config.ansi_c.int_width);  
}

typet int_type()
{
  return signedbv_typet(config.ansi_c.int_width);  
}

type2tc int_type2()
{
  return type_pool.get_int(config.ansi_c.int_width);
}

typet uint_type()
{
  return unsignedbv_typet(config.ansi_c.int_width);  
}

type2tc uint_type2()
{
  return type_pool.get_uint(config.ansi_c.int_width);
}

typet long_int_type()
{
  return signedbv_typet(config.ansi_c.long_int_width);  
}

type2tc long_int_type2()
{
  return get_int_type(config.ansi_c.long_int_width);  
}

typet long_long_int_type()
{
  return signedbv_typet(config.ansi_c.long_long_int_width);  
}

type2tc long_long_int_type2()
{
  return get_int_type(config.ansi_c.long_long_int_width);  
}

typet long_uint_type()
{
  return unsignedbv_typet(config.ansi_c.long_int_width);  
}

type2tc long_uint_type2()
{
  return get_uint_type(config.ansi_c.long_int_width);  
}

typet long_long_uint_type()
{
  return unsignedbv_typet(config.ansi_c.long_long_int_width);  
}

type2tc long_long_uint_type2()
{
  return get_uint_type(config.ansi_c.long_long_int_width);
}

typet char_type()
{
  if(config.ansi_c.char_is_unsigned)
    return unsignedbv_typet(config.ansi_c.char_width);
  else
    return signedbv_typet(config.ansi_c.char_width);
}

type2tc char_type2()
{
  if (config.ansi_c.char_is_unsigned)
    return type_pool.get_uint(config.ansi_c.char_width);
  else
    return type_pool.get_int(config.ansi_c.char_width);
}

typet float_type()
{
  return build_float_type(config.ansi_c.single_width);
}

type2tc float_type2()
{
  return build_float_type2(config.ansi_c.single_width);
}

typet double_type()
{
  return build_float_type(config.ansi_c.double_width);
}

type2tc double_type2()
{
  return build_float_type2(config.ansi_c.double_width);
}

typet long_double_type()
{
  return build_float_type(config.ansi_c.long_double_width);
}

type2tc long_double_type2()
{
  return build_float_type2(config.ansi_c.long_double_width);
}

typet size_type()
{
  return unsignedbv_typet(config.ansi_c.pointer_width);
}

typet signed_size_type()
{
  return signedbv_typet(config.ansi_c.pointer_width);
}
