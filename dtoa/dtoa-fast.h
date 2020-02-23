#ifndef _INCLUDE_DTOAFAST_H_
#define _INCLUDE_DTOAFAST_H_

#if !defined(DTOA_FUNC)
#define DTOA_FUNC extern
#endif

#ifdef __cplusplus
extern "C" {
#endif
DTOA_FUNC double strtod_fast(const char *s, char **e);
DTOA_FUNC char* dtoa_fast(double x, int digits, int *sgn, int *len, int *dec);
DTOA_FUNC char* dtoa_ifmt(char *s, int sgn, int len, int dec, char mode);
#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_DTOAFAST_H_
