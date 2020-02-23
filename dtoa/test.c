// Testing my strtod-fast/dtoa-fast code
// -DTESTA test dtoa_fast(x)    == dtoa_gay(x)
// -DTESTD test strtod_fast(s)  == strtod_gay(s)
// -DTESTR test strtod(dtoa(x)) == x
// -DMODE= FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, or FE_TOWARDZERO
//
// Usage examples:
// gcc test.c -DTESTD -std=gnu99 -Id:/cpp -Ic:/ -DUSE_MAPM=1 -lmapm
// gcc test.c -DTESTD -std=gnu99 -Id:/cpp -Ic:/ -DUSE_MAPM=2 -lmapm
// gcc test.c -DTESTA -std=gnu99 -Id:/cpp -Ic:/ -DDIGITS=10 -DUSE_MAPM=3
//            -DMODE=FE_TOWARDZERO
// gcc test.c -DTESTR -std=gnu99 -Id:/cpp -Ic:/ -DDIGITS=17 -lgmp
// gcc test.c -DTESTR -std=gnu99 -Id:/cpp -Ic:/ -DDIGITS=~0
//            -DMODE=FE_UPWARD -DUSE_MAPM -lmapm
//
// # gcc 3.4.2 strtod good enough to round-trip
// gcc test.c -DTESTR -std=gnu99 -DGCC -DMIN_EXP=-307 -DDIGITS=17
//
// For testing other softwares, below does NO TEST
// # 1,000,000 random samples
// gcc test.c -std=gnu99 -DMIN_EXP=0 -DMAX_EXP=0 -DMIN_LEN=10 -DMAX_LEN=20
// # random samples + shortest digits conversion DUMP
// gcc test.c -DTESTR -DDUMP -std=gnu99 -Id:/cpp -Ic:/ -lgmp
//
// With no arguments, test against generated random decimals
// With filenames supplied, compare numbers in file (1 per line)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fenv.h>

#ifndef SAMPLES
#define SAMPLES     1000000
#endif
#ifndef MIN_EXP
#define MIN_EXP     -324
#endif
#ifndef MAX_EXP
#define MAX_EXP     308
#endif
#ifndef MIN_LEN     // MUST be at least 1 digit
#define MIN_LEN     1
#endif
#ifndef MAX_LEN     // samples digits = [MIN_LEN, MAX_LEN]
#define MAX_LEN     100
#endif
#if !DIGITS         // dtoa digits, 0=shortest
#define DIGITS      0
#endif
#ifdef MODE
#define Honor_FLT_ROUNDS
#endif

#if TESTA                   // match digits of dtoa.c vs dtoa-fast.c
  #ifdef MODE
    #include "dtoa/dtoa-mode.c"
  #elif defined(LITE)      // no BIGNUM fall back, but pretty good
    #include "dtoa/strtod-lite.c"
    #include "dtoa/dtoa-lite.c"
  #else
    #include "dtoa/dtoa-fast.c"
  #endif
  #ifndef GCC
  #include "dtoa/dtoa.c"
  #endif

  int test(char *decimal)   // for TESTA
  {
    int sgn, len, dec1, dec2;
    char *f, *g, *e;
    double x = strtod_fast(decimal, NULL);
    f = dtoa_fast(x, DIGITS, &sgn, &len, &dec1);

    #if GCC                 // fast vs gcc ecvt
    g = ecvt(x, DIGITS, &dec2, &sgn);
    for(e=g+DIGITS-1; *e=='0' && e>g; e--) ;
    e++;
    #else
    g = dtoa_gay (x, (DIGITS?2:0), DIGITS, &dec2, &sgn, &e);
    #endif

    if (len == e - g)
      if (dec1 == dec2)
        if (strncmp(f, g, len) == 0) return 0;

    // special cases
    if (f[0] == '0' && g[0] == '0')    return 0;
    if (f[0] == 'I' && strchr(g, 'I')) return 0;

  #ifndef QUIET
    puts(decimal);
    printf("dbl  = %.17g\n", x);
    printf("fast = %s\n", f);
    printf("gay  = %s\n", g);
  #endif
    return 1;
  }

#elif TESTD                 // test strtod_fast(s) == strtod_gay(s)
  #ifdef MODE
    #include "dtoa/strtod-mode.c"
  #elif defined(LITE)       // no BIGNUM fall back, but pretty good
    #include "dtoa/strtod-lite.c"
  #else
    #include "dtoa/strtod-fast.c"
  #endif
  #if GCC                   // strtod vs fast
    #define strtod_gay      strtod
  #elif MPFR                // mpfr vs fast
    #define strtod_gay      strtod
    #include "dtoa/strtod-mpfr.c"
  #else
    #include "dtoa/dtoa.c"  // gay vs fast
  #endif

  int test(char *decimal)   // for TESTD
  {
    double dgay = strtod_gay(decimal, NULL);
    double fast = strtod_fast(decimal, NULL);
    if (dgay == fast) return 0;
  #ifndef QUIET
    puts(decimal);
    printf("\tfast: %.17g\n", fast);
    printf("\tgay : %.17g\n", dgay);
  #endif
    return 1;
  }

#elif TESTR
  #if GCC
    #include <math.h>       // for isinf()
  #elif GAY                 // NOTE: dtoa.c does not support DIGITS < 0
    #include "dtoa/dtoa.c"
    #include "dtoa/dtoa-ifmt2.c"
    #define atof(x) strtod_gay(x, NULL)
  #else
    #ifdef MODE
    #include "dtoa/dtoa-mode.c"
    #elif defined(LITE)    // no BIGNUM fall back, but pretty good
    #include "dtoa/strtod-lite.c"
    #include "dtoa/dtoa-lite.c"
    #else
    #include "dtoa/dtoa-fast.c"
    #endif
    #include "dtoa/dtoa-ifmt2.c"
    #define atof(x) strtod_fast(x,NULL)
  #endif

  int test(char *decimal)   // for TESTR
  {
    double x = atof(decimal);
    if (isinf(x)) return 0; // skip inf

  #if GCC                   // gcc 17 digits round-trip!
    char cvt[40];
    sprintf(cvt, "%.*e", DIGITS-1, x);
  #elif GAY                 // DIGITS > 17 is OK
    #define GMODE   (DIGITS ? 2 : 0)
    int sgn, dec;
    char buf[DIGITS+32], *e, *cvt=buf+6;
    dtoa_r(x, GMODE, DIGITS, &dec, &sgn, &e, cvt, sizeof(buf)-6);
    cvt = dtoa_ifmt(cvt, sgn, e-cvt, dec, 'r');
  #else
    int sgn, len, dec;      // DIIGTS = [~17, 17]
    char *cvt = dtoa_fast(x, DIGITS, &sgn, &len, &dec);
    cvt = dtoa_ifmt(cvt, sgn, len, dec, 'r');
  #endif

  #if DUMP                  // dump decimal and cvt only
    return !printf("%s\t%s\n", decimal, cvt);
  #endif

    double y = atof(cvt);   // round-trip test
    if (x == y) return 0;

  #ifndef QUIET
    puts(decimal);
    printf("dtoa = %s\n", cvt);
    printf("dbl  = %.17g\n", x);
    printf("trip = %.17g\n", y);
  #endif
    return 1;
  }

#else
  #warning TESTA, TESTD, or TESTR not defined
  #warning Default to data dump (no test)
  #define test      puts
#endif

unsigned seed = 1;  // "Standard C Library", p359
#define NEXT(x)     (x = x*1103515245+12345)
#define RAND(m)     ((NEXT(seed)>>16) % (m))

void randDecimal(char* decimal, int digits)
{
  int i=0, exp = MIN_EXP;
  decimal[i++] = RAND(9) + '1';   // normalized
  decimal[i++] = '.';
  while (i <= digits) decimal[i++] = RAND(10) + '0';
  #if MAX_EXP > MIN_EXP
  exp += RAND((MAX_EXP)-(MIN_EXP)+1);
  #endif
  decimal[i++] = 'e';             // exp = [MIN_EXP, MAX_EXP)
  if (exp< 0) decimal[i++] = '-', exp=-exp;
  decimal[i++] = exp/100+'0', exp %= 100;
  decimal[i++] = exp/10 +'0';
  decimal[i++] = exp%10 +'0';
  decimal[i]   = 0;
}

void randomTest(char *decimal)
{
  #ifndef SEED
  #define SEED time(NULL)
  #endif
  seed = SEED;

  int bad = 0;
  for(int i = SAMPLES; i; i--) {
    #if MAX_LEN > MIN_LEN
    randDecimal(decimal, MIN_LEN + RAND((MAX_LEN)-(MIN_LEN)+1));
    #else
    randDecimal(decimal, MIN_LEN);
    #endif
    bad += test(decimal);
  }
  fprintf(stderr, "<<< random >>>\tbad = %d/%d\n", bad, SAMPLES);
}

int main (int argc, char **argv)
{
  // 64 bits: 1e16 + 2.9999 -> 10000000000000004
  // 53 bits: 1e16 + 2.9999 -> 10000000000000002
  fesetenv (FE_PC53_ENV);       // setup dtoa.c
#ifdef MODE
  fesetround(MODE);             // rounding mode
#endif
  char line[8192];
  if (argc == 1) {randomTest(line); return 0;}

  for (int i=1; i<argc; i++) {  // read from files
    FILE *f = fopen(argv[i], "rb");
    if (f == NULL) continue;    // skip non-files
    int bad = 0, total = 0;
    for(; fgets(line, sizeof(line), f); total++) {
      bad += test(line);
    }
    fprintf(stderr, "%s\t\t bad = %d/%d\n", argv[i], bad, total);
    fclose(f);
  }
  return 0;
}
