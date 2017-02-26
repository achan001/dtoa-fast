#if USE_LIB > 3
#include "dtoa/strtod-gmp.c"
#else

#if USE_LIB == 1
#include "mapm/m_apm.h"     // just use libmapm.a
#else
#include "mapm/mapm_mul.c"  // high school multiply only
#endif

#if USE_LIB == 3            // 1 big strtod-fast.c
  #include "mapm/mapm_util.c"
  #include "mapm/mapm_utl2.c"
  #include "mapm/mapm_cnst.c"
  #include "mapm/mapm_stck.c"
  #include "mapm/mapm_set.c"
  #include "mapm/mapm_pwr2.c"
#endif

// Let b = half-way 54 bits estimate
// Let d = decimals from string
// strtod_ulp return sign(|d| - |b|)

static int strtod_ulp(const char *str, unsigned n2, unsigned n1, int bexp)
{
  M_APM d = M_get_stack_var();  // d = decimals from string
  m_apm_set_string(d, str);

  if (d->m_apm_exponent >= d->m_apm_datalength && bexp <= 91+BIAS) {
    M_restore_stack(1);         // FAST-PATH, d <= 0x1p91
    return 0;                   // |d-b| < 1, thus d = b
  }

  int old = fegetround();
  fesetround(FE_TONEAREST);
  M_APM b = M_get_stack_var();
  uint64_t m = (uint64_t) n2 << 22;
  m += ((n1>>9) + 1) >> 1;      // 54-bit half-way
  m_apm_set_uint64_t(b, m);
  m_apm_ishift(b, bexp-54-BIAS);
  fesetround(old);

  int sign = m_apm_compare_absolute(d, b);
  EPRINT("\tsign = %+d\n", sign);
  M_restore_stack(2);
  return sign;
}
#endif
