/*******************************************************************\

Module: Builtin Models

Author: Rafael Menezes

Description: Models for custom functions that are easier to implement
 and test here

\*******************************************************************/

int __ESBMC_sync_fetch_and_add(int *ptr, int value)
{
  __ESBMC_atomic_begin();
  int initial = *ptr;
  *ptr += value;
  __ESBMC_atomic_end();
  return initial;
}

#if __ESBMC_CHERI__ == 128

// #include <stdint.h>
// #include <stddef.h>
// #include <cheri.h>

#if 1

#if !defined(cheri_debug_assert)
/* Disable cheri-compressed-cap's debug assertions since they assert that
 * base <= top in compute_base_top, which the comment above it admits is not
 * always true. */
#if 1
#define cheri_debug_assert(...)
#else
#define cheri_debug_assert(...)                                                \
  __ESBMC_assert(__VA_ARGS__, "cheri-compressed-cap internal assertion")
#endif
#endif
#include <cheri_compressed_cap.h>

#include <cheri/cheric.h>
// #include <assert.h>

/* modelled after UCAM-CL-TR-951 semantics of CHERI-MIPS instruction CSetBounds */
void *__capability
__esbmc_cheri_bounds_set(void *__capability cap, __SIZE_TYPE__ sz)
{
  __PTRADDR_TYPE__ cursor = (__PTRADDR_TYPE__)cap;
  __PTRADDR_TYPE__ base = cheri_getbase(cap);
  __PTRADDR_TYPE__ top = cheri_gettop(cap);
  __ESBMC_assert(cheri_gettag(cap), "tag-violation c2exception");
  __ESBMC_assert(!cheri_getsealed(cap), "seal-violation c2exception");
  __ESBMC_assert(base <= cursor, "length-violation c2exception");
  __uint128_t newTop = cursor;
  newTop += sz;
  __ESBMC_assert(newTop <= top, "length-violation c2exception");
  cc128_cap_t comp;
  bool exact = cc128_setbounds(&comp, cursor, newTop);
  (void)exact; /* ignore */
  union
  {
    struct
    {
      __UINT64_TYPE__ pesbt;
      __UINT64_TYPE__ cursor;
    };
    void *__capability cap;
  } ret;
  __ESBMC_assume(ret.cursor == cursor);
  __ESBMC_assume(ret.pesbt == comp.cr_pesbt);
  __ESBMC_assume(cheri_gettag(ret.cap) == cheri_gettag(cap));
  __ESBMC_assume(cheri_gettype(ret.cap) == cheri_gettype(cap));
  __ESBMC_assume(cheri_getperm(ret.cap) == cheri_getperm(cap));
  __ESBMC_assume(cheri_getflags(ret.cap) == cheri_getflags(cap));
  return ret.cap;
}

#endif

#if 0
#define cheri_ptr(ptr, len)                                                    \
  cheri_bounds_set(                                                            \
    (union __esbmc_cheri_cap128){.cursor = (uintptr_t)(ptr)}.cap, len)
#endif

__SIZE_TYPE__ __esbmc_cheri_length_get(void *__capability cap)
{
#if 1
  union
  {
    void *__capability cap;
    struct
    {
      __UINT64_TYPE__ pesbt;
      __UINT64_TYPE__ cursor;
    };
  } u = {cap};
  cc128_cap_t result;
  cc128_decompress_mem(u.pesbt, u.cursor, true /* TODO: tag bit */, &result);
  return result.cr_bounds_valid ? result._cr_top - result.cr_base : 0;
#else
  union u
  {
    void *__capability cap;
    struct
    {
      __UINT64_TYPE__ pesbt;
      __UINT64_TYPE__ cursor;
    };
    struct
    {
#if __BYTE_ORDER == __LITTLE_ENDIAN
      __UINT64_TYPE__ B_E : 3;
      __UINT64_TYPE__ B_13_3 : 11;
      __UINT64_TYPE__ T_E : 3;
      __UINT64_TYPE__ T_11_3 : 9;
      __UINT64_TYPE__ I_E : 1;
      __UINT64_TYPE__ otype : 18;
      __UINT64_TYPE__ /* reserved */ : 3;
      __UINT64_TYPE__ perms : 16;
#elif __BYTE_ORDER == __BIG_ENDIAN
      __UINT64_TYPE__ perms : 16;
      __UINT64_TYPE__ /* reserved */ : 3;
      __UINT64_TYPE__ otype : 18;
      __UINT64_TYPE__ I_E : 1;
      __UINT64_TYPE__ T_11_3 : 9;
      __UINT64_TYPE__ T_E : 3;
      __UINT64_TYPE__ B_13_3 : 11;
      __UINT64_TYPE__ B_E : 3;
#else
#error neither big nor little endian, no CHERI-C support for this architecture
#endif
    };
  } u = {cap};
  u.pesbt ^=
    (union u){
      .B_E = 52 & 7,
      .T_E = 52 >> 3,
      .I_E = 1,
      .otype = -1,
    } /* encoding of the NULL capability, little endian: 0x1ffffc018004 */
      .pesbt;
  __UINT64_TYPE__ E;
  __UINT64_TYPE__ T = u.T_11_3 << 3;
  __UINT64_TYPE__ B = u.B_13_3 << 3;
  int L_carry_out;
  int L_msb;
  if(u.I_E)
  {
    E = u.T_E << 3 | u.B_E;
    L_carry_out = (T & 0x7f8) < (B & 0x7f8);
    L_msb = 1;
  }
  else
  {
    E = 0;
    T |= u.T_E;
    B |= u.B_E;
    L_carry_out = (T & 0x7ff) < (B & 0x7ff);
    L_msb = 0;
  }
  if(E > 52) /* 52 is the max. exponent for this format on non-morello */
    E = 52;
  T |= (((B >> 12) + L_carry_out + L_msb) & 3) << 12;
  __UINT64_TYPE__ a_top = E + 14 < 64 ? u.cursor >> (E + 14) : 0;
  __UINT64_TYPE__ one = 1;
  __UINT64_TYPE__ a_mid = (u.cursor >> E) & ((one << 14) - 1);
  __UINT64_TYPE__ a_low = u.cursor & ((one << E) - 1);
  __UINT64_TYPE__ A3 = (u.cursor >> 11) & 0x7;
  __UINT64_TYPE__ B3 = (B >> 11) & 0x7;
  __UINT64_TYPE__ T3 = (T >> 11) & 0x7;
  __UINT64_TYPE__ R = B3 - 1;
  int c_t = (T3 < R) - (A3 < R);
  int c_b = (B3 < R) - (A3 < R);
  __UINT64_TYPE__ t_lo =
    ((__UINT64_TYPE__)((__INT64_TYPE__)a_top + c_t) << 14 | T) << E;
  __UINT64_TYPE__ t_hi;
  if(14 + E <= 64)
    t_hi = (__UINT64_TYPE__)((__INT64_TYPE__)a_top + c_t) >> (64 - (14 + E));
  else
    t_hi = (__UINT64_TYPE__)((__INT64_TYPE__)a_top + c_t) << (14 + E - 64) |
           (E ? T >> (64 - E) : 0);
  __UINT64_TYPE__ b = (E + 14 < 64 ? (a_top + c_b) << (E + 14) : 0) | B << E;
  if(t_hi > 1)
    t_hi = 1;
  if(E < 51 && (int)((t_hi << 1 | t_lo >> 63) & 3) - (int)(b >> 63) > 1)
    t_hi ^= 1;
  return t_lo - b;
#endif
}

#endif
