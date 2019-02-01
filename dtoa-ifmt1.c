#include "mapm/common.h"

// formatted number INPLACE (need room front + back)
// fmt 'E' use printf specs = -?d.d+E[+-]ddd
// fmt 'r' -> "31416e-004"  = raw form w/ integer mantissa
// fmt 'e' -> "3.1416e+000" = normalized mantissa
// fmt 'g' -> "3.1416"      = shortest form
// capitalized fmt uses 'E' instead of 'e'

// s points to mantissa digits
// -> s[-6] .. s[len+5] must be available to use
// -> buf[len + 12], s=buf+6 will cover all cases

#ifdef DTOA_IFMT_G      // 1.0 -> "1.0"
#undef DTOA_IFMT_G
#define DTOA_IFMT_G     2
#else                   // 1.0 -> "1"
#define DTOA_IFMT_G     0
#endif

char* dtoa_ifmt(char *s, int sgn, int len, int dec, char fmt)
{
  if (ISDIGIT(*s)) {                    // skip Inf, NaN
    if ((fmt|32)!='g' || dec < -3 || dec-len > 5 - DTOA_IFMT_G) {
      char *p = &s[len];                // exponential form
      if ((fmt|32)=='r') dec -= len;    // pat = d+E[+-]ddd
      else if (dec--, len>1) {s[-1]=s[0]; *s-- = '.';}
      *p++ = 'E' | (fmt&32);
      *p++ = dec>=0 ? '+' : (dec=-dec, '-');
      *p++ = '0' + dec/100; dec %= 100;
      *p++ = '0' + dec/10;
      *p++ = '0' + dec%10;
      *p = '\0';
    } else if (dec <= 0) {              // 0.0{0,3}d+
      do { *--s = '0'; } while (dec++ <= 0);
      s[1] = '.';
    } else if (dec < len) {             // d+.d+
      do {s[len+1] = s[len];} while (dec < len--);
      s[dec] = '.';
    } else {                            // d+0{0,5} or d+0{0,3}.0
      s[DTOA_IFMT_G ? dec+=2 : dec] = '\0';
      while(len < dec) s[len++] = '0';
      if (DTOA_IFMT_G) s[dec-2] = '.';
    }
  }
  if (sgn) *--s = '-';                  // add sign
  return s;
}