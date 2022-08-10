#ifndef _ESBMC_SOLVERS_SIYBL_CONV_H_
#define _ESBMC_SOLVERS_SIYBL_CONV_H_

typedef enum AST_TYPE {
    FORALL, EXISTS, AND, OR, NOT, IMPLIES, IFF, // Boolean Logic (0-6)
    SYMBOL, FUNCTION,                           // Symbols and functions calls (7-8)
    REAL_CONSTANT, BOOL_CONSTANT, INT_CONSTANT, STR_CONSTANT, // Constants (9-12)
    PLUS, MINUS, TIMES,                         // LIA/LRA operators (13-15)
    LE, LT, EQUALS,                             // LIA/LRA relations (16-18)
    ITE,                                        // Term-ite (19)
    TOREAL,                                     // LIRA toreal() function (20)
    //
    // MG: FLOOR? INT_MOD_CONGR?
    //
    // BV
    BV_CONSTANT,                                // Bit-Vector constant (21)
    BV_NOT, BV_AND, BV_OR, BV_XOR,              // Logical Operators on Bit (22-25)
    BV_CONCAT,                                  // BV Concatenation (26)
    BV_EXTRACT,                                 // BV sub-vector extraction (27)
    BV_ULT, BV_ULE,                             // Unsigned Comparison (28-29)
    BV_NEG, BV_ADD, BV_SUB,                     // Basic arithmetic (30-32)
    BV_MUL, BV_UDIV, BV_UREM,                   // Division/Multiplication (33-35)
    BV_LSHL, BV_LSHR,                           // Shifts (36-37)
    BV_ROL, BV_ROR,                             // Rotation (38-39)
    BV_ZEXT, BV_SEXT,                           // Extension (40-41)
    BV_SLT, BV_SLE,                             // Signed Comparison (42-43)
    BV_COMP,                                    // Returns 1_1 if the arguments are
                                                // equal otherwise it returns 0_1 (44)
    BV_SDIV, BV_SREM,                           // Signed Division and Reminder(45-46)
    BV_ASHR,                                    // Arithmetic shift right (47)
    //
    // STRINGS
    //
    STR_LENGTH,                                 // Length (48)
    STR_CONCAT,                                 // Concat (49)
    STR_CONTAINS,                               // Contains (50)
    STR_INDEXOF,                                // IndexOf (51)
    STR_REPLACE,                                // Replace (52)
    STR_SUBSTR,                                 // Sub String (53)
    STR_PREFIXOF,                               // Prefix (54)
    STR_SUFFIXOF,                               // Suffix (55)
    STR_TO_INT,                                 // atoi (56)
    INT_TO_STR,                                 // itoa (57)
    STR_CHARAT,                                 // Char at an index (58)
    //
    // ARRAYS
    //
    ARRAY_SELECT,                               // Array Select (59)
    ARRAY_STORE,                                // Array Store (60)
    ARRAY_VALUE,                                // Array Value (61)

    DIV,                                        // Arithmetic Division (62)
    POW,                                        // Arithmetic Power (63)
    ALGEBRAIC_CONSTANT,                         // Algebraic Number (64)
    BV_TONATURAL,
    CONTEXT,

} AST_TYPE;

#include <solvers/smt/smt_conv.h>
#include <solvers/smt/fp/fp_conv.h>

class sibyl_smt_ast : public smt_ast
{
public:
  sibyl_smt_ast(
    smt_convt *ctx,
    AST_TYPE k,
    const smt_sort *_s,
    const messaget &msg)
    : smt_ast(ctx, _s, msg), ast_type(k) {}
  ~sibyl_smt_ast() override = default;

  void dump() const override;

  AST_TYPE ast_type;
  std::string symname;
  BigInt intval;
  std::string realval;
  bool boolval;
  int extract_high;
  int extract_low;
  std::vector<smt_astt> args;
};

class sibyl_convt : public smt_convt, public array_iface, public fp_convt
{
public:
  sibyl_convt(
    const namespacet &ns,
    const optionst &options,
    const messaget &msg);
  ~sibyl_convt() override;

  resultt dec_solve() override;
  const std::string solver_text() override;

  const std::string raw_solver_text() override
  {
    return "sibyl";
  }

  void assert_ast(smt_astt a) override;

  smt_astt mk_add(smt_astt a, smt_astt b) override;
  smt_astt mk_bvadd(smt_astt a, smt_astt b) override;
  smt_astt mk_sub(smt_astt a, smt_astt b) override;
  smt_astt mk_bvsub(smt_astt a, smt_astt b) override;
  smt_astt mk_mul(smt_astt a, smt_astt b) override;
  smt_astt mk_bvmul(smt_astt a, smt_astt b) override;
  smt_astt mk_bvsmod(smt_astt a, smt_astt b) override;
  smt_astt mk_bvumod(smt_astt a, smt_astt b) override;
  smt_astt mk_bvsdiv(smt_astt a, smt_astt b) override;
  smt_astt mk_bvudiv(smt_astt a, smt_astt b) override;
  smt_astt mk_bvshl(smt_astt a, smt_astt b) override;
  smt_astt mk_bvashr(smt_astt a, smt_astt b) override;
  smt_astt mk_bvlshr(smt_astt a, smt_astt b) override;
  smt_astt mk_neg(smt_astt a) override;
  smt_astt mk_bvneg(smt_astt a) override;
  smt_astt mk_bvnot(smt_astt a) override;
  smt_astt mk_bvxor(smt_astt a, smt_astt b) override;
  smt_astt mk_bvor(smt_astt a, smt_astt b) override;
  smt_astt mk_bvand(smt_astt a, smt_astt b) override;
  smt_astt mk_implies(smt_astt a, smt_astt b) override;
  smt_astt mk_xor(smt_astt a, smt_astt b) override;
  smt_astt mk_or(smt_astt a, smt_astt b) override;
  smt_astt mk_and(smt_astt a, smt_astt b) override;
  smt_astt mk_not(smt_astt a) override;
  smt_astt mk_lt(smt_astt a, smt_astt b) override;
  smt_astt mk_bvult(smt_astt a, smt_astt b) override;
  smt_astt mk_bvslt(smt_astt a, smt_astt b) override;
  smt_astt mk_le(smt_astt a, smt_astt b) override;
  smt_astt mk_bvule(smt_astt a, smt_astt b) override;
  smt_astt mk_bvsle(smt_astt a, smt_astt b) override;
  smt_astt mk_eq(smt_astt a, smt_astt b) override;
  smt_astt mk_store(smt_astt a, smt_astt b, smt_astt c) override;
  smt_astt mk_select(smt_astt a, smt_astt b) override;

  smt_sortt mk_bool_sort() override;
  smt_sortt mk_real_sort() override;
  smt_sortt mk_int_sort() override;
  smt_sortt mk_bv_sort(std::size_t width) override;
  smt_sortt mk_array_sort(smt_sortt domain, smt_sortt range) override;
  smt_sortt mk_fbv_sort(std::size_t width) override;
  smt_sortt mk_bvfp_sort(std::size_t ew, std::size_t sw) override;
  smt_sortt mk_bvfp_rm_sort() override;
  smt_sortt mk_fpbv_sort(const unsigned ew, const unsigned sw) override;
  smt_sortt mk_fpbv_rm_sort() override;

  smt_astt mk_smt_int(const BigInt &theint) override;
  smt_astt mk_smt_real(const std::string &str) override;
  smt_astt mk_smt_bool(bool val) override;
  smt_astt mk_smt_symbol(const std::string &name, const smt_sort *s) override;
  smt_astt mk_array_symbol(
    const std::string &name,
    const smt_sort *s,
    smt_sortt array_subtype) override;
  smt_astt mk_extract(smt_astt a, unsigned int high, unsigned int low) override;
  smt_astt mk_sign_ext(smt_astt a, unsigned int topwidth) override;
  smt_astt mk_zero_ext(smt_astt a, unsigned int topwidth) override;
  smt_astt mk_concat(smt_astt a, smt_astt b) override;
  smt_astt mk_ite(smt_astt cond, smt_astt t, smt_astt f) override;

  smt_astt mk_smt_bv(const BigInt &theint, smt_sortt s) override;

  void push_ctx() override;
  void pop_ctx() override;

  bool get_bool(smt_astt a) override;
  BigInt get_bv(smt_astt a, bool is_signed) override;
  ieee_floatt get_fpbv(smt_astt a) override;
  expr2tc get_array_elem(smt_astt array, uint64_t index, const type2tc &subtype)
    override;

  smt_astt
  convert_array_of(smt_astt init_val, unsigned long domain_width) override;

  // void check_msat_error(msat_term &r) const;

  void dump_smt() override;
  void print_model() override;
  void insert_node(unsigned int x);
  unsigned int emit_ast(const sibyl_smt_ast *ast);

  struct symbol_table_rec
  {
    std::string ident;
    unsigned int num;
    smt_sortt sort;
  };

  typedef boost::multi_index_container<
    symbol_table_rec,
    boost::multi_index::indexed_by<
      boost::multi_index::hashed_unique<
        BOOST_MULTI_INDEX_MEMBER(symbol_table_rec, std::string, ident)>,
      boost::multi_index::ordered_unique<
        BOOST_MULTI_INDEX_MEMBER(symbol_table_rec, unsigned int, num),
        std::greater<unsigned int>>>>
    symbol_tablet;

  symbol_tablet symbol_table;

  std::vector<unsigned int> nodes;
  std::vector<unsigned int> outEdges;
  std::vector<unsigned int> inEdges;
  std::vector<unsigned int> edge_attr;
  unsigned int numNodes = 0;

  // Flag to workaround the fact that MathSAT does not support fma. It's
  // set to true so every operation is converted using the fpapi
  bool use_fp_api;
};


#endif /* _ESBMC_SOLVERS_SIBYL_CONV_H_ */