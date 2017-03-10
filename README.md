## dtoa-fast		
strtod and dtoa correct rounding ... **FAST** (only require 53 bits precision or more)

Both uses normalized 96-bit float for calculation.		

If binary estimate is **far** from halfway, no need for arbitrary precision math.		

If estimate is **guaranteed** halfway, no need for arbitrary precision math.

Remaing cases uses arbitrary precision math to break ties.		
		
| X | select math libraries to use with -D USE_MAPM=X          |		
|---| ---------------------------------------------------------|
| 0 | use GMP for hard cases (**DEFAULT**)                     |		
| 1 | link with my revised MAPM v5.0 C libary                  |		
| 2 | same as 1, but use *grade-school* multiply only          |		
| 3 | same as 2, but #include as 1 BIG file (just like dtoa.c) |		
			
		
| 96-bit float nice properties    | Eliminated bugs |		
| ----------------------------    | --------------- |		
| no concept of normals/denormals | no *boundary crossing* bug |		
| no need for correction loops    | no *infinite loops* bug    |		
| no need to set precision mode   | no *double rounding* bug   |		
		
| Files           | Comments |		
| -----           | -------- |		
| dtoa-aux.c      | common routine for dtoa-fast.c and dtoa-mode.c              |		
| dtoa-fast.c     | double to string, FE_TONEAREST only (even if it is not)     |		
| dtoa-fast.txt   | explanations / proves of dtoa_fast()                        |		
| dtoa-ifmt.c     | in-place format dtoa_fast() result, mode allowed = [regREG] |		
| dtoa-mode.c     | double to string, honors rounding mode                      |		
| dtoa.c          | David Gay's dtoa.c, to test strtod_fast() / dtoa_fast()     |		
| mapm-src.7z     | much revised MAPM C Library (I name it version 5.0)         |		
| max-ulp.py      | strtod-fast.c accuracy simulation, max ulp <= 25 (96-bits)  |		
| strtod-aux.c    | strtod_ulp() for getting sign of (str - halfway)            |		
| strtod-fast.c   | string to double, FE_TONEAREST only (even if it is not)     |		
| strtod-fast.txt | explanations / proves of strtod_fast()                      |		
| strtod-gmp.c    | strtod_ulp() using GMP (scale only halfway side)            |		
| strtod-mode.c   | string to double, honors rounding mode                      |
| strtod-mpfr.c   | strtod implementation using mpfr, to test strtod_fast()     |
| test.c          | strtod, dtoa, and round-trip tests of my routines           |		
		
