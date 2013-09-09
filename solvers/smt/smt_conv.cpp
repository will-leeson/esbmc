#include <sstream>
#include <set>
#include <iomanip>

#include <base_type.h>
#include <arith_tools.h>
#include <ansi-c/c_types.h>

#include "smt_conv.h"
#include <solvers/prop/literal.h>

// Helpers extracted from z3_convt.

static std::string
extract_magnitude(std::string v, unsigned width)
{
    return integer2string(binary2integer(v.substr(0, width / 2), true), 10);
}

static std::string
extract_fraction(std::string v, unsigned width)
{
    return integer2string(binary2integer(v.substr(width / 2, width), false), 10);
}

static std::string
double2string(double d)
{
  std::ostringstream format_message;
  format_message << std::setprecision(12) << d;
  return format_message.str();
}

static std::string
itos(int64_t i)
{
  std::stringstream ss;
  ss << i;
  return ss.str();
}

static unsigned int
get_member_name_field(const type2tc &t, const irep_idt &name)
{
  unsigned int idx = 0;
  const struct_union_data &data_ref =
          dynamic_cast<const struct_union_data &>(*t);

  forall_names(it, data_ref.member_names) {
    if (*it == name)
      break;
    idx++;
  }
  assert(idx != data_ref.member_names.size() &&
         "Member name of with expr not found in struct/union type");

  return idx;
}

static unsigned int
get_member_name_field(const type2tc &t, const expr2tc &name)
{
  const constant_string2t &str = to_constant_string2t(name);
  return get_member_name_field(t, str.value);
}

smt_convt::smt_convt(bool enable_cache, bool intmode, const namespacet &_ns,
                     bool is_cpp, bool _tuple_support, bool _nobools,
                     bool can_init_inf_arrays)
  : ctx_level(0), caching(enable_cache), int_encoding(intmode), ns(_ns),
    tuple_support(_tuple_support), no_bools_in_arrays(_nobools),
    can_init_unbounded_arrs(can_init_inf_arrays)
{
  std::vector<type2tc> members;
  std::vector<irep_idt> names;

  members.push_back(type_pool.get_uint(config.ansi_c.pointer_width));
  members.push_back(type_pool.get_uint(config.ansi_c.pointer_width));
  names.push_back(irep_idt("pointer_object"));
  names.push_back(irep_idt("pointer_offset"));

  struct_type2t *tmp = new struct_type2t(members, names, "pointer_struct");
  pointer_type_data = tmp;
  pointer_struct = type2tc(tmp);

  pointer_logic.push_back(pointer_logict());

  addr_space_sym_num.push_back(0);

  members.clear();
  names.clear();
  members.push_back(type_pool.get_uint(config.ansi_c.pointer_width));
  members.push_back(type_pool.get_uint(config.ansi_c.pointer_width));
  names.push_back(irep_idt("start"));
  names.push_back(irep_idt("end"));
  tmp = new struct_type2t(members, names, "addr_space_type");
  addr_space_type_data = tmp;
  addr_space_type = type2tc(tmp);

  addr_space_arr_type = type2tc(new array_type2t(addr_space_type,
                                                 expr2tc(), true)) ;

  addr_space_data.push_back(std::map<unsigned, unsigned>());

  machine_int = type2tc(new signedbv_type2t(config.ansi_c.int_width));
  machine_uint = type2tc(new unsignedbv_type2t(config.ansi_c.int_width));
  machine_ptr = type2tc(new unsignedbv_type2t(config.ansi_c.pointer_width));

  // Pick a modelling array to shoehorn initialization data into. Because
  // we don't yet have complete data for whether pointers are dynamic or not,
  // this is the one modelling array that absolutely _has_ to be initialized
  // to false for each element, which is going to be shoved into
  // convert_identifier_pointer.
  if (is_cpp) {
    dyn_info_arr_name = "cpp::__ESBMC_is_dynamic&0#1";
  } else {
    dyn_info_arr_name = "c::__ESBMC_is_dynamic&0#1";
  }
}

smt_convt::~smt_convt(void)
{
}

void
smt_convt::smt_post_init(void)
{
  if (int_encoding) {
    machine_int_sort = mk_sort(SMT_SORT_INT, false);
    machine_uint_sort = machine_int_sort;
  } else {
    machine_int_sort = mk_sort(SMT_SORT_BV, config.ansi_c.int_width, true);
    machine_uint_sort = mk_sort(SMT_SORT_BV, config.ansi_c.int_width, false);
  }

  init_addr_space_array();
}

void
smt_convt::push_ctx(void)
{
  addr_space_data.push_back(addr_space_data.back());
  addr_space_sym_num.push_back(addr_space_sym_num.back());
  pointer_logic.push_back(pointer_logic.back());

  ctx_level++;
}

void
smt_convt::pop_ctx(void)
{
  ctx_level--;

  union_varst::nth_index<1>::type &union_numindex = union_vars.get<1>();
  union_numindex.erase(ctx_level);
  smt_cachet::nth_index<1>::type &cache_numindex = smt_cache.get<1>();
  cache_numindex.erase(ctx_level);
  pointer_logic.pop_back();
  addr_space_sym_num.pop_back();
  addr_space_data.pop_back();
}

const smt_ast *
smt_convt::make_disjunct(const ast_vec &v)
{
  const smt_ast *args[v.size()];
  const smt_ast *result = NULL;
  unsigned int i = 0;

  // This is always true.
  if (v.size() == 0)
    return mk_smt_bool(true);

  // Slightly funky due to being morphed from lor:
  for (ast_vec::const_iterator it = v.begin(); it != v.end(); it++, i++) {
    args[i] = *it;
  }

  // Chain these.
  if (i > 1) {
    unsigned int j;
    const smt_ast *argstwo[2];
    const smt_sort *sort = mk_sort(SMT_SORT_BOOL);
    argstwo[0] = args[0];
    for (j = 1; j < i; j++) {
      argstwo[1] = args[j];
      argstwo[0] = mk_func_app(sort, SMT_FUNC_OR, argstwo, 2);
    }
    result = argstwo[0];
  } else {
    result = args[0];
  }

  return result;
}

const smt_ast *
smt_convt::make_conjunct(const ast_vec &v)
{
  const smt_ast *args[v.size()];
  const smt_ast *result;
  unsigned int i;

  // Funky on account of conversion from land...
  for (i = 0; i < v.size(); i++) {
    args[i] = v[i];
  }

  // Chain these.
  if (i > 1) {
    unsigned int j;
    const smt_ast *argstwo[2];
    const smt_sort *sort = mk_sort(SMT_SORT_BOOL);
    argstwo[0] = args[0];
    for (j = 1; j < i; j++) {
      argstwo[1] = args[j];
      argstwo[0] = mk_func_app(sort, SMT_FUNC_AND, argstwo, 2);
    }
    result = argstwo[0];
  } else {
    result = args[0];
  }

  return result;
}

const smt_ast *
smt_convt::invert_ast(const smt_ast *a)
{
  assert(a->sort->id == SMT_SORT_BOOL);
  return mk_func_app(a->sort, SMT_FUNC_NOT, &a, 1);
}

const smt_ast *
smt_convt::imply_ast(const smt_ast *a, const smt_ast *b)
{
  assert(a->sort->id == SMT_SORT_BOOL && b->sort->id == SMT_SORT_BOOL);
  const smt_ast *args[2];
  args[0] = a;
  args[1] = b;
  return mk_func_app(a->sort, SMT_FUNC_IMPLIES, args, 2);
}

void
smt_convt::set_to(const expr2tc &expr, bool value)
{

  const smt_ast *a = convert_ast(expr);
  if (value == false)
    a = invert_ast(a);
  assert_ast(a);

  // Workaround for the fact that we don't have a good way of encoding unions
  // into SMT. Just work out what the last assigned field is.
  if (is_equality2t(expr) && value) {
    const equality2t eq = to_equality2t(expr);
    if (is_union_type(eq.side_1->type) && is_with2t(eq.side_2)) {
      const symbol2t sym = to_symbol2t(eq.side_1);
      const with2t with = to_with2t(eq.side_2);
      const union_type2t &type = to_union_type(eq.side_1->type);
      const std::string &ref = sym.get_symbol_name();
      const constant_string2t &str = to_constant_string2t(with.update_field);

      unsigned int idx = 0;
      forall_names(it, type.member_names) {
        if (*it == str.value)
          break;
        idx++;
      }

      assert(idx != type.member_names.size() &&
             "Member name of with expr not found in struct/union type");

      union_var_mapt mapentry = { ref, idx, 0 };
      union_vars.insert(mapentry);
    }
  }
}

const smt_ast *
smt_convt::convert_ast(const expr2tc &expr)
{
  // Variable length array; constant array's and so forth can have hundreds
  // of fields.
  const smt_ast *args[expr->get_num_sub_exprs()];
  const smt_sort *sort;
  const smt_ast *a;
  unsigned int num_args, used_sorts = 0;
  bool seen_signed_operand = false;
  bool make_ints_reals = false;
  bool special_cases = true;

  if (caching) {
    smt_cachet::const_iterator cache_result = smt_cache.find(expr);
    if (cache_result != smt_cache.end())
      return (cache_result->ast);
  }

  // Second fail -- comparisons are turning up in 01_cbmc_abs1 that compare
  // ints and reals, which is invalid. So convert ints up to reals.
  if (int_encoding && expr->get_num_sub_exprs() >= 2 &&
                      (is_fixedbv_type((*expr->get_sub_expr(0))->type) ||
                       is_fixedbv_type((*expr->get_sub_expr(1))->type))) {
    make_ints_reals = true;
  }

  unsigned int i = 0;

  // FIXME: turn this into a lookup table
  if (is_constant_array2t(expr) || is_with2t(expr) || is_index2t(expr) ||
      is_address_of2t(expr) ||
      (is_equality2t(expr) && is_array_type(to_equality2t(expr).side_1->type)))
    // Nope; needs special handling
    goto nocvt;
  special_cases = false;

  // Convert /all the arguments/.
  forall_operands2(it, idx, expr) {
    args[i] = convert_ast(*it);

    if (make_ints_reals && args[i]->sort->id == SMT_SORT_INT) {
      args[i] = mk_func_app(mk_sort(SMT_SORT_REAL), SMT_FUNC_INT2REAL,
                            &args[i], 1);
    }

    used_sorts |= args[i]->sort->id;
    i++;
    if (is_signedbv_type(*it) || is_fixedbv_type(*it))
      seen_signed_operand = true;
  }
nocvt:

  num_args = i;

  sort = convert_sort(expr->type);

  const expr_op_convert *cvt = &smt_convert_table[expr->expr_id];

  // Irritating special case: if we're selecting a bool out of an array, and
  // we're in QF_AUFBV mode, do special handling.
  if ((!int_encoding && is_index2t(expr) && is_bool_type(expr->type) &&
       no_bools_in_arrays) ||
       special_cases)
    goto expr_handle_table;

  if ((int_encoding && cvt->int_mode_func > SMT_FUNC_INVALID) ||
      (!int_encoding && cvt->bv_mode_func_signed > SMT_FUNC_INVALID)) {
    assert(cvt->args == num_args);
    // An obvious check, but catches cases where we add a field to a future expr
    // and then fail to update the SMT layer, leading to an ignored field.

    // Now check sort types.
    if ((used_sorts | cvt->permitted_sorts) == cvt->permitted_sorts) {
      // Matches; we can just convert this.
      smt_func_kind k = (int_encoding) ? cvt->int_mode_func
                      : (seen_signed_operand)
                          ? cvt->bv_mode_func_signed
                          : cvt->bv_mode_func_unsigned;
      a = mk_func_app(sort, k, &args[0], cvt->args);
      goto done;
    }
  }

  if ((int_encoding && cvt->int_mode_func == SMT_FUNC_INVALID) ||
      (!int_encoding && cvt->bv_mode_func_signed == SMT_FUNC_INVALID)) {
    std::cerr << "Invalid expression " << get_expr_id(expr) << " for encoding "
      << "mode discovered, refusing to convert to SMT" << std::endl;
    abort();
  }

expr_handle_table:
  switch (expr->expr_id) {
  case expr2t::constant_int_id:
  case expr2t::constant_fixedbv_id:
  case expr2t::constant_bool_id:
  case expr2t::symbol_id:
    a = convert_terminal(expr);
    break;
  case expr2t::constant_string_id:
  {
    const constant_string2t &str = to_constant_string2t(expr);
    expr2tc newarr = str.to_array();
    a = convert_ast(newarr);
    break;
  }
  case expr2t::constant_struct_id:
  {
    a = tuple_create(expr);
    break;
  }
  case expr2t::constant_union_id:
  {
    a = union_create(expr);
    break;
  }
  case expr2t::constant_array_id:
  case expr2t::constant_array_of_id:
  {
    const array_type2t &arr = to_array_type(expr->type);
    if (!can_init_unbounded_arrs && arr.size_is_infinite) {
      // Don't honour inifinite sized array initializers. Modelling only.
      // If we have an array of tuples and no tuple support, use tuple_fresh.
      // Otherwise, mk_fresh.
      if ((is_structure_type(arr.subtype) || is_pointer_type(arr.subtype))
          && !tuple_support)
        a = tuple_fresh(sort);
      else
        a = mk_fresh(sort, "inf_array");
      break;
    }

    // Domain sort may be mesed with:
    const smt_sort *domain;
    if (int_encoding) {
      domain = machine_int_sort;
    } else {
      domain = mk_sort(SMT_SORT_BV, calculate_array_domain_width(arr), false);
    }

    if (is_struct_type(arr.subtype) || is_union_type(arr.subtype) ||
        is_pointer_type(arr.subtype))
      a = tuple_array_create_despatch(expr, domain);
    else
      a = array_create(expr);
    break;
  }
  case expr2t::add_id:
  case expr2t::sub_id:
  {
    a = convert_pointer_arith(expr, expr->type);
    break;
  }
  case expr2t::mul_id:
  {
    assert(!int_encoding);

    // Handle BV mode multiplies: for normal integers multiply normally, for
    // fixedbv apply hacks.
    if (is_fixedbv_type(expr)) {
      const mul2t &mul = to_mul2t(expr);
      const fixedbv_type2t &fbvt = to_fixedbv_type(mul.type);
      unsigned int fraction_bits = fbvt.width - fbvt.integer_bits;
      unsigned int topbit = mul.side_1->type->get_width();
      const smt_sort *s1 = convert_sort(mul.side_1->type);
      const smt_sort *s2 = convert_sort(mul.side_2->type);
      args[0] = convert_sign_ext(args[0], s1, topbit, fraction_bits);
      args[1] = convert_sign_ext(args[1], s2, topbit, fraction_bits);
      a = mk_func_app(sort, SMT_FUNC_BVMUL, args, 2);
      a = mk_extract(a, fbvt.width + fraction_bits - 1, fraction_bits, sort);
    } else {
      assert(is_bv_type(expr));
      a = mk_func_app(sort, SMT_FUNC_BVMUL, args, 2);
    }
    break;
  }
  case expr2t::div_id:
  {
    // Handle BV mode divisions. Similar arrangement to multiplies.
    if (int_encoding) {
      a = mk_func_app(sort, SMT_FUNC_DIV, args, 2);
    } else if (is_fixedbv_type(expr)) {
      const div2t &div = to_div2t(expr);
      fixedbvt fbt(migrate_expr_back(expr));

      unsigned int fraction_bits = fbt.spec.get_fraction_bits();
      unsigned int topbit2 = div.side_2->type->get_width();
      const smt_sort *s2 = convert_sort(div.side_2->type);

      args[1] = convert_sign_ext(args[1], s2, topbit2,fraction_bits);
      const smt_ast *zero = mk_smt_bvint(BigInt(0), false, fraction_bits);
      const smt_ast *op0 = args[0];
      const smt_ast *concatargs[2];
      concatargs[0] = op0;
      concatargs[1] = zero;
      args[0] = mk_func_app(s2, SMT_FUNC_CONCAT, concatargs, 2);

      // Sorts.
      a = mk_func_app(s2, SMT_FUNC_BVSDIV, args, 2);
      a = mk_extract(a, fbt.spec.width - 1, 0, s2);
    } else {
      assert(is_bv_type(expr));
      smt_func_kind k = (seen_signed_operand)
              ? cvt->bv_mode_func_signed
              : cvt->bv_mode_func_unsigned;
      a = mk_func_app(sort, k, args, 2);
    }
    break;
  }
  case expr2t::index_id:
  {
    a = convert_array_index(expr, sort);
    break;
  }
  case expr2t::with_id:
  {
    const with2t &with = to_with2t(expr);

    // We reach here if we're with'ing a struct, not an array. Or a bool.
    if (is_struct_type(expr->type) || is_union_type(expr)) {
      unsigned int idx = get_member_name_field(expr->type, with.update_field);
      a = tuple_update(convert_ast(with.source_value), idx, with.update_value);
    } else {
      a = convert_array_store(expr, sort);
    }
    break;
  }
  case expr2t::member_id:
  {
    a = convert_member(expr, args[0]);
    break;
  }
  case expr2t::same_object_id:
  {
    // Two projects, then comparison.
    smt_sort *s = convert_sort(pointer_type_data->members[0]);
    args[0] = tuple_project(args[0], s, 0);
    args[1] = tuple_project(args[1], s, 0);
    a = mk_func_app(sort, SMT_FUNC_EQ, &args[0], 2);
    break;
  }
  case expr2t::pointer_offset_id:
  {
    smt_sort *s = convert_sort(pointer_type_data->members[1]);
    const pointer_offset2t &obj = to_pointer_offset2t(expr);
    // Can you cay super irritating?
    const expr2tc *ptr = &obj.ptr_obj;
    while (is_typecast2t(*ptr) && !is_pointer_type((*ptr)))
      ptr = &to_typecast2t(*ptr).from;

    args[0] = convert_ast(*ptr);
    a = tuple_project(args[0], s, 1);
    break;
  }
  case expr2t::pointer_object_id:
  {
    smt_sort *s = convert_sort(pointer_type_data->members[0]);
    const pointer_object2t &obj = to_pointer_object2t(expr);
    // Can you cay super irritating?
    const expr2tc *ptr = &obj.ptr_obj;
    while (is_typecast2t(*ptr) && !is_pointer_type((*ptr)))
      ptr = &to_typecast2t(*ptr).from;

    args[0] = convert_ast(*ptr);
    a = tuple_project(args[0], s, 0);
    break;
  }
  case expr2t::typecast_id:
  {
    a = convert_typecast(expr);
    break;
  }
  case expr2t::if_id:
  {
    // Only attempt to handle struct.s
    const if2t &if_ref = to_if2t(expr);
    if (is_structure_type(expr) || is_pointer_type(expr)) {
      a = tuple_ite(if_ref.cond, if_ref.true_value, if_ref.false_value,
                    if_ref.type);
    } else {
      assert(is_array_type(expr));
      a = tuple_array_ite(if_ref.cond, if_ref.true_value, if_ref.false_value);
    }
    break;
  }
  case expr2t::isnan_id:
  {
    a = convert_is_nan(expr, args[0]);
    break;
  }
  case expr2t::overflow_id:
  {
    a = overflow_arith(expr);
    break;
  }
  case expr2t::overflow_cast_id:
  {
    a = overflow_cast(expr);
    break;
  }
  case expr2t::overflow_neg_id:
  {
    a = overflow_neg(expr);
    break;
  }
  case expr2t::zero_length_string_id:
  {
    // Extremely unclear.
    a = tuple_project(args[0], sort, 0);
    break;
  }
  case expr2t::zero_string_id:
  {
    // Actually broken. And always has been.
    a = mk_smt_symbol("zero_string", sort);
    break;
  }
  case expr2t::byte_extract_id:
  {
    a = convert_byte_extract(expr);
    break;
  }
  case expr2t::byte_update_id:
  {
    a = convert_byte_update(expr);
    break;
  }
  case expr2t::address_of_id:
  {
    a = convert_addr_of(expr);
    break;
  }
  case expr2t::equality_id:
  {
    const equality2t &eq = to_equality2t(expr);
    if (is_struct_type(eq.side_1->type) && is_struct_type(eq.side_2->type)) {
      // Struct equality
      a = tuple_equality(args[0], args[1]);
    } else if (is_array_type(eq.side_1->type) &&
               is_array_type(eq.side_2->type)) {
      if (is_structure_type(to_array_type(eq.side_1->type).subtype) ||
          is_pointer_type(to_array_type(eq.side_1->type).subtype)) {
        // Array of structs equality.
        args[0] = convert_ast(eq.side_1);
        args[1] = convert_ast(eq.side_2);
        a = tuple_array_equality(args[0], args[1]);
      } else {
        // Normal array equality
        a = convert_array_equality(eq.side_1, eq.side_2);
      }
    } else if (is_pointer_type(eq.side_1) && is_pointer_type(eq.side_2)) {
      // Pointers are tuples
      a = tuple_equality(args[0], args[1]);
    } else if (is_union_type(eq.side_1) && is_union_type(eq.side_2)) {
      // Unions are also tuples
      a = tuple_equality(args[0], args[1]);
    } else {
      std::cerr << "Unrecognized equality form" << std::endl;
      expr->dump();
      abort();
    }
    break;
  }
  case expr2t::shl_id:
  {
    const shl2t &shl = to_shl2t(expr);

    if (shl.side_1->type->get_width() != shl.side_2->type->get_width()) {
      // FIXME: frontend doesn't cast the second operand up to the width of
      // the first, which SMT does not enjoy.
      typecast2tc cast(shl.side_1->type, shl.side_2);
      args[1] = convert_ast(cast);
    }

    if (int_encoding) {
      // Raise 2^shift, then multiply first operand by that value. If it's
      // negative, what to do? FIXME.
      constant_int2tc two(shl.type, BigInt(2));
      const smt_ast *powargs[2];
      powargs[0] = args[1];
      powargs[1] = convert_ast(two);
      args[1] = mk_func_app(sort, SMT_FUNC_POW, &powargs[0], 2);
      a = mk_func_app(sort, SMT_FUNC_MUL, &args[0], 2);
    } else {
      a = mk_func_app(sort, SMT_FUNC_BVSHL, &args[0], 2);
    }
    break;
  }
  case expr2t::ashr_id:
  {
    const ashr2t &ashr = to_ashr2t(expr);

    if (ashr.side_1->type->get_width() != ashr.side_2->type->get_width()) {
      // FIXME: frontend doesn't cast the second operand up to the width of
      // the first, which SMT does not enjoy.
      typecast2tc cast(ashr.side_1->type, ashr.side_2);
      args[1] = convert_ast(cast);
    }

    if (int_encoding) {
      // Raise 2^shift, then divide first operand by that value. If it's
      // negative, I suspect the correct operation is to latch to -1,
      // XXX XXX XXX haven't implemented that yet.
      constant_int2tc two(ashr.type, BigInt(2));
      const smt_ast *powargs[2];
      powargs[0] = args[1];
      powargs[1] = convert_ast(two);
      args[1] = mk_func_app(sort, SMT_FUNC_POW, &powargs[0], 2);
      a = mk_func_app(sort, SMT_FUNC_DIV, &args[0], 2);
    } else {
      a = mk_func_app(sort, SMT_FUNC_BVASHR, &args[0], 2);
    }
    break;
  }
  case expr2t::lshr_id:
  {
    // Like ashr. Haven't got around to cleaning this up yet.
    const lshr2t &lshr = to_lshr2t(expr);

    if (lshr.side_1->type->get_width() != lshr.side_2->type->get_width()) {
      // FIXME: frontend doesn't cast the second operand up to the width of
      // the first, which SMT does not enjoy.
      typecast2tc cast(lshr.side_1->type, lshr.side_2);
      args[1] = convert_ast(cast);
    }

    if (int_encoding) {
      // Raise 2^shift, then divide first operand by that value. If it's
      // negative, I suspect the correct operation is to latch to -1,
      // XXX XXX XXX haven't implemented that yet.
      constant_int2tc two(lshr.type, BigInt(2));
      const smt_ast *powargs[2];
      powargs[0] = args[1];
      powargs[1] = convert_ast(two);
      args[1] = mk_func_app(sort, SMT_FUNC_POW, &powargs[0], 2);
      a = mk_func_app(sort, SMT_FUNC_DIV, &args[0], 2);
    } else {
      a = mk_func_app(sort, SMT_FUNC_BVLSHR, &args[0], 2);
    }
    break;
  }
  case expr2t::notequal_id:
  {
    const notequal2t &notequal = to_notequal2t(expr);
    // Handle all kinds of structs by inverted equality. The only that's really
    // going to turn up is pointers though.
    if (is_structure_type(notequal.side_1) ||is_pointer_type(notequal.side_1)) {
      a = tuple_equality(args[0], args[1]);
      a = mk_func_app(sort, SMT_FUNC_NOT, &a, 1);
    } else {
      std::cerr << "Unexpected inequailty operands" << std::endl;
      expr->dump();
      abort();
    }
    break;
  }
  case expr2t::abs_id:
  {
    const abs2t &abs = to_abs2t(expr);
    if (is_unsignedbv_type(abs.value)) {
      // No need to do anything.
      a = args[0];
    } else {
      constant_int2tc zero(abs.value->type, BigInt(0));
      lessthan2tc lt(abs.value, zero);
      sub2tc sub(abs.value->type, zero, abs.value);
      if2tc ite(abs.type, lt, sub, abs.value);
      a = convert_ast(ite);
    }
    break;
  }
  case expr2t::lessthan_id:
  case expr2t::lessthanequal_id:
  case expr2t::greaterthan_id:
  case expr2t::greaterthanequal_id:
  {
    // Pointer relation:
    const expr2tc &side1 = *expr->get_sub_expr(0);
    const expr2tc &side2 = *expr->get_sub_expr(1);
    if (is_pointer_type(side1->type) && is_pointer_type(side2->type)) {
      a = convert_ptr_cmp(side1, side2, expr);
    } else {
      // One operand isn't a pointer; go the slow way, with typecasts.
      type2tc inttype = machine_ptr;
      expr2tc cast1 = (!is_unsignedbv_type(side1))
        ? typecast2tc(inttype, side1)
        : side1;
      expr2tc cast2 = (!is_unsignedbv_type(side2))
        ? typecast2tc(inttype, side2)
        : side2;
      expr2tc new_expr = expr;
      *new_expr.get()->get_sub_expr_nc(0) = cast1;
      *new_expr.get()->get_sub_expr_nc(1) = cast2;
      a = convert_ast(new_expr);
    }
    break;
  }
  case expr2t::concat_id:
  {
    assert(!int_encoding && "Concatonate encountered in integer mode; "
           "unimplemented (and funky)");
    const concat2t &cat = to_concat2t(expr);
    std::vector<expr2tc>::const_iterator it = cat.data_items.begin();
    args[0] = convert_ast(*it);
    unsigned long accuml_size = (*it)->type->get_width();
    it++;
    for (; it != cat.data_items.end() ;it++) {
      accuml_size += (*it)->type->get_width();
      const smt_sort *s = mk_sort(SMT_SORT_BV, accuml_size, false);
      args[1] = convert_ast(*it);
      args[0] = mk_func_app(s, SMT_FUNC_CONCAT, args, 2);
    }

    a = args[0];
    break;
  }
  default:
    std::cerr << "Couldn't convert expression in unrecognized format"
              << std::endl;
    expr->dump();
    abort();
  }

done:
  if (caching) {
    struct smt_cache_entryt entry = { expr, a, ctx_level };
    smt_cache.insert(entry);
  }

  return a;
}

void
smt_convt::assert_expr(const expr2tc &e)
{
  assert_ast(convert_ast(e));
  return;
}

smt_sort *
smt_convt::convert_sort(const type2tc &type)
{
  bool is_signed = true;

  switch (type->type_id) {
  case type2t::bool_id:
    return mk_sort(SMT_SORT_BOOL);
  case type2t::struct_id:
    if (!tuple_support) {
      return new tuple_smt_sort(type);
    } else {
      return mk_struct_sort(type);
    }
  case type2t::union_id:
    if (!tuple_support) {
      return new tuple_smt_sort(type);
    } else {
      return mk_union_sort(type);
    }
  case type2t::code_id:
  case type2t::pointer_id:
    if (!tuple_support) {
      return new tuple_smt_sort(pointer_struct);
    } else {
      return mk_struct_sort(pointer_struct);
    }
  case type2t::unsignedbv_id:
    is_signed = false;
    /* FALLTHROUGH */
  case type2t::signedbv_id:
  {
    unsigned int width = type->get_width();
    if (int_encoding)
      return mk_sort(SMT_SORT_INT, is_signed);
    else
      return mk_sort(SMT_SORT_BV, width, is_signed);
  }
  case type2t::fixedbv_id:
  {
    unsigned int width = type->get_width();
    if (int_encoding)
      return mk_sort(SMT_SORT_REAL);
    else
      return mk_sort(SMT_SORT_BV, width, false);
  }
  case type2t::string_id:
  {
    const string_type2t &str_type = to_string_type(type);
    constant_int2tc width(get_uint_type(config.ansi_c.int_width),
                          BigInt(str_type.width));
    type2tc new_type(new array_type2t(get_uint8_type(), width, false));
    return convert_sort(new_type);
  }
  case type2t::array_id:
  {
    const array_type2t &arr = to_array_type(type);

    // Index arrays by the smallest integer required to represent its size.
    // Unless it's either infinite or dynamic in size, in which case use the
    // machine int size. Also, faff about if it's an array of arrays, extending
    // the domain.
    const smt_sort *d = make_array_domain_sort(arr);

    // Determine the range if we have arrays of arrays.
    type2tc range = arr.subtype;
    while (is_array_type(range))
      range = to_array_type(range).subtype;

    if (!tuple_support && (is_structure_type(range) || is_pointer_type(range))){
      return new tuple_smt_sort(type, calculate_array_domain_width(arr));
    }

    // Work around QF_AUFBV demanding arrays of bitvectors.
    smt_sort *r;
    if (!int_encoding && is_bool_type(range) && no_bools_in_arrays) {
      r = mk_sort(SMT_SORT_BV, 1, false);
    } else {
      r = convert_sort(range);
    }
    return mk_sort(SMT_SORT_ARRAY, d, r);
  }
  case type2t::cpp_name_id:
  case type2t::symbol_id:
  case type2t::empty_id:
  default:
    std::cerr << "Unexpected type ID reached SMT conversion" << std::endl;
    abort();
  }
}

static std::string
fixed_point(std::string v, unsigned width)
{
  const int precision = 1000000;
  std::string i, f, b, result;
  double integer, fraction, base;

  i = extract_magnitude(v, width);
  f = extract_fraction(v, width);
  b = integer2string(power(2, width / 2), 10);

  integer = atof(i.c_str());
  fraction = atof(f.c_str());
  base = (atof(b.c_str()));

  fraction = (fraction / base);

  if (fraction < 0)
    fraction = -fraction;

   fraction = fraction * precision;

  if (fraction == 0)
    result = double2string(integer);
  else  {
    int64_t numerator = (integer*precision + fraction);
    result = itos(numerator) + "/" + double2string(precision);
  }

  return result;
}

smt_ast *
smt_convt::convert_terminal(const expr2tc &expr)
{
  switch (expr->expr_id) {
  case expr2t::constant_int_id:
  {
    bool sign = is_signedbv_type(expr);
    const constant_int2t &theint = to_constant_int2t(expr);
    unsigned int width = expr->type->get_width();
    if (int_encoding)
      return mk_smt_int(theint.constant_value, sign);
    else
      return mk_smt_bvint(theint.constant_value, sign, width);
  }
  case expr2t::constant_fixedbv_id:
  {
    const constant_fixedbv2t &thereal = to_constant_fixedbv2t(expr);
    if (int_encoding) {
      std::string val = thereal.value.to_expr().value().as_string();
      std::string result = fixed_point(val, thereal.value.spec.width);
      return mk_smt_real(result);
    } else {
      assert(thereal.type->get_width() <= 64 && "Converting fixedbv constant to"
             " SMT, too large to fit into a uint64_t");

      uint64_t magnitude, fraction, fin;
      unsigned int bitwidth = thereal.type->get_width();
      std::string m, f, c;
      std::string theval = thereal.value.to_expr().value().as_string();

      m = extract_magnitude(theval, bitwidth);
      f = extract_fraction(theval, bitwidth);
      magnitude = strtoll(m.c_str(), NULL, 10);
      fraction = strtoll(f.c_str(), NULL, 10);

      magnitude <<= (bitwidth / 2);
      fin = magnitude | fraction;

      return mk_smt_bvint(mp_integer(fin), false, bitwidth);
    }
  }
  case expr2t::constant_bool_id:
  {
    const constant_bool2t &thebool = to_constant_bool2t(expr);
    return mk_smt_bool(thebool.constant_value);
  }
  case expr2t::symbol_id:
  {
    // Special case for tuple symbols
    if (!tuple_support &&
        (is_union_type(expr) || is_struct_type(expr) || is_pointer_type(expr))){
      // Perform smt-tuple hacks.
      return mk_tuple_symbol(expr);
    } else if (!tuple_support && is_array_type(expr)) {
      // Determine the range if we have arrays of arrays.
      const array_type2t &arr = to_array_type(expr->type);
      type2tc range = arr.subtype;
      while (is_array_type(range))
        range = to_array_type(range).subtype;

      // If this is an array of structs, we have a tuple array sym.
      if (is_structure_type(range) || is_pointer_type(range)) {
        return mk_tuple_array_symbol(expr);
      } else {
        ; // continue onwards;
      }
    }

    // Just a normal symbol.
    const symbol2t &sym = to_symbol2t(expr);
    std::string name = sym.get_symbol_name();
    const smt_sort *sort = convert_sort(sym.type);
    return mk_smt_symbol(name, sort);
  }
  default:
    std::cerr << "Converting unrecognized terminal expr to SMT" << std::endl;
    expr->dump();
    abort();
  }
}

std::string
smt_convt::mk_fresh_name(const std::string &tag)
{
  std::string new_name = "smt_conv::" + tag;
  std::stringstream ss;
  ss << new_name << fresh_map[new_name]++;
  return ss.str();
}

smt_ast *
smt_convt::mk_fresh(const smt_sort *s, const std::string &tag)
{
  return mk_smt_symbol(mk_fresh_name(tag), s);
}

const smt_ast *
smt_convt::convert_is_nan(const expr2tc &expr, const smt_ast *operand)
{
  const smt_ast *args[3];
  const isnan2t &isnan = to_isnan2t(expr);
  const smt_sort *bs = mk_sort(SMT_SORT_BOOL);

  // Assumes operand is fixedbv.
  assert(is_fixedbv_type(isnan.value));
  unsigned width = isnan.value->type->get_width();

  const smt_ast *t = mk_smt_bool(true);
  const smt_ast *f = mk_smt_bool(false);

  if (int_encoding) {
    args[0] = round_real_to_int(operand);
    args[1] = mk_smt_int(BigInt(0), false);
    args[0] = mk_func_app(bs, SMT_FUNC_GTE, args, 2);
    args[1] = t;
    args[2] = f;
    return mk_func_app(bs, SMT_FUNC_ITE, args, 3);
  } else {
    args[0] = operand;
    args[1] = mk_smt_bvint(BigInt(0), false, width);
    args[0] = mk_func_app(bs, SMT_FUNC_GTE, args, 2);
    args[1] = t;
    args[2] = f;
    return mk_func_app(bs, SMT_FUNC_ITE, args, 3);
  }
}

const smt_ast *
smt_convt::convert_member(const expr2tc &expr, const smt_ast *src)
{
  const smt_sort *sort = convert_sort(expr->type);
  const member2t &member = to_member2t(expr);
  unsigned int idx = -1;

  if (is_union_type(member.source_value->type)) {
    union_varst::const_iterator cache_result;
    const union_type2t &data_ref = to_union_type(member.source_value->type);

    if (is_symbol2t(member.source_value)) {
      const symbol2t &sym = to_symbol2t(member.source_value);
      cache_result = union_vars.find(sym.get_symbol_name().c_str());
    } else {
      cache_result = union_vars.end();
    }

    if (cache_result != union_vars.end()) {
      const std::vector<type2tc> &members = data_ref.get_structure_members();

      const type2tc source_type = members[cache_result->idx];
      if (source_type == member.type) {
        // Type we're fetching from union matches expected type; just return it.
        idx = cache_result->idx;
      } else {
        // Union field and expected type mismatch. Need to insert a cast.
        // Duplicate expr as we're changing it
        member2tc memb2(source_type, member.source_value, member.member);
        typecast2tc cast(member.type, memb2);
        return convert_ast(cast);
      }
    } else {
      // If no assigned result available, we're probably broken for whatever
      // reason, just go haywire.
      idx = get_member_name_field(member.source_value->type, member.member);
    }
  } else {
    idx = get_member_name_field(member.source_value->type, member.member);
  }

  return tuple_project(src, sort, idx);
}

const smt_ast *
smt_convt::convert_sign_ext(const smt_ast *a, const smt_sort *s,
                            unsigned int topbit, unsigned int topwidth)
{
  const smt_ast *args[4];

  const smt_sort *bit = mk_sort(SMT_SORT_BV, 1, false);
  args[0] = mk_extract(a, topbit-1, topbit-1, bit);
  args[1] = mk_smt_bvint(BigInt(0), false, 1);
  const smt_sort *b = mk_sort(SMT_SORT_BOOL);
  const smt_ast *t = mk_func_app(b, SMT_FUNC_EQ, args, 2);

  const smt_ast *z = mk_smt_bvint(BigInt(0), false, topwidth);

  // Calculate the exact value; SMTLIB text parsers don't like taking an
  // over-full integer literal.
  uint64_t big = 0xFFFFFFFFFFFFFFFFULL;
  unsigned int num_topbits = 64 - topwidth;
  big >>= num_topbits;
  BigInt big_int(big);
  const smt_ast *f = mk_smt_bvint(big_int, false, topwidth);

  args[0] = t;
  args[1] = z;
  args[2] = f;
  const smt_sort *topsort = mk_sort(SMT_SORT_BV, topwidth, false);
  const smt_ast *topbits = mk_func_app(topsort, SMT_FUNC_ITE, args, 3);

  args[0] = topbits;
  args[1] = a;
  return mk_func_app(s, SMT_FUNC_CONCAT, args, 2);
}

const smt_ast *
smt_convt::convert_zero_ext(const smt_ast *a, const smt_sort *s,
                            unsigned int topwidth)
{
  const smt_ast *args[2];

  const smt_ast *z = mk_smt_bvint(BigInt(0), false, topwidth);
  args[0] = z;
  args[1] = a;
  return mk_func_app(s, SMT_FUNC_CONCAT, args, 2);
}

const smt_ast *
smt_convt::round_real_to_int(const smt_ast *a)
{
  // SMT truncates downwards; however C truncates towards zero, which is not
  // the same. (Technically, it's also platform dependant). To get around this,
  // add one to the result in all circumstances, except where the value was
  // already an integer.
  const smt_sort *realsort = mk_sort(SMT_SORT_REAL);
  const smt_sort *intsort = mk_sort(SMT_SORT_INT);
  const smt_sort *boolsort = mk_sort(SMT_SORT_BOOL);
  const smt_ast *args[3];
  args[0] = a;
  args[1] = mk_smt_real("0");
  const smt_ast *is_lt_zero = mk_func_app(realsort, SMT_FUNC_LT, args, 2);

  // The actual conversion
  const smt_ast *as_int = mk_func_app(intsort, SMT_FUNC_REAL2INT, args, 2);

  const smt_ast *one = mk_smt_int(BigInt(1), false);
  args[0] = one;
  args[1] = as_int;
  const smt_ast *plus_one = mk_func_app(intsort, SMT_FUNC_ADD, args, 2);

  // If it's an integer, just keep it's untruncated value.
  args[0] = mk_func_app(boolsort, SMT_FUNC_IS_INT, &a, 1);
  args[1] = as_int;
  args[2] = plus_one;
  args[1] = mk_func_app(intsort, SMT_FUNC_ITE, args, 3);

  // Switch on whether it's > or < 0.
  args[0] = is_lt_zero;
  args[2] = as_int;
  return mk_func_app(intsort, SMT_FUNC_ITE, args, 3);
}

const smt_ast *
smt_convt::round_fixedbv_to_int(const smt_ast *a, unsigned int fromwidth,
                                unsigned int towidth)
{
  // Perform C rounding: just truncate towards zero. Annoyingly, this isn't
  // that simple for negative numbers, because they're represented as a negative
  // integer _plus_ a positive fraction. So we need to round up if there's a
  // nonzero fraction, and not if there's not.
  const smt_ast *args[3];
  unsigned int frac_width = fromwidth / 2;

  // Sorts
  const smt_sort *bit = mk_sort(SMT_SORT_BV, 1, false);
  const smt_sort *halfwidth = mk_sort(SMT_SORT_BV, frac_width, false);
  const smt_sort *tosort = mk_sort(SMT_SORT_BV, towidth, false);
  const smt_sort *boolsort = mk_sort(SMT_SORT_BOOL);

  // Determine whether the source is signed from its topmost bit.
  const smt_ast *is_neg_bit = mk_extract(a, fromwidth-1, fromwidth-1, bit);
  const smt_ast *true_bit = mk_smt_bvint(BigInt(1), false, 1);

  // Also collect data for dealing with the magnitude.
  const smt_ast *magnitude = mk_extract(a, fromwidth-1, frac_width, halfwidth);
  const smt_ast *intvalue = convert_sign_ext(magnitude, tosort, frac_width,
                                             frac_width);

  // Data for inspecting fraction part
  const smt_ast *frac_part = mk_extract(a, frac_width-1, 0, bit);
  const smt_ast *zero = mk_smt_bvint(BigInt(0), false, frac_width);
  args[0] = frac_part;
  args[1] = zero;
  const smt_ast *is_zero_frac = mk_func_app(boolsort, SMT_FUNC_EQ, args, 2);

  // So, we have a base number (the magnitude), and need to decide whether to
  // round up or down. If it's positive, round down towards zero. If it's neg
  // and the fraction is zero, leave it, otherwise round towards zero.

  // We may need a value + 1.
  args[0] = intvalue;
  args[1] = mk_smt_bvint(BigInt(1), false, towidth);
  const smt_ast *intvalue_plus_one =
    mk_func_app(tosort, SMT_FUNC_BVADD, args, 2);

  args[0] = is_zero_frac;
  args[1] = intvalue;
  args[2] = intvalue_plus_one;
  const smt_ast *neg_val = mk_func_app(tosort, SMT_FUNC_ITE, args, 3);

  args[0] = true_bit;
  args[1] = is_neg_bit;
  const smt_ast *is_neg = mk_func_app(boolsort, SMT_FUNC_EQ, args, 2);

  // final switch
  args[0] = is_neg;
  args[1] = neg_val;
  args[2] = intvalue;
  return mk_func_app(tosort, SMT_FUNC_ITE, args, 3);
}

const smt_ast *
smt_convt::make_bool_bit(const smt_ast *a)
{

  assert(a->sort->id == SMT_SORT_BOOL && "Wrong sort fed to "
         "smt_convt::make_bool_bit");
  const smt_ast *one = mk_smt_bvint(BigInt(1), false, 1);
  const smt_ast *zero = mk_smt_bvint(BigInt(0), false, 1);
  const smt_ast *args[3];
  args[0] = a;
  args[1] = one;
  args[2] = zero;
  return mk_func_app(one->sort, SMT_FUNC_ITE, args, 3);
}

const smt_ast *
smt_convt::make_bit_bool(const smt_ast *a)
{

  assert(a->sort->id == SMT_SORT_BV && "Wrong sort fed to "
         "smt_convt::make_bit_bool");
  const smt_sort *boolsort = mk_sort(SMT_SORT_BOOL);
  const smt_ast *one = mk_smt_bvint(BigInt(1), false, 1);
  const smt_ast *args[2];
  args[0] = a;
  args[1] = one;
  return mk_func_app(boolsort, SMT_FUNC_EQ, args, 2);
}

expr2tc
smt_convt::fix_array_idx(const expr2tc &idx, const type2tc &arr_sort)
{
  if (int_encoding)
    return idx;

  const smt_sort *s = convert_sort(arr_sort);
  unsigned int domain_width = s->get_domain_width();
  if (domain_width == config.ansi_c.int_width)
    return idx;

  // Otherwise, we need to extract the lower bits out of this.
  return typecast2tc(get_uint_type(domain_width), idx);
}

unsigned long
smt_convt::size_to_bit_width(unsigned long sz)
{
  uint64_t domwidth = 2;
  unsigned int dombits = 1;

  // Shift domwidth up until it's either larger or equal to sz, or we risk
  // overflowing.
  while (domwidth != 0x8000000000000000ULL && domwidth < sz) {
    domwidth <<= 1;
    dombits++;
  }

  if (domwidth == 0x8000000000000000ULL)
    dombits = 64;

  return dombits;
}

unsigned long
smt_convt::calculate_array_domain_width(const array_type2t &arr)
{
  // Index arrays by the smallest integer required to represent its size.
  // Unless it's either infinite or dynamic in size, in which case use the
  // machine int size.
  if (!is_nil_expr(arr.array_size) && is_constant_int2t(arr.array_size)) {
    constant_int2tc thesize = arr.array_size;
    return size_to_bit_width(thesize->constant_value.to_ulong());
  } else {
    return config.ansi_c.int_width;
  }
}

const smt_sort *
smt_convt::make_array_domain_sort(const array_type2t &arr)
{

  // Start special casing if this is an array of arrays.
  if (!is_array_type(arr.subtype)) {
    // Normal array, work out what the domain sort is.
    if (int_encoding)
      return mk_sort(SMT_SORT_INT);
    else
      return mk_sort(SMT_SORT_BV, calculate_array_domain_width(arr), false);
  } else {
    // This is an array of arrays -- we're going to convert this into a single
    // array that has an extended domain. Work out that width. Firstly, how
    // many levels of array do we have?

    unsigned int how_many_arrays = 1;
    type2tc subarr = arr.subtype;
    while (is_array_type(subarr)) {
      how_many_arrays++;
      subarr = to_array_type(subarr).subtype;
    }

    assert(how_many_arrays < 64 && "Suspiciously large number of array "
                                   "dimensions");
    unsigned int domwidth;
    unsigned int i;
    domwidth = calculate_array_domain_width(arr);
    subarr = arr.subtype;
    for (i = 1; i < how_many_arrays; i++) {
      domwidth += calculate_array_domain_width(to_array_type(arr.subtype));
      subarr = arr.subtype;
    }

    return mk_sort(SMT_SORT_BV, domwidth, false);
  }
}

type2tc
smt_convt::make_array_domain_sort_exp(const array_type2t &arr)
{

  // Start special casing if this is an array of arrays.
  if (!is_array_type(arr.subtype)) {
    // Normal array, work out what the domain sort is.
    if (int_encoding)
      return get_uint_type(config.ansi_c.int_width);
    else
      return get_uint_type(calculate_array_domain_width(arr));
  } else {
    // This is an array of arrays -- we're going to convert this into a single
    // array that has an extended domain. Work out that width. Firstly, how
    // many levels of array do we have?

    unsigned int how_many_arrays = 1;
    type2tc subarr = arr.subtype;
    while (is_array_type(subarr)) {
      how_many_arrays++;
      subarr = to_array_type(subarr).subtype;
    }

    assert(how_many_arrays < 64 && "Suspiciously large number of array "
                                   "dimensions");
    unsigned int domwidth;
    unsigned int i;
    domwidth = calculate_array_domain_width(arr);
    subarr = arr.subtype;
    for (i = 1; i < how_many_arrays; i++) {
      domwidth += calculate_array_domain_width(to_array_type(arr.subtype));
      subarr = arr.subtype;
    }

    return get_uint_type(domwidth);
  }
}

expr2tc
smt_convt::array_domain_to_width(const type2tc &type)
{
  const unsignedbv_type2t &uint = to_unsignedbv_type(type);
  uint64_t sz = 1ULL << uint.width;
  return constant_int2tc(index_type2(), BigInt(sz));
}

expr2tc
smt_convt::twiddle_index_width(const expr2tc &expr, const type2tc &type)
{
  const array_type2t &arrtype = to_array_type(type);
  unsigned int width = calculate_array_domain_width(arrtype);
  typecast2tc t(type2tc(new unsignedbv_type2t(width)), expr);
  expr2tc tmp = t->simplify();
  if (is_nil_expr(tmp))
    return t;
  else
    return tmp;
}

expr2tc
smt_convt::decompose_select_chain(const expr2tc &expr, expr2tc &base)
{
  // So: some series of index exprs will occur here, with some symbol or
  // other expression at the bottom that's actually some symbol, or whatever.
  // So, extract all the indexes, and concat them, with the first (lowest)
  // index at the top, then descending.

  unsigned long accuml_size = 0;
  std::vector<expr2tc> output;
  index2tc idx = expr;
  output.push_back(twiddle_index_width(idx->index, idx->source_value->type));
  accuml_size += output.back()->type->get_width();
  while (is_index2t(idx->source_value)) {
    idx = idx->source_value;
    output.push_back(twiddle_index_width(idx->index, idx->source_value->type));
    accuml_size += output.back()->type->get_width();
  }

  concat2tc concat(get_uint_type(accuml_size), output);

  // Give the caller the base array object / thing. So that it can actually
  // select out of the right piece of data.
  base = idx->source_value;
  return concat;
}

expr2tc
smt_convt::decompose_store_chain(const expr2tc &expr, expr2tc &base)
{
  // Just like handle_select_chain, we have some kind of multidimensional
  // array, which we're representing as a single array with an extended domain,
  // and using different segments of the domain to represent different
  // dimensions of it. Concat all of the indexs into one index; also give the
  // caller the base object that this is being applied to.

  unsigned long accuml_size = 0;
  std::vector<expr2tc> output;
  with2tc with = expr;
  output.push_back(twiddle_index_width(with->update_field, with->type));
  accuml_size += output.back()->type->get_width();
  while (is_with2t(with->update_value)) {
    with = with->update_value;
    output.push_back(twiddle_index_width(with->update_field, with->type));
    accuml_size += output.back()->type->get_width();
  }

  // With's are in reverse order to indexes; so swap around.
  std::reverse(output.begin(), output.end());

  concat2tc concat(get_uint_type(accuml_size), output);

  // Give the caller the actual value we're updating with.
  base = with->update_value;
  return concat;
}

const smt_ast *
smt_convt::convert_array_index(const expr2tc &expr, const smt_sort *ressort)
{
  const smt_ast *a;
  const index2t &index = to_index2t(expr);
  expr2tc src_value = index.source_value;
  expr2tc newidx;

  if (is_index2t(index.source_value)) {
    newidx = decompose_select_chain(expr, src_value);
  } else {
    newidx = fix_array_idx(index.index, index.source_value->type);
  }

  expr2tc tmp_idx = newidx->simplify();
  if (!is_nil_expr(tmp_idx))
    newidx = tmp_idx;

  // Firstly, if it's a string, shortcircuit.
  if (is_string_type(index.source_value)) {
    return mk_select(src_value, newidx, ressort);
  }

  const array_type2t &arrtype = to_array_type(index.source_value->type);
  if (!int_encoding && is_bool_type(arrtype.subtype) && no_bools_in_arrays) {
    // Perform a fix for QF_AUFBV, only arrays of bv's are allowed.
    const smt_sort *tmpsort = mk_sort(SMT_SORT_BV, 1, false);
    a = mk_select(src_value, newidx, tmpsort);
    return make_bit_bool(a);
  } else if (is_tuple_array_ast_type(index.source_value->type)) {
    a = convert_ast(src_value);
    return tuple_array_select(a, ressort, newidx);
  } else {
    return mk_select(src_value, newidx, ressort);
  }
}

const smt_ast *
smt_convt::convert_array_store(const expr2tc &expr, const smt_sort *ressort)
{
  const with2t &with = to_with2t(expr);
  expr2tc update_val = with.update_value;
  expr2tc newidx;

  if (is_array_type(with.type) &&
      is_array_type(to_array_type(with.type).subtype) &&
      is_with2t(with.update_value)) {
    newidx = decompose_store_chain(expr, update_val);
  } else {
    newidx = fix_array_idx(with.update_field, with.type);
  }

  expr2tc tmp_idx = newidx->simplify();
  if (!is_nil_expr(tmp_idx))
    newidx = tmp_idx;

  assert(is_array_type(expr->type));
  const array_type2t &arrtype = to_array_type(expr->type);
  if (!int_encoding && is_bool_type(arrtype.subtype) && no_bools_in_arrays){
    typecast2tc cast(get_uint_type(1), update_val);
    return mk_store(with.source_value, newidx, cast, ressort);
  } else if (is_tuple_array_ast_type(with.type)) {
    const smt_sort *sort = convert_sort(with.update_value->type);
    const smt_ast *src, *update;
    src = convert_ast(with.source_value);
    update = convert_ast(update_val);
    return tuple_array_update(src, newidx, update, sort);
  } else {
    // Normal operation
    return mk_store(with.source_value, newidx, update_val, ressort);
  }
}

type2tc
smt_convt::flatten_array_type(const type2tc &type)
{
  unsigned long arrbits = 0;

  type2tc type_rec = type;
  while (is_array_type(type_rec)) {
    arrbits += calculate_array_domain_width(to_array_type(type_rec));
    type_rec = to_array_type(type_rec).subtype;
  }

  // type_rec is now the base type.
  uint64_t arr_size = 1ULL << arrbits;
  constant_int2tc arr_size_expr(index_type2(), BigInt(arr_size));
  return type2tc(new array_type2t(type_rec, arr_size_expr, false));
}

const smt_ast *
smt_convt::mk_select(const expr2tc &array, const expr2tc &idx,
                     const smt_sort *ressort)
{
  assert(ressort->id != SMT_SORT_ARRAY);
  const smt_ast *args[2];
  args[0] = convert_ast(array);
  args[1] = convert_ast(idx);
  return mk_func_app(ressort, SMT_FUNC_SELECT, args, 2);
}

const smt_ast *
smt_convt::mk_store(const expr2tc &array, const expr2tc &idx,
                    const expr2tc &value, const smt_sort *ressort)
{
  const smt_ast *args[3];

  args[0] = convert_ast(array);
  args[1] = convert_ast(idx);
  args[2] = convert_ast(value);
  return mk_func_app(ressort, SMT_FUNC_STORE, args, 3);
}

std::string
smt_convt::get_fixed_point(const unsigned width, std::string value) const
{
  std::string m, f, tmp;
  size_t found, size;
  double v, magnitude, fraction, expoent;

  found = value.find_first_of("/");
  size = value.size();
  m = value.substr(0, found);
  if (found != std::string::npos)
    f = value.substr(found + 1, size);
  else 
		f = "1";

  if (m.compare("0") == 0 && f.compare("0") == 0)
    return "0";

  v = atof(m.c_str()) / atof(f.c_str());

  magnitude = (int)v;
  fraction = v - magnitude;
  tmp = integer2string(power(2, width / 2), 10);
  expoent = atof(tmp.c_str());
  fraction = fraction * expoent;
  fraction = floor(fraction);

  std::string integer_str, fraction_str;
  integer_str = integer2binary(string2integer(double2string(magnitude), 10), width / 2);
	
  fraction_str = integer2binary(string2integer(double2string(fraction), 10), width / 2);

  value = integer_str + fraction_str;

  if (magnitude == 0 && v<0) {
    value = integer2binary(string2integer("-1", 10) - binary2integer(integer_str, true), width)
          + integer2binary(string2integer(double2string(fraction), 10), width / 2);
  }

  return value;
}


const smt_convt::expr_op_convert
smt_convt::smt_convert_table[expr2t::end_expr_id] =  {
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const int
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const fixedbv
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const bool
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const string
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const struct
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const union
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const array
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //const array_of
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //symbol
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //typecast
{ SMT_FUNC_ITE, SMT_FUNC_ITE, SMT_FUNC_ITE, 3, SMT_SORT_ALLINTS | SMT_SORT_BOOL | SMT_SORT_ARRAY},  //if
{ SMT_FUNC_EQ, SMT_FUNC_EQ, SMT_FUNC_EQ, 2, SMT_SORT_ALLINTS | SMT_SORT_BOOL},  //equality
{ SMT_FUNC_NOTEQ, SMT_FUNC_NOTEQ, SMT_FUNC_NOTEQ, 2, SMT_SORT_ALLINTS | SMT_SORT_BOOL},  //notequal
{ SMT_FUNC_LT, SMT_FUNC_BVSLT, SMT_FUNC_BVULT, 2, SMT_SORT_ALLINTS},  //lt
{ SMT_FUNC_GT, SMT_FUNC_BVSGT, SMT_FUNC_BVUGT, 2, SMT_SORT_ALLINTS},  //gt
{ SMT_FUNC_LTE, SMT_FUNC_BVSLTE, SMT_FUNC_BVULTE, 2, SMT_SORT_ALLINTS},  //lte
{ SMT_FUNC_GTE, SMT_FUNC_BVSGTE, SMT_FUNC_BVUGTE, 2, SMT_SORT_ALLINTS},  //gte
{ SMT_FUNC_NOT, SMT_FUNC_NOT, SMT_FUNC_NOT, 1, SMT_SORT_BOOL},  //not
{ SMT_FUNC_AND, SMT_FUNC_AND, SMT_FUNC_AND, 2, SMT_SORT_BOOL},  //and
{ SMT_FUNC_OR, SMT_FUNC_OR, SMT_FUNC_OR, 2, SMT_SORT_BOOL},  //or
{ SMT_FUNC_XOR, SMT_FUNC_XOR, SMT_FUNC_XOR, 2, SMT_SORT_BOOL},  //xor
{ SMT_FUNC_IMPLIES, SMT_FUNC_IMPLIES, SMT_FUNC_IMPLIES, 2, SMT_SORT_BOOL},//impl
{ SMT_FUNC_INVALID, SMT_FUNC_BVAND, SMT_FUNC_BVAND, 2, SMT_SORT_BV},  //bitand
{ SMT_FUNC_INVALID, SMT_FUNC_BVOR, SMT_FUNC_BVOR, 2, SMT_SORT_BV},  //bitor
{ SMT_FUNC_INVALID, SMT_FUNC_BVXOR, SMT_FUNC_BVXOR, 2, SMT_SORT_BV},  //bitxor
{ SMT_FUNC_INVALID, SMT_FUNC_BVNAND, SMT_FUNC_BVNAND, 2, SMT_SORT_BV},//bitnand
{ SMT_FUNC_INVALID, SMT_FUNC_BVNOR, SMT_FUNC_BVNOR, 2, SMT_SORT_BV},  //bitnor
{ SMT_FUNC_INVALID, SMT_FUNC_BVNXOR, SMT_FUNC_BVNXOR, 2, SMT_SORT_BV}, //bitnxor
{ SMT_FUNC_INVALID, SMT_FUNC_BVNOT, SMT_FUNC_BVNOT, 1, SMT_SORT_BV},  //bitnot
  // See comment below about shifts
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0}, // lshl
{ SMT_FUNC_NEG, SMT_FUNC_BVNEG, SMT_FUNC_BVNEG, 1, SMT_SORT_ALLINTS},  //neg
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //abs
{ SMT_FUNC_ADD, SMT_FUNC_BVADD, SMT_FUNC_BVADD, 2, SMT_SORT_ALLINTS},//add
{ SMT_FUNC_SUB, SMT_FUNC_BVSUB, SMT_FUNC_BVSUB, 2, SMT_SORT_ALLINTS},//sub
{ SMT_FUNC_MUL, SMT_FUNC_BVMUL, SMT_FUNC_BVMUL, 2, SMT_SORT_INT | SMT_SORT_REAL },//mul
{ SMT_FUNC_DIV, SMT_FUNC_BVSDIV, SMT_FUNC_BVUDIV, 2, SMT_SORT_INT | SMT_SORT_REAL },//div
{ SMT_FUNC_MOD, SMT_FUNC_BVSMOD, SMT_FUNC_BVUMOD, 2, SMT_SORT_BV | SMT_SORT_INT},//mod
// Error: C frontend doesn't upcast the 2nd operand to shift to the 1st operands
// bit width. Therefore this doesn't work. Fall back to backup method.
//{ SMT_FUNC_INVALID, SMT_FUNC_BVASHR, SMT_FUNC_BVASHR, 2, SMT_SORT_BV},  //ashr
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0}, // shl
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //ashr

{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //dyn_obj_id
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //same_obj_id
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //ptr_offs
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //ptr_obj
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //addr_of
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //byte_extract
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //byte_update
{ SMT_FUNC_STORE, SMT_FUNC_STORE, SMT_FUNC_STORE, 3, 0},  //with
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //member
{ SMT_FUNC_SELECT, SMT_FUNC_SELECT, SMT_FUNC_SELECT, 2, 0},  //index
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //zero_str_id
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //zero_len_str
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //isnan
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //overflow
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //overflow_cast
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //overflow_neg
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //unknown
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //invalid
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //null_obj
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //deref
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //valid_obj
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //deallocated
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //dyn_size
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //sideeffect
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_block
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_assign
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_init
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_decl
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_printf
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_expr
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_return
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_skip
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_free
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_goto
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //obj_desc
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_func_call
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_comma
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //invalid_ptr
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //buffer_sz
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //code_asm
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //cpp_del_arr
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //cpp_del_id
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //cpp_catch
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //cpp_throw
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //cpp_throw_dec
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //isinf
{ SMT_FUNC_INVALID, SMT_FUNC_INVALID, SMT_FUNC_INVALID, 0, 0},  //isnormal
{ SMT_FUNC_HACKS, SMT_FUNC_HACKS, SMT_FUNC_HACKS, 0, 0},  //concat
};

const std::string
smt_convt::smt_func_name_table[expr2t::end_expr_id] =  {
  "hack_func_id",
  "invalid_func_id",
  "int_func_id",
  "bool_func_id",
  "bvint_func_id",
  "real_func_id",
  "symbol_func_id",
  "add",
  "bvadd",
  "sub",
  "bvsub",
  "mul",
  "bvmul",
  "div",
  "bvudiv",
  "bvsdiv",
  "mod",
  "bvsmod",
  "bvumod",
  "shl",
  "bvshl",
  "bvashr",
  "neg",
  "bvneg",
  "bvshlr",
  "bvnot",
  "bvnxor",
  "bvnor",
  "bvnand",
  "bvxor",
  "bvor",
  "bvand",
  "=>",
  "xor",
  "or",
  "and",
  "not",
  "lt",
  "bvslt",
  "bvult",
  "gt",
  "bvsgt",
  "bvugt",
  "lte",
  "bvsle",
  "bvule",
  "gte",
  "bvsge",
  "bvuge",
  "=",
  "distinct",
  "ite",
  "store",
  "select",
  "concat",
  "extract",
  "int2real",
  "real2int",
  "pow",
  "is_int"
};

// Debis from prop_convt: to be reorganized.

expr2tc
smt_convt::get(const expr2tc &expr)
{
  switch (expr->type->type_id) {
  case type2t::bool_id:
    return get_bool(convert_ast(expr));
  case type2t::unsignedbv_id:
  case type2t::signedbv_id:
    return get_bv(expr->type, convert_ast(expr));
  case type2t::fixedbv_id:
  {
    // XXX -- again, another candidate for refactoring.
    expr2tc tmp = get_bv(expr->type, convert_ast(expr));
    const constant_int2t &intval = to_constant_int2t(tmp);
    uint64_t val = intval.constant_value.to_ulong();
    std::stringstream ss;
    ss << val;
    constant_exprt value_expr(migrate_type_back(expr->type));
    value_expr.set_value(get_fixed_point(expr->type->get_width(), ss.str()));
    fixedbvt fbv;
    fbv.from_expr(value_expr);
    return constant_fixedbv2tc(expr->type, fbv);
  }
  case type2t::array_id:
    return get_array(convert_ast(expr), expr->type);
  case type2t::struct_id:
  case type2t::union_id:
  case type2t::pointer_id:
    return tuple_get(expr);
  default:
    std::cerr << "Unimplemented type'd expression (" << expr->type->type_id
              << ") in smt get" << std::endl;
    abort();
  }
}

expr2tc
smt_convt::get_array(const smt_ast *array, const type2tc &t)
{
  // XXX -- printing multidimensional arrays?

  type2tc newtype = flatten_array_type(t);

  const array_type2t &ar = to_array_type(newtype);
  if (is_tuple_ast_type(ar.subtype)) {
    std::cerr << "Tuple array getting not implemented yet, sorry" << std::endl;
    return expr2tc();
  }

  // Fetch the array bounds, if it's huge then assume this is a 1024 element
  // array. Then fetch all elements and formulate a constant_array.
  size_t w = array->sort->get_domain_width();
  if (w > 10)
    w = 10;

  constant_int2tc arr_size(index_type2(), BigInt(1 << w));
  type2tc arr_type = type2tc(new array_type2t(ar.subtype, arr_size, false));
  std::vector<expr2tc> fields;
  const smt_sort *subtype_sort = convert_sort(ar.subtype);

  for (size_t i = 0; i < (1ULL << w); i++) {
    fields.push_back(get_array_elem(array, i, subtype_sort));
  }

  return constant_array2tc(arr_type, fields);
}
