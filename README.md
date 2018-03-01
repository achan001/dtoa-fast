## dtoa-fast		
strtod and dtoa correct rounding **FAST** (only require 53 bits precision or more)

Both uses **normalized 96-bits float** for calculations		

If binary estimate is **far** from halfway, no need for arbitrary precision math		

If estimate is **guaranteed** halfway, no need for arbitrary precision math (**FAST-PATH**)

Remaining hard cases uses arbitrary precision math to break ties.		
		
| X | math libraries to use with -D USE_MAPM=X                     |		
|---| -------------------------------------------------------------|
| 0 | use **GMP** for hard cases (**DEFAULT**)                     |		
| 1 | link with my revised **MAPM** v5.0 C libary libmapm.a        |		
| 2 | same as 1, but use **grade-school** multiply only (no FFT)   |		
| 3 | same as 2, but as 1 **BIG file** (no need for libmapm.a)     |		
			
		
| 96-bit float nice properties    | Eliminated bugs |		
| ----------------------------    | --------------- |		
| no concept of normals/denormals | no *boundary crossing* bug |		
| no need for correction loops    | no *infinite loops* bug    |		
| no need to set precision mode   | no *double rounding* bug   |		
		
| Files           | Comments |		
| -----           | -------- |		
| dtoa-aux.c      | common routine for dtoa-fast.c and dtoa-mode.c              |		
| dtoa-fast.c     | double to string, use **FE_TONEAREST** mode                 |		
| dtoa-fast.txt   | algorithm used for dtoa-fast.c and dtoa-mode.c              |
| dtoa-ifmt.c     | in-place format dtoa_fast() result, mode allowed = [regREG] |	
| dtoa-lite.c     | dtoa-fast without bignum library, accurary > 99.95%         |
| dtoa-mode.c     | double to string, honors rounding mode                      |		
| dtoa.c          | David Gay's dtoa.c, to test strtod_fast() / dtoa_fast()     |		
| mapm-src.7z     | much revised MAPM C Library v5.0                            |		
| max_ulp.7z      | simulation shows max ulp error at most 95 ULP               |
| max_ulp.txt     | PROVED max ulp error is at most 95 ULP                      |
| strtod-aux.c    | use MAPM to break ties for hard cases                       |		
| strtod-fast.c   | string to double, use **FE_TONEAREST** mode                 |		
| strtod-fast.txt | algorithm used for strtod-fast.c and strtod-mode.c          |		
| strtod-lite.c   | strtod-fast without bignum library, accuracy almost 100%    |
| strtod-gmp.c    | use GMP to break ties for hard cases                        |
| strtod-mode.c   | string to double, honors rounding mode                      |
| strtod-mpfr.c   | strtod implementation using MPFR                            |
| test.c          | strtod, dtoa, and round-trip tests of my routines           |	
