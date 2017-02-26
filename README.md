# dtoa-fast
strtod and dtoa correct rounding ... FAST

Both uses normalized 96-bit float for calculation.

If result not too close to halfway, do not
need arbitrary precision math, and it is DONE !
With 96-bit float, that covers 99.9%+ of cases.

If it is very close to halfway, it uses
arbitrary precision math to break ties.
Can select math libraries w/ -D USE_LIB=X

X
0   no library, use David Gay's dtoa.c for hard cases
1   link with MAPM C libary (my revised version)
2   same as 1, but only use high-school multiply (very lean)
3   same as 2, but #include as 1 BIG file (just like dtoa.c)
4   link with GMP for hard cases (DEFAULT)

# dtoa.c:
  dtoa.c need to set rounding mode to 53-bits precision.
  Before calculation, fesetenv(FE_PC53_ENV) maybe needed
  After calculation, restore precision mode maybe needed
  
  It was added to show 96-bit float idea will speed up dtoa.c
  It may simplify dtoa.c code by eliminate correction loop.
  David Gay's have now added 96-bit float idea to dtoa.c
  If old version is needed, add -D NO_BF96
  
  change history in www.ampl.com/netlib/fp/changes
  Latest version in www.ampl.com/netlib/fp/dtoa.c

# MAPM or GMP:
  strtod_fast() uses ONLY integer math
  dtoa_fast() needed ONLY 40 bits accurary for double
  -> NO NEED to set FE_PC53_ENV

  FAST-PATH is added for many half-way cases.
  This remove most of remaining 0.1% cases.
  
# 96-bit float:
  96-bit float idea have certain nice properties:
  no concept of normal/denormals -> no crossing boundary bug
  no need for correction loops   -> no infinite loop bug
  no need to set precision mode  -> no double rounding bug
  (except -D USE_LIB=0, w/ dtoa.c)
  
  Thanks to Rick Regan's articles in http://www.exploringbinary.com/

# FILES
dtoa-aux.c      common routine for dtoa-fast.c and dtoa-mode.c
dtoa-fast.c     double to string, uses FE_TONEAREST only
dtoa-fast.txt   explanations / proves of dtoa_fast()
dtoa-ifmt.c     in-place format dtoa_fast() result, mode allowed = [regREG]
dtoa-mode.c     double to string, honors rounding mode
dtoa.c          96-bit float version of dtoa.c (can disable with -DNO_BF96)
mapm-src.c      much revised MAPM C Library (I name it version 5.0)
max-ulp.py      strtod-fast.c accuracy simulation, max ulp <= 25 (96-bits)
strtod-aux.c    strtod_ulp() for getting sign of (str - halfway)
strtod-fast.c   string to double, uses FE_TONEAREST only
strtod-fast.txt explanations / proves of strtod_fast()
strtod-gmp.c    strtod_ulp() using GMP (scale only halfway side)
strtod-mode.c   string to double, honors rounding mode
test.c          strtod, dtoa, and round-trip tests of my routines

