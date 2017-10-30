#include "mapm/common.h"

// formatted number INPLACE (need room front + back)
// mode 'r' -> "31416e-004"  = raw form w/ integer mantissa
// mode 'e' -> "3.1416e+000" = normalized mantissa
// mode 'g' -> "3.1416"      = shortest form
// capitalized mode uses 'E' instead of 'e'

// s points to mantissa digits
// For mode 'r', s[-sgn]   .. s[len+6] must be available
// For mode 'e', s[-sgn-1] .. s[len+6] must be available
// For mode 'g', s[-sgn-5] .. s[len+6] must be available
// -> s[-6] .. s[len+6] is enough for ALL modes
// -> buf[len + 12], s=buf+6 will cover all cases

char* dtoa_ifmt(char *s, int sgn, int len, int dec, char mode)
{
  if (INRANGE(*s, '1', '9')) {          // skip 0 and inf
    if ((mode|32)!='g' || dec < -3 || dec-len > 5) {
      char *p = &s[len];                // exponential form
      if ((mode|32)=='r') dec -= len;   // pat = d+E[+-]ddd
      else if (dec--, len>1) {s[-1]=s[0]; *s-- = '.';}
      *p++ = 'E'|(mode&32);
      *p++ = dec>=0 ? '+' : (dec=-dec, '-');
      *p++ = '0' + dec/100; dec %= 100;
      *p++ = '0' + dec/10;
      *p++ = '0' + dec%10;
      *p = 0;
    } else if (dec <= 0) {              // 0.0{0,3}d+
      do { *--s = '0'; } while (dec++ <= 0);
      s[1] = '.';
    } else if (dec < len) {             // d+.d+
      do {s[len+1] = s[len];} while (dec < len--);
      s[dec] = '.';
    } else {                            // d+0{0,5}
      while(len < dec) s[len++] = '0';
      s[dec] = 0;
    }
  }
  if (sgn) *--s = '-';                  // add sign
  return s;
}
