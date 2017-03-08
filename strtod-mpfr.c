#include <fenv.h>
#include <math.h>
#include "gmp/mpfr.h"

double mpfr_atof2(mpfr_t z, mpfr_rnd_t round)
{
    int sign = mpfr_signbit(z);     // assumed |z| < str
    if ( round == MPFR_RNDZ ||      // round-to-0 AGAIN
        (round == MPFR_RNDU && sign) ||
        (round == MPFR_RNDD && !sign))
        return mpfr_get_d(z, MPFR_RNDZ);

    int bexp = mpfr_get_exp(z);
    sign = 1 - 2 * sign;
    if (bexp <= -1074) {
        if (bexp < -1074 && round == MPFR_RNDN) return 0.0;
        return sign * 0x1p-1074;
    }
    if (bexp > 1024) return sign * HUGE_VAL;

    int prec = bexp + 1074;         // bexp = [-1073, 1024]
    if (prec > 53) prec = 53;       // prec = [1, 53]

    if (round != MPFR_RNDN) {       // away-from-0 rounding
        if (prec == 1) return sign * 0x2p-1074;
        mpfr_prec_round(z, prec, MPFR_RNDZ);
    } else if (prec < 53) {         // to-nearest denormals
        mpfr_prec_round(z, prec+1, MPFR_RNDZ);
    }
    mpfr_add_one_ulp(z, 0);
    return mpfr_get_d(z, MPFR_RNDZ);
}

double mpfr_strtod(const char *str, char **end, mpfr_rnd_t round)
{
    mpfr_t z;
    mpfr_init2(z, 54);
    double x = mpfr_strtofr(z, str, end, 10, MPFR_RNDZ)
        ? mpfr_atof2(z, round)      // |z| < str
        : mpfr_get_d(z, round);     // z == str
    mpfr_clear(z);
    return x;
}

double strtod(const char *str, char **end)
{
    switch (fegetround()) {
        case FE_TOWARDZERO: return mpfr_strtod(str, end, MPFR_RNDZ);
        case FE_TONEAREST: return mpfr_strtod(str, end, MPFR_RNDN);
        case FE_DOWNWARD: return mpfr_strtod(str, end, MPFR_RNDD);
        default:         return mpfr_strtod(str, end, MPFR_RNDU);
    }
}
