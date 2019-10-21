#include "mapm/common.h"

// formatted number INPLACE (need room front + back)
// SHORTEST string by removed unneeded characters
// Non-recognized fmt defaulted to exponential form
// capitalized fmt uses capitalized 'E' exponent

// fmt='n' -> ".31416e1"    = normalized form, fractional mantissa
// fmt='e' -> "3.1416e0"    = exponential form, d.ddddddd mantissa
// fmt='r' -> "31416e-4"    = raw form, integer mantissa
// fmt='g' -> "3.1416"      = shortest form

// s points to mantissa digits
// -> s[-6] .. s[len+5] must be available to use
// -> buf[len + 12], s=buf+6 will cover all cases

#ifdef DTOA_IFMT_G      // 1.0 -> "1."
#undef DTOA_IFMT_G
#define DTOA_IFMT_G     1
#else                   // 1.0 -> "1"
#define DTOA_IFMT_G     0
#endif

char* dtoa_ifmt(char *s, int sgn, int len, int dec, char fmt)
{
  int i = 0;
  if (ISDIGIT(*s)) {                    // skip Inf, NaN
    int z = 2 + (len > 1) + (dec > 10); // zeroes
    char c = fmt | 32;                  // fmt lower cased
    if (c != 'g' || dec < -z || dec-len > z - DTOA_IFMT_G) {
      if (c == 'r') dec -= len;         // d+E-?d{1,3}
      else if (c == 'n')     {s[i = -1] = '.';}
      else if (dec--, len>1) {s[i = -1] = s[0]; s[0] = '.';}
      s[len++] = 'E' | (fmt & 32);
      itoa(dec, &s[len], 10);
    } else if (dec <= 0) {              // .0{0,3}d+
      do { s[--i] = '0'; } while (dec++ < 0);
      s[i] = '.';
    } else if (dec < len) {             // d+.d+
      do {s[len+1] = s[len];} while (dec < len--);
      s[dec] = '.';
    } else {                            // d+0{0,4} or d+0{0,3}.
      s[DTOA_IFMT_G ? dec+=1 : dec] = '\0';
      while(len < dec) s[len++] = '0';
      if (DTOA_IFMT_G) s[dec-1] = '.';
    }
  }
  if (sgn) s[--i] = '-';                // add sign
  return s + i;
}
