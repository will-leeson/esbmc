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

void sibyl_convt::add_node(AST_TYPE a){
    this->nodes<<int(a)<<",";
}
void sibyl_convt::add_edge(int a, int b, int edge_attr){
    this->edges<<a<<","<<b<<",";
    this->edge_attr<<edge_attr<<",";
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
{}

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
    const sibyl_smt_ast *sast = to_solver_smt_ast<sibyl_smt_ast>(a);
    // this->add_node(AST_TYPE::FORALL);
}

smt_convt::resultt sibyl_convt::dec_solve()
{
//   msg.debug("Sibyl should not be used to solve");
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
  expr2tc result = get_by_ast(subtype, new_ast(0, convert_sort(subtype)));

  return result;
}

const std::string sibyl_convt::solver_text()
{
  return "Sibyl";
}

smt_astt sibyl_convt::mk_add(smt_astt a, smt_astt b)
{
    this->add_node(AST_TYPE::PLUS);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvadd(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_ADD);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_sub(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::MINUS);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvsub(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_SUB);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_mul(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::TIMES);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvmul(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_MUL);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvsmod(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_SREM);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvumod(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_UREM);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvsdiv(smt_astt a, smt_astt b){
    this->add_node(AST_TYPE::BV_SDIV);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvudiv(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_UDIV);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvshl(smt_astt a, smt_astt b){
    this->add_node(AST_TYPE::BV_LSHL);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvashr(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_ASHR);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvlshr(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_LSHR);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_neg(smt_astt a){
    this->add_node(AST_TYPE::TIMES);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(new_ast(this->numNodes, mk_int_sort())); 
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);


    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvneg(smt_astt a) {
    this->add_node(AST_TYPE::BV_NEG);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvnot(smt_astt a){
    this->add_node(AST_TYPE::BV_NOT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvxor(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_XOR);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvor(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_OR);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvand(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_AND);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_implies(smt_astt a, smt_astt b){
    this->add_node(AST_TYPE::IMPLIES);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_xor(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::AND);
    int nodeVal = this->numNodes++;

    this->add_node(AST_TYPE::AND);
    int andVal = this->numNodes++;
    auto andLHS = to_solver_smt_ast<sibyl_smt_ast>(a);
    auto andRHS = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(andVal, andLHS->a, 0);
    this->add_edge(andLHS->a, andVal, 1);
    this->add_edge(andVal, andRHS->a, 0);
    this->add_edge(andRHS->a, andVal, 1);

    this->add_node(AST_TYPE::NOT);
    int notVal = this->numNodes++;
    this->add_edge(notVal, andVal, 0);
    this->add_edge(andVal, notVal, 1);
    
    this->add_node(AST_TYPE::OR);
    int orVal = this->numNodes++;
    auto orLHS = to_solver_smt_ast<sibyl_smt_ast>(a);
    auto orRHS = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(orVal, orLHS->a, 0);
    this->add_edge(orLHS->a, orVal, 1);
    this->add_edge(orVal, orRHS->a, 0);
    this->add_edge(orRHS->a, orVal, 1);

    this->add_edge(nodeVal, notVal, 0);
    this->add_edge(notVal, nodeVal, 1);
    this->add_edge(nodeVal, orVal, 0);
    this->add_edge(orVal, nodeVal, 1);


    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_or(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::OR);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_and(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::AND);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_not(smt_astt a) {
    this->add_node(AST_TYPE::NOT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_lt(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::LT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvult(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_ULT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvslt(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_SLT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_le(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::LE);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvule(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_ULE);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_bvsle(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_SLE);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_eq(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::EQUALS);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_store(smt_astt a, smt_astt b, smt_astt c) {
    this->add_node(AST_TYPE::ARRAY_STORE);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    auto cPrime = to_solver_smt_ast<sibyl_smt_ast>(c);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_select(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::ARRAY_SELECT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_smt_int(const BigInt &theint) {
    this->add_node(AST_TYPE::INT_CONSTANT);

    return new_ast(this->numNodes++, mk_int_sort());
}

smt_astt sibyl_convt::mk_smt_real(const std::string &str) {
    this->add_node(AST_TYPE::REAL_CONSTANT);

    return new_ast(this->numNodes++, mk_real_sort());
}

smt_astt sibyl_convt::mk_smt_bv(const BigInt &theint, smt_sortt s){
    this->add_node(AST_TYPE::BV_CONSTANT);
    
    return new_ast(this->numNodes++, s);
}

smt_astt sibyl_convt::mk_smt_bool(bool val) {
    this->add_node(AST_TYPE::BOOL_CONSTANT);

    return new_ast(this->numNodes++, mk_bool_sort());
}


smt_astt sibyl_convt::mk_array_symbol(
const std::string &name,
const smt_sort *s,
smt_sortt array_subtype) {
    return mk_smt_symbol(name, s);
}


smt_astt sibyl_convt::mk_smt_symbol(const std::string &name, const smt_sort *s) {
    this->add_node(AST_TYPE::SYMBOL);

    return new_ast(this->numNodes++, s);
}

smt_astt sibyl_convt::mk_extract(smt_astt a, unsigned int high, unsigned int low) {
    this->add_node(AST_TYPE::BV_TONATURAL);
    int nodeVal = this->numNodes++;

    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_sign_ext(smt_astt a, unsigned int topwidth){
    this->add_node(AST_TYPE::BV_SEXT);
    int nodeVal = this->numNodes++;

    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_zero_ext(smt_astt a, unsigned int topwidth){
    this->add_node(AST_TYPE::BV_ZEXT);

    int nodeVal = this->numNodes++;

    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_concat(smt_astt a, smt_astt b) {
    this->add_node(AST_TYPE::BV_CONCAT);
    int nodeVal = this->numNodes++;
    
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(a);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    auto bPrime = to_solver_smt_ast<sibyl_smt_ast>(b);
    this->add_edge(nodeVal, bPrime->a, 0);
    this->add_edge(bPrime->a, nodeVal, 1);

    return new_ast(nodeVal, a->sort);
}

smt_astt sibyl_convt::mk_ite(smt_astt cond, smt_astt t, smt_astt f) {
    this->add_node(AST_TYPE::ITE);
    int nodeVal = this->numNodes++;

    auto condPrime = to_solver_smt_ast<sibyl_smt_ast>(cond);    
    this->add_edge(nodeVal, condPrime->a, 0);
    this->add_edge(condPrime->a, nodeVal, 1);

    auto tPrime = to_solver_smt_ast<sibyl_smt_ast>(t);
    this->add_edge(nodeVal, tPrime->a, 0);
    this->add_edge(tPrime->a, nodeVal, 1);

    auto fPrime = to_solver_smt_ast<sibyl_smt_ast>(f);
    this->add_edge(nodeVal, fPrime->a, 0);
    this->add_edge(fPrime->a, nodeVal, 1);

    return new_ast(nodeVal, t->sort);
}

smt_astt
sibyl_convt::convert_array_of(smt_astt init_val, unsigned long domain_width){
    smt_sortt dom_sort = mk_int_bv_sort(domain_width);
    smt_sortt arrsort = mk_array_sort(dom_sort, init_val->sort);

    this->add_node(AST_TYPE::ARRAY_VALUE);
    int nodeVal = this->numNodes++;
    auto aPrime = to_solver_smt_ast<sibyl_smt_ast>(init_val);    
    this->add_edge(nodeVal, aPrime->a, 0);
    this->add_edge(aPrime->a, nodeVal, 1);

    return new_ast(nodeVal, arrsort);
}

sibyl_smt_ast::sibyl_smt_ast(
  smt_convt *ctx,
  int _t,
  const smt_sort *_s,
  const messaget &msg)
  : solver_smt_ast<int>(ctx, _t, _s, msg)
{
  auto convt = dynamic_cast<const sibyl_convt *>(context);
  assert(convt != nullptr);
  // convt->check_msat_error(_t);
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