#include "mapm/common.h"

// SHORTEST string by removing unneeded dec pt, sign, leading 0's
// formatted number INPLACE (need room front + back)
// fmt 'r' -> "31416e-4"    = raw form w/ integer mantissa
// fmt 'e' -> "3.1416e0"    = normalized mantissa
// fmt 'g' -> "3.1416"      = shortest form
// capitalized fmt uses 'E' instead of 'e'

// s points to mantissa digits
// used dtoa-ifmt1.c requirement:
// -> s[-6] .. s[len+6] must be available to use
// -> buf[len + 12], s=buf+6 will cover all cases

char* dtoa_ifmt(char *s, int sgn, int len, int dec, char fmt)
{
  if (ISDIGIT(*s)) {                    // skip Inf, NaN
    int z = 2 + (len > 1) + (dec > 10); // zeroes
    if ((fmt|32)!='g' || dec < -z || dec-len > z) {
      char *p = &s[len];                // exponential form
      if ((fmt|32)=='r') dec -= len;    // pat = d+E-?d{1,3}
      else if (dec--, len>1) {s[-1]=s[0]; *s-- = '.';}
      *p++ = 'E' | (fmt&32);
      if (dec < 0) {*p++ = '-'; dec = -dec;}
      switch((dec>=100) + (dec>=10)) {
        case 2: *p++ = '0' + dec/100; dec %= 100;
        case 1: *p++ = '0' + dec/10; dec %= 10;
       default: *p++ = '0' + dec; *p = '\0';
      }
    } else if (dec <= 0) {              // .0{0,3}d+
      do { *--s = '0'; } while (dec++ < 0);
      s[0] = '.';
    } else if (dec < len) {             // d+.d+
      do {s[len+1] = s[len];} while (dec < len--);
      s[dec] = '.';
    } else {                            // d+0{0,4}
      while(len < dec) s[len++] = '0';
      s[dec] = '\0';
    }
  }
  if (sgn) *--s = '-';                  // add sign
  return s;
}
