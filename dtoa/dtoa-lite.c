#include "dtoa/dtoa-fast.h"
#include "mapm/mapm-dtoa.h"

struct dtoa_case {int head, len, dec; char str[LAST+9];};
static struct dtoa_case safe;

static int dtoa_safe(int64_t hi, int gap, int dec)
{
  EPRINT("safe = %I64d to %I64d\n", hi - gap, hi);
  
  // hi = m2 1E9 + m1, m1 = [0, 1E9)
  int m2 = lrint(1E-9 * (hi - (int64_t) 5E8));
  int m1 = hi - (int64_t) 1E9 * m2;
  if ((unsigned) m1 >= (unsigned) 1E9)
    m1 += (m1 < 0) ? (m2--, (int) 1E9) : (m2++, (int) -1E9);

  int i = LAST, j = LAST;
  memset(safe.str, '0', LAST+1);    // build hi digits
  for(;;) {
    for(; m1 > 9; m1 /= 10U) safe.str[i--] += m1 % 10U;
    safe.str[i--] += m1;
    if (m2 == 0) break;
    m1 = m2; m2 = 0; i = LAST-9;    // build billions
  }

  safe.str[0] = safe.str[j] - gap;  // low last digit
  if (safe.str[0] < '1')            // hi too long
    while(safe.str[--j] == '0') ;

  safe.dec = dec - (safe.head = i);
  safe.len = j - i;
  return (safe.str[0] < '1');       // hi shortened
}

static struct dtoa_case *dtoa_boundary
(int64_t m, double frac, int dec, double ulp)
{
  if (ulp >= 20./3) {
    int64_t m0 = m;
    ulp *= 0.1; dec++;
    frac = (frac + (m0 - 10 * (m/=10U))) * 0.1;
  }
  else if (ulp < 2./3) {
    int d = (int) (frac *= 10);
    m = m * 10 + d;
    ulp *= 10; dec--;
    frac -= d;
  }
  int i = lrint(frac - (ulp * 0.5 - 0.5));
  int j = lrint(frac + (ulp - 0.5));

  if (!dtoa_safe(m + j, j - i, dec))
    if (i != j)
      safe.str[LAST] -= j - (frac > 0.5);
  return &safe;
}

static struct dtoa_case *dtoa_shortest
(int64_t m, double frac, int dec, double ulp, double x)
{
  if (ulp >= 5) {           // ulp < 5 imply j-i < 10
    uint64_t m0 = m, scale = 1;
    do {scale *= 10; dec++;} while ((ulp *= 0.1) >= 5);
    m /= scale;
    frac = (frac + (m0 - m * scale)) / scale;
  }
  else if (ulp < 0.5) {     // ulp >= 0.5 imply j >= i
    int d = (int) (frac *= 10);
    m = m * 10 + d;
    ulp *= 10; dec--;
    frac -= d;
  }
  int i = lrint(frac - (ulp - 0.5));
  int j = lrint(frac + (ulp - 0.5));
  EPRINT("\nmantissa => %I64u + %.17g", m, frac);
  EPRINT("\nsafe ulp => %.17g\n", ulp);
  
  if (dtoa_safe(m + j, j - i, dec)) return &safe;
  
  // skip edge cases -> possibly extra digits
  
  safe.str[LAST] -= j - (frac > 0.5);  
  if (fabs(frac - 0.5) < ERR)
    if (safe.str[LAST] & 1)     // assumed half-way
      ++safe.str[LAST];         // round-to-even
  return &safe;
}

static struct dtoa_case *dtoa_do_digit
(int64_t m, double frac, int dec, int digits)
{
  if (digits > 17) digits = 17;   // limit max digits
  safe.head = 0;
  safe.len = digits;

  int len=17;
  if (m < (int64_t) 1E16) len--;  // len of m digits
  dec += (len -= digits);         // scale m to digits

  if (len > 0) {                  // scale down mantissa
    if (frac > 0.5) {m++; frac--;}
    uint64_t scale = 1, m0 = m;
    for(; len>=2; len-=2) scale *= 100;
    if (len) scale *= 10;
    m /= scale;
    m0 -= m * scale;
    scale >>= 1;                  // check half-way
    if      (m0 < scale) ;
    else if (m0 > scale) m++;
    else if (fabs(frac) < ERR)    // assumed half-way
      m += (m & 1);               // round-to-even
    else if (frac > 0) m++;       // frac to break tie
  } else {
    if (len) {m*=10; frac*=10;}   // scale up 10x
    int d = lrint(frac);
    if ((m += d) & 1)             // odd m
      if (fabs(frac-d) > 0.5-ERR) // assume half-way
        m += (frac > d) ? 1 : -1; // round-to-even
  }
  dtoa_safe(m, 0, dec);
  return &safe;
}

char *dtoa_fast(double x, int digits, int *psgn, int *plen, int *pdec)
{
  union HexDouble u = {d:x};
  int i=0;
  int bexp = u.u >> 52;
  u.u &= ~0ULL >> 12;       // mantissa part only
  *psgn = bexp >> 11;       // save sign
  int pow2 = (u.u == 0);    // x is power of 2

  switch (bexp &= 0x7ff) {  // remove sign
    case 0:
      if (pow2) {           // only a zero
        *plen = *pdec = 1;
        return strcpy(safe.str, "0");
      }
      i = clzll(u.u) - 11;  // subnormal number
      u.u <<= i;            // treat it as normal
      bexp = -i;
      break;
    case 0x7ff:
      *plen = *pdec = 9999; // bad number
      return strcpy(safe.str, u.u ? "NaN" : "Inf");
    default:                // normal number
      u.u |= 1ULL<<52;      // add imply bits
      bexp--;
  }
  uint64_t m0 = u.u << 11;
  u.u += (uint64_t)(BIAS+54-i) << 52;

  unsigned n2 = m0 >> 32;   // top    32 bits
  unsigned n1 = m0;         // middle 32 bits
  unsigned n0 = 0;          // bottom 32 bits
  int dec = LAST;           // powers of 10

  BIT96("rounded double");
#ifdef FAST_BF96
  i = BIAS + 56 - bexp;     // scale away 2^i, i=[-968, 1129]
  i = i * 315653 >> 20;     // 315653/2^20 = 0.301030 ~= log2
  dec -= i;
  MUL_96(n2,n1,n0,bexp, i);
#else
  for(; bexp >BIAS+56    ; dec+=73) DIV_1E73(n2,n1,n0,bexp);
  for(; bexp<=BIAS+56-200; dec-=60) MUL_1E60(n2,n1,n0,bexp);
  for(; bexp<=BIAS+56- 84; dec-=25) MUL_1E25(n2,n1,n0,bexp);
  for(; bexp<=BIAS+56- 44; dec-=12) MUL_1E12(n2,n1,n0,bexp);
  i = BIAS + 56 - bexp;     // scale away 2^i, i=[0, 12]
  dec -= i = i * 77 >> 8;   // 77/2^8 = 0.3008 ~= log2
  MUL_32(n2,n1,n0,bexp, i);
#endif
  BIT96("mantissa only");

  uint64_t m = ((uint64_t) n2<<32 | n1) >> (BIAS+64-bexp);
  double ulp = (1-2*ERR) / u.d * m;

  u.d = 1;                  // mantissa fractions after 1
  u.u |= ((uint64_t) n1<<32 | n0) << (bexp-BIAS-32) >> 12;
  if (digits < 0) digits=~digits;

  struct dtoa_case *best;
  if (digits)    best = dtoa_do_digit(m, u.d-1, dec, digits);
  else if (pow2) best = dtoa_boundary(m, u.d-1, dec, ulp);
  else           best = dtoa_shortest(m, u.d-1, dec, ulp, x);

  char *s = best->str + best->head + 1;
  *pdec = best->dec;
  *plen = best->len;
  s[*plen] = '\0';
  EPRINT("best = %s\n", s);
  return s;
}
