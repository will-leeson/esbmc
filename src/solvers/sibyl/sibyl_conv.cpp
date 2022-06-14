#include <cassert>
#include <gmp.h>
#include <sibyl_conv.h>
#include <sstream>
#include <string>
#include <util/c_types.h>
#include <util/expr_util.h>

#define new_ast new_solver_ast<sibyl_smt_ast>

// void sibyl_convt::check_msat_error(msat_term &r) const
// {
//   if(MSAT_ERROR_TERM(r))
//   {
//     msg.error("Error creating SMT ");
//     msg.error(fmt::format("Error text: \"{}\"", msat_last_error_message(env)));
//     abort();
//   }
// }

unsigned int sibyl_convt::emit_ast(const sibyl_smt_ast* ast){
    nodes.push_back(ast->ast_type);
    unsigned int nodeNum = numNodes++;
    for(auto arg : ast->args){
        const sibyl_smt_ast *sa = static_cast<const sibyl_smt_ast *>(arg);
        unsigned int argNodeNum = emit_ast(sa);
        outEdges.push_back(nodeNum);
        inEdges.push_back(argNodeNum);
        edge_attr.push_back(0);

        outEdges.push_back(argNodeNum);
        inEdges.push_back(nodeNum);
        edge_attr.push_back(1);
    }

    if(ast->ast_type == AST_TYPE::SYMBOL){
        symbol_tablet::iterator it = symbol_table.find(ast->symname);
        outEdges.push_back(nodeNum);
        inEdges.push_back(it->num);
        edge_attr.push_back(2);
    }

    return nodeNum;
}

smt_convt *create_new_sibyl_solver(
  const optionst &options,
  const namespacet &ns,
  tuple_iface **tuple_api [[gnu::unused]],
  array_iface **array_api,
  fp_convt **fp_api,
  const messaget &msg)
{
  sibyl_convt *conv = new sibyl_convt(ns, options, msg);
  *array_api = static_cast<array_iface *>(conv);
  *fp_api = static_cast<fp_convt *>(conv);
  return conv;
}

sibyl_convt::sibyl_convt(
  const namespacet &ns,
  const optionst &options,
  const messaget &msg)
  : smt_convt(ns, options, msg),
    array_iface(false, false),
    fp_convt(this, msg),
    use_fp_api(false)
{
    nodes.push_back(int(AST_TYPE::AND));
    numNodes++;
}

sibyl_convt::~sibyl_convt()
{
}

void sibyl_convt::push_ctx()
{
  smt_convt::push_ctx();
}

void sibyl_convt::pop_ctx()
{
  smt_convt::pop_ctx();
}

void sibyl_convt::assert_ast(smt_astt a){
    const sibyl_smt_ast *sa = static_cast<const sibyl_smt_ast *>(a);

    int res = emit_ast(sa);
    
    outEdges.push_back(0);
    inEdges.push_back(res);
    edge_attr.push_back(0);
    
    inEdges.push_back(0);
    outEdges.push_back(res);
    edge_attr.push_back(1);
}

smt_convt::resultt sibyl_convt::dec_solve()
{
//   msg.debug("Push Sibyl should not be used to solve");
  return smt_convt::P_ERROR;
}

bool sibyl_convt::get_bool(smt_astt a)
{
  msg.error("Do not use sibyl to retrieve values");
  return true;
}

BigInt sibyl_convt::get_bv(smt_astt a, bool is_signed)
{
  msg.error("Do not use sibyl to retrieve values");
  return 0;
}

ieee_floatt sibyl_convt::get_fpbv(smt_astt a)
{
  const sibyl_smt_ast *mast = to_solver_smt_ast<sibyl_smt_ast>(a);
 //check_msat_error(t);

  // GMP rational value object.
  mpq_t val;
  mpq_init(val);

  mpz_t num;
  mpz_init(num);
  mpz_set(num, mpq_numref(val));
  char buffer[mpz_sizeinbase(num, 10) + 2];
  mpz_get_str(buffer, 10, num);

  size_t ew, sw;

  ieee_floatt number(ieee_float_spect(sw, ew));
  number.unpack(BigInt(buffer));
  msg.error("Do not use sibyl to retrieve values");
  return number;
}

expr2tc sibyl_convt::get_array_elem(
  smt_astt array,
  uint64_t index,
  const type2tc &subtype)
{
  msg.error("Do not use sibyl to retrieve values");
  expr2tc result;

  return result;
}

const std::string sibyl_convt::solver_text()
{
  return "Sibyl";
}

smt_astt sibyl_convt::mk_add(smt_astt a, smt_astt b)
{
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::PLUS, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvadd(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_ADD, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_sub(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::MINUS, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvsub(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_SUB, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_mul(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::TIMES, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvmul(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_MUL, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvsmod(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_SREM, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvumod(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_UREM, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvsdiv(smt_astt a, smt_astt b){
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_SDIV, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvudiv(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_UDIV, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvshl(smt_astt a, smt_astt b){
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_LSHL, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvashr(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_ASHR, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvlshr(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_LSHR, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_neg(smt_astt a){
    smt_astt b = mk_smt_int(-1);
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::TIMES, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvneg(smt_astt a) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_NEG, a->sort, msg);
    ast->args.push_back(a);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvnot(smt_astt a){
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_NOT, a->sort, msg);
    ast->args.push_back(a);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvxor(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_XOR, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvor(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_OR, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvand(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_AND, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_implies(smt_astt a, smt_astt b){
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::IMPLIES, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_xor(smt_astt a, smt_astt b) {
    smt_astt NotA = mk_not(a);
    smt_astt NotB = mk_not(a);

    smt_astt ANotB = mk_and(a, NotB);
    smt_astt NotAB = mk_and(NotA, b);

    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::OR, a->sort, msg);
    ast->args.push_back(ANotB);
    ast->args.push_back(NotAB);
    
    return ast;
}

smt_astt sibyl_convt::mk_or(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::OR, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_and(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::AND, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_not(smt_astt a) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::NOT, a->sort, msg);
    ast->args.push_back(a);
    
    return ast;
}

smt_astt sibyl_convt::mk_lt(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::LT, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvult(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_ULT, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvslt(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_SLT, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_le(smt_astt a, smt_astt b) {
   sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::LE, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvule(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_ULE, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_bvsle(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_SLE, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_eq(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::EQUALS, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_store(smt_astt a, smt_astt b, smt_astt c) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::ARRAY_STORE, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    ast->args.push_back(c);
    
    return ast;
}

smt_astt sibyl_convt::mk_select(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::ARRAY_SELECT, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_smt_int(const BigInt &theint) {
    smt_sortt s = mk_int_sort();
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::INT_CONSTANT, s, msg);
    ast->intval = theint;
    
    return ast;
}

smt_astt sibyl_convt::mk_smt_real(const std::string &str) {
    smt_sortt s = mk_real_sort();
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::REAL_CONSTANT, s, msg);
    ast->realval = str;
    
    return ast;
}

smt_astt sibyl_convt::mk_smt_bv(const BigInt &theint, smt_sortt s){
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_CONSTANT, s, msg);
    ast->intval = theint;
    
    return ast;
}

smt_astt sibyl_convt::mk_smt_bool(bool val) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BOOL_CONSTANT, boolean_sort, msg);
    ast->boolval = val;
    
    return ast;
}


smt_astt sibyl_convt::mk_array_symbol(
const std::string &name,
const smt_sort *s,
smt_sortt array_subtype [[gnu::unused]]) {
    return mk_smt_symbol(name, s);
}


smt_astt sibyl_convt::mk_smt_symbol(const std::string &name, const smt_sort *s) {
    sibyl_smt_ast *a = new sibyl_smt_ast(this, AST_TYPE::SYMBOL, s, msg);
    a->symname = name;

    symbol_tablet::iterator it = symbol_table.find(name);

    if(it != symbol_table.end())
        return a;
    
    struct symbol_table_rec record = {name, numNodes, s};
    symbol_table.insert(record);

    numNodes++;
    nodes.push_back(int(AST_TYPE::CONTEXT));

    if(s->id == SMT_SORT_STRUCT)
        return a;
    
    return a;
}

smt_astt sibyl_convt::mk_extract(smt_astt a, unsigned int high, unsigned int low) {
    smt_sortt s = mk_bv_sort(high - low + 1);
    sibyl_smt_ast *n = new sibyl_smt_ast(this, AST_TYPE::BV_TONATURAL, s, msg);

    n->extract_high = high;
    n->extract_low = low;
    n->args.push_back(a);
    
    return n;
}

smt_astt sibyl_convt::mk_sign_ext(smt_astt a, unsigned int topwidth){
    std::size_t topbit = a->sort->get_data_width();
    smt_astt the_top_bit = mk_extract(a, topbit - 1, topbit - 1);
    smt_astt zero_bit = mk_smt_bv(0, mk_bv_sort(1));
    smt_astt t = mk_eq(the_top_bit, zero_bit);

    smt_astt z = mk_smt_bv(0, mk_bv_sort(topwidth));

    // Calculate the exact value; SMTLIB text parsers don't like taking an
    // over-full integer literal.
    uint64_t big = 0xFFFFFFFFFFFFFFFFULL;
    unsigned int num_topbits = 64 - topwidth;
    big >>= num_topbits;
    smt_astt f = mk_smt_bv(big, mk_bv_sort(topwidth));

    smt_astt topbits = mk_ite(t, z, f);

    return mk_concat(topbits, a);
}

smt_astt sibyl_convt::mk_zero_ext(smt_astt a, unsigned int topwidth){
    smt_astt z = mk_smt_bv(0, mk_bv_sort(topwidth));
    return mk_concat(z, a);
}

smt_astt sibyl_convt::mk_concat(smt_astt a, smt_astt b) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::BV_CONCAT, a->sort, msg);
    ast->args.push_back(a);
    ast->args.push_back(b);
    
    return ast;
}

smt_astt sibyl_convt::mk_ite(smt_astt cond, smt_astt t, smt_astt f) {
    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::ITE, t->sort, msg);
    ast->args.push_back(cond);
    ast->args.push_back(t);
    ast->args.push_back(f);
    
    return ast;
}

smt_astt
sibyl_convt::convert_array_of(smt_astt init_val, unsigned long domain_width){
    smt_sortt dom_sort = mk_int_bv_sort(domain_width);
    smt_sortt arrsort = mk_array_sort(dom_sort, init_val->sort);

    sibyl_smt_ast *ast = new sibyl_smt_ast(this, AST_TYPE::ARRAY_VALUE, arrsort, msg);
    ast->args.push_back(init_val);

    return ast;
}

void sibyl_smt_ast::dump() const
{
  default_message msg;
  // We need to get the env
  auto convt = dynamic_cast<const sibyl_convt *>(context);
  assert(convt != nullptr);

  msg.error("Sibyl doesn't support dumping");
}

void sibyl_convt::dump_smt()
{
  msg.error("Sibyl doesn't support dumping");
}

void sibyl_convt::print_model()
{
  msg.error("Sibyl doesn't support model printing");
}

smt_sortt sibyl_convt::mk_fpbv_sort(const unsigned ew, const unsigned sw) {
    if(use_fp_api)
        return fp_convt::mk_fpbv_sort(ew, sw);

    return new solver_smt_sort<int>(SMT_SORT_FPBV, 0, ew + sw + 1, sw + 1);
}

smt_sortt sibyl_convt::mk_fpbv_rm_sort() {
    if(use_fp_api)
        return mk_bvfp_rm_sort();

    return new solver_smt_sort<int>(SMT_SORT_FPBV_RM, 0, 3);
}

smt_sortt sibyl_convt::mk_bvfp_sort(std::size_t ew, std::size_t sw) {
    return new solver_smt_sort<int>(SMT_SORT_BVFP, 0, ew + sw + 1, sw + 1);
}

smt_sortt sibyl_convt::mk_bvfp_rm_sort() {
    return new solver_smt_sort<int>(SMT_SORT_BVFP_RM, 0, 3);
}

smt_sortt sibyl_convt::mk_bool_sort() {
    return new solver_smt_sort<int>(SMT_SORT_BOOL, 0, 1);
}

smt_sortt sibyl_convt::mk_real_sort() {
    return new solver_smt_sort<int>(SMT_SORT_REAL, 0, 0);
}

smt_sortt sibyl_convt::mk_int_sort() {
    return new solver_smt_sort<int>(SMT_SORT_INT, 0, 0);
}

smt_sortt sibyl_convt::mk_bv_sort(std::size_t width) {
    return new solver_smt_sort<int>(SMT_SORT_BV, 0, width);
}

smt_sortt sibyl_convt::mk_fbv_sort(std::size_t width) {
    return new solver_smt_sort<int>(SMT_SORT_FIXEDBV, 0, width);
}

smt_sortt sibyl_convt::mk_array_sort(smt_sortt domain, smt_sortt range) {
    auto domain_sort = to_solver_smt_sort<int>(domain);
    auto range_sort = to_solver_smt_sort<int>(range);

    return new solver_smt_sort<int>(SMT_SORT_ARRAY, 0, domain->get_data_width(), range);
}