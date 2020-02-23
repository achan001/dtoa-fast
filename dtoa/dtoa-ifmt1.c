#include "mapm/common.h"

// formatted number INPLACE (need room front + back)
// Non-recognized fmt defaulted to exponential form
// capitalized fmt uses capitalized 'E' exponent

// fmt='n' -> "0.31416e+001" = normalized form, fractional mantissa
// fmt='e' -> "3.1416e+000"  = exponential form, d.ddddddd mantissa
// fmt='r' -> "31416e-004"   = raw form, integer mantissa
// fmt='g' -> "3.1416"       = shortest form

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
  int i = 0;
  if (ISDIGIT(*s)) {                    // skip Inf, NaN
    char c = fmt | 32;                  // fmt lower cased
    if (c != 'g' || dec < -3 || dec-len > 5 - DTOA_IFMT_G) {
      if (c == 'r') dec -= len;         // d+E[+-]ddd
      else if (c == 'n')     {s[i = -2] = '0'; s[-1] = '.';}
      else if (dec--, len>1) {s[i = -1] = s[0]; s[0] = '.';}
      s[len++] = 'E' | (fmt & 32);
      s[len++] = dec>=0 ? '+' : (dec=-dec, '-');
      s[len++] = '0' + dec/100; dec %= 100;
      s[len++] = '0' + dec/10;
      s[len++] = '0' + dec%10;
      s[len++] = '\0';
    } else if (dec <= 0) {              // 0.0{0,3}d+
      do { s[--i] = '0'; } while (dec++ <= 0);
      s[i+1] = '.';
    } else if (dec < len) {             // d+.d+
      do {s[len+1] = s[len];} while (dec < len--);
      s[dec] = '.';
    } else {                            // d+0{0,5} or d+0{0,3}.0
      s[DTOA_IFMT_G ? dec+=2 : dec] = '\0';
      while(len < dec) s[len++] = '0';
      if (DTOA_IFMT_G) s[dec-2] = '.';
    }
  }
  if (sgn) s[--i] = '-';                // add sign
  return s + i;
}
