#include "mapm/mapm-dtoa.h"
#include "gmp/gmp.h"

// parse a VALID decimal string so mantissa = s[i:j]
// with at most 1 decimal point inside
static void strtod_parse(const char *s, int *i, int *j, int *nexp)
{
  int k, pt;
  for(k=0; s[k] <= '0'; k++) ;      // skip to 1st digit
  *i = pt = k;

  for(--pt; pt>=0 && s[pt] == '0'; --pt) ;
  if (pt>=0 && s[pt] != '.') pt = -1;

  for(;; k++) {
    if (ISDIGIT(s[k])) continue;
    if (pt>=0 || s[k] != '.') break;
    pt = k;                         // point inside s[i:j]
  }
  *nexp = pt>=0 ? pt-k+1 : 0;       // build exponents
  *j = k++;
  if ((s[*j]|32) == 'e') *nexp += atoi(s + k);
}

// Due to carry, 999... will compared < 1000...
static int strtod_cmp(const char *s, const char *b, int s_bytes)
{
  int i = s[0] - *b++;            // differ due to carry
  if (i) return abs(i)>1 ? -i : i;
  while (++i < s_bytes) {
    if (s[i] == *b) {b++; continue;}
    if (s[i] > '0') return s[i] - *b;
    if (s[i] == '0' && *b) return -1;
  }
  while (*b == '0') b++;          // s side shorter
  return *b ? -1 : 0;             // last non-zeroes
}

// IF i>=0, z = x * 2^i
// ELSE     z = x * 5^-i * 10^i
static void mpz_scale_2exp(mpz_t z, int *nexp, mpz_t x, int i)
{
  if (i >= 0) {mpz_mul_2exp(z, x, i); *nexp=0; return;}
  mpz_t k;
  mpz_init(k);
  mpz_ui_pow_ui(k, 5, -i);
  mpz_mul(z, x, k);
  mpz_clear(k);
  *nexp = i;
}

// convert m * 2^bexp -> int(dec) * 10^nexp
static void strtod_scale(char *dec, int *nexp, uint64_t m, int bexp)
{
  mpz_t b;
  mpz_init(b);
  mpz_import(b, 1, -1, sizeof(m), 0, 0, &m);
  mpz_scale_2exp(b, nexp, b, bexp);
  mpz_get_str(dec, 10, b);
  mpz_clear(b);
}

// Let b = half-way 54 bits estimate
// Let d = decimals from string
// strtod_ulp return sign(|d| - |b|)

static int strtod_ulp(const char *str, unsigned n2, unsigned n1, int bexp)
{
  int i, j, nexp;
  strtod_parse(str, &i, &j, &nexp);

  if (nexp>=0 && bexp <= FASTPATH) return 0;

  char halfway[770];
  int old = fegetround();
  fesetround(FE_TONEAREST);
  uint64_t m = (uint64_t) n2 << 22;
  m += ((n1>>9) + 1) >> 1;      // 54-bit half-way
  strtod_scale(halfway, &nexp, m, bexp-54-BIAS);
  fesetround(old);

  int cmp = strtod_cmp(str + i, halfway, j - i);
  EPRINT("\tcmp = %d\n", cmp);
  return cmp < 0 ? -1 : !!cmp;  // return -1, 0, +1
}
