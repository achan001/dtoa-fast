// Convert String to Double Number FAST (Honor Rounding Modes)
#include "dtoa/strtod-aux.c"

static double strtod_cvt (const char *str, uint64_t m, unsigned bot,
                          int bexp, int i, int neg, int round)
{
  unsigned n2 = m >> 32;  // top    32 bits
  unsigned n1 = m;        // middle 32 bits
  unsigned n0 = bot;      // bottom 32 bits

  BIT96("mantissa only");
  for(; i < 0; i += 73) DIV_1E73(n2,n1,n0,bexp);
  for(; i>=60; i -= 60) MUL_1E60(n2,n1,n0,bexp);
  for(; i>=25; i -= 25) MUL_1E25(n2,n1,n0,bexp);
  for(; i>=13; i -= 12) MUL_1E12(n2,n1,n0,bexp);
  MUL32(n2,n1,n0,bexp, t[i], t_bexp[i]);
  BIT96("scaled binary");

  bot = n0;
  if ((unsigned) bexp >= 2046) {  // not normal number !
    if (bexp >= 2046) {
      double bad = round ? HUGE_VAL : DBL_MAX;
      return neg ? -bad : bad;
    }
    i = -bexp;
    bexp = 0;
    uint64_t tail = ((uint64_t) n1 << 32 | n0) >> i;
    n0 = tail |= (uint64_t) n2 << (64-i);
    n1 = tail >> 32;
    n2 = (uint64_t) n2 >> i;
    BIT96("fix subnormal");
  }

  if (bot == 0) {
    switch (round) {
      case 0x400:   // round-nearest 0100 ...
        if ((n1 & 0xfff) == 0x400 && n0 == 0) FIX_0x400(round);
        break;
      case 0x800:   // round-away-0  ?000 ...
        if ((n1 & 0x7ff) == 0x000 && n0 == 0) FIX_0x400(round);
    }
  } else if (bot > HOLE) {
    switch (round) {
      case 0x400:   // round-nearest ?011 ...
        if ((n1 & 0x7ff) == 0x3ff && n0 > HOLE) FIX_0x3ff(round);
        break;
      case 0:       // already reach DBL_MAX
        if (bexp==0x7fd && n2==~0u && n1>=~0x7ffu) break;
      default:      // to/away zero  ?111 ...
        if ((n1 & 0x7ff) == 0x7ff && n0 > HOLE) FIX_0x7ff(round);
    }
  }
  EPRINT("\tround %s\n", ((n1+round)^n1) > 0x7ff ? "up" : "down");

  union HexDouble u;
  u.u = (((uint64_t) n1 + round) >> 11);
  u.u += ((uint64_t) (neg|bexp)<<52) + ((uint64_t) n2<<21);
  return u.d;
}

static double strtod_hex(const char *s, char **e, int neg, int round)
{
  const char *p = s;          // s = xxx[.]xxx[p+dd]
  static char lookup[] =
  ".!\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9!!!!!!!"
  "\xa\xb\xc\xd\xe\xf!!!!!!!!!p!!!!!!!!!!!!!!!!"
  "\xa\xb\xc\xd\xe\xf!!!!!!!!!p";

  uint64_t m = 0;
  int i=0, d=0, bexp=0, pt=-1, sign=1;

  for(; *p=='0'; p++) ;
  int lz = p - s;             // leading zeroes

  while (INRANGE(d = *p++, '.', 'p')) {
    d = lookup[d - '.'];
    if (i++, d < 16) {        // hex digits 0-f
      if (i < 16) m = m*16 + d;
      else if (d) m |= 1;     // to break ties
      continue;
    } else if (i--, d=='.') { // hex pt
      if (pt >= 0) break;     // 2nd hex pt ?
      if ((pt = i) == 0)      // more zeroes ?
        for(; *p=='0'; p++, lz++) bexp -= 4;
      continue;
    } else if (d=='p') {      // build bexp
      const char *q = p;
      if      (*q == '-') q++, sign = -1;
      else if (*q == '+') q++;
      if (ISDIGIT(*q)) {      // valid exponent
        for(d = *q++ - '0'; ISDIGIT(*q); q++)
          if (d < 100000000) d = 10 * d + *q - '0';
        bexp += sign * d;
        p = q + 1;
      }
    }
    break;
  }
  if (e) *e = (char *) (i + lz ? p-1 : s-1);

  if (i == 0) return neg ? -0. : 0.;  // no hex digits
  if (pt < 0) pt = i;                 // add hex point
  if (i > 15) i = 15;                 // max 14 hex + sticky
  lz = clzll(m) - 1;                  // 63 bit mantissa
  bexp += (pt-i)*4 + BIAS + 63 - lz;  // normalized with BIAS

  if (INRANGE(bexp, -53, 2045)) {
    d = 0;
    if (bexp < 0) d=-bexp, bexp=0;    // pos d = denormal
    m <<= lz;
    if (lz < 10 + d) {                // m > 63-10 = 53 bits
      if (round == 0x400) {           // round-to-nearest
        uint64_t halfway = 0x200ULL<<d;
        if ((m & ~0ULL>>(53-d)) > halfway) m += halfway;
      } else if (round == 0x800)      // round-away from 0
        m += ~0ULL>>(54-d);           // round up sticky bit
    }
    union HexDouble u;
    u.u = ((uint64_t) (neg|bexp) << 52) + (m >> (10 + d));
    return u.d;
  }
  double bad;
  if (bexp>0) bad = round ? HUGE_VAL : DBL_MAX;
  else        bad = round!=0x800 ? 0 : 0x1p-1074;
  errno = ERANGE;
  return neg ? -bad : bad;
}

double strtod_fast(const char *str, char **endptr)
{
  const char *p = str;
  int neg=0, lz = 0, round = 0x400; // round-to-nearest

  while (ISSPACE(*p)) p++;          // skip whitespace

  if      (*p == '-') p++, neg = 0x800;
  else if (*p == '+') p++;

  switch(fegetround()) {            // rounding mode
    case FE_DOWNWARD:   round = neg; break;
    case FE_UPWARD:     round = 0x800 - neg; break;
    case FE_TOWARDZERO: round = 0;
  }

  while (*p == '0') p++, lz++;      // skip leading zeroes

  if (lz==1 && (*p|32)=='x')        // handle hex float
    return strtod_hex(p+1, endptr, neg, round);

  uint64_t m=0, bot=0;
  int digits=0, nexp=0, bexp=30;

  for(; ISDIGIT(*p); p++) {         // digits before dec pt
    if (digits >= 19) {
      if (digits == 29) {nexp++; continue;}
      if (digits == 19) KEEP30(m, bot);
    }
    m = 10 * m + (*p - '0');
    digits++;
  }

  if (*p == '.') {                  // digits after dec pt
    int before = lz + digits;       // mark before processing ...
    if (++p, !digits)               // skip leading zeroes again
      for(; *p == '0'; p++) lz++;
    for(; ISDIGIT(*p); p++) {
      if (digits >= 19) {
          if (digits == 29) continue;
          if (digits == 19) KEEP30(m, bot);
      }
      m = 10 * m + (*p - '0');
      digits++;
    }
    nexp -= (lz + digits) - before; // adjust exponent
  }

  if (digits == 0) {                // invalid or zero
    if (endptr) *endptr = (char *) (lz ? p : str);
    return (lz && neg) ? -0.0 : 0.0;
  }

  if ((*p|32) == 'e') {             // build nexp
    const char *q = p+1;
    int d, sign = 1;
    if      (*q == '-') q++, sign = -1;
    else if (*q == '+') q++;
    if (ISDIGIT(*q)) {              // valid exponent
      for(p=q+1,d=*q-'0'; ISDIGIT(*p); p++)
        if (d < 100000000) d = 10 * d + *p - '0';
      nexp += sign * d;
    }
  }

  if (endptr) *endptr = (char *) p; // parsing done

  if (!INRANGE(digits + nexp, EXP_MIN, EXP_MAX)) {
    double bad;
    if (nexp>0) bad = round ? HUGE_VAL : DBL_MAX;
    else        bad = round!=0x800 ? 0 : 0x1p-1074;
    errno = ERANGE;
    return neg ? -bad : bad;
  }

  switch (digits) {
    case 29: bexp = 33;
             bot *= 1250000000; break;
    case 28: bot *= 1000000000; break;
    case 27: bot *= 100000000 ; break;
    case 26: bot *= 10000000  ; break;
    case 25: bot *= 1000000   ; break;
    case 24: bot *= 100000    ; break;
    case 23: bot *= 10000     ; break;
    case 22: bot *= 1000      ; break;
    case 21: bot *= 100       ; break;
    case 20: bot *= 10        ; break;
    default:
      m <<= lz = clzll(m);            // under 64 bits
      bexp = 64 + BIAS - lz;
      goto CVT;
  }
  uint64_t top;
  top = bot + (m >> bexp);
  lz = clz(top>>32);
  bot = m << (64-bexp) >> (32-lz);    // bot 32 bits
  m = (top << lz) | (bot >> 32);      // top 64 bits
  bexp += 64 + BIAS - lz;

CVT:
  EPRINT("%.*s\n", (p-str), str);     // decimal string
  return strtod_cvt(str, m, bot, bexp, nexp, neg, round);
}
