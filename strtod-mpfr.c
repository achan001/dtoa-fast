#include "mapm/dtoa-fast.h"
#include "gmp/mpfr.h"
#include <float.h>

double mpfr_strtod(const char *str, char **end, mpfr_rnd_t round)
{
    static __mpfr_struct z = {._mpfr_prec = 0};
    if (z._mpfr_prec == 0) mpfr_init2(&z, 54);

    if (mpfr_strtofr(&z, str, end, 10, MPFR_RNDZ) == 0)
        return mpfr_get_d(&z, round);

    int bexp = z._mpfr_exp;         // |z| < |str|
    int sign = z._mpfr_sign;
    switch (round) {
        case MPFR_RNDU: if (sign < 0) round = MPFR_RNDZ; break;
        case MPFR_RNDD: if (sign > 0) round = MPFR_RNDZ; break;
        default: break;
    }

    if (!INRANGE(bexp, -1073, 1024)) {
        switch (round) {            // handle edge cases
            case MPFR_RNDZ: return (bexp < 0) ? 0.0 : sign * DBL_MAX;
            case MPFR_RNDN: if (bexp < -1074) return 0.0;
                                    // fall thru to away-from-0
            default: return sign * ((bexp < 0) ? 0x1p-1074 : HUGE_VAL);
        }
    }

    union HexDouble u;              // build double
    u.u = (uint64_t) z._mpfr_d[1] << 32 | z._mpfr_d[0];

    int prec = bexp + 1074;         // bexp = [-1073, 1024]
    if (prec > 53) prec = 53;       // prec = [1, 53]
    u.u >>= 64 - prec - (round == MPFR_RNDN);

    switch (round) {
        case MPFR_RNDZ: break;      // no rounding
        case MPFR_RNDN: u.u = (u.u + 1) >> 1; break;
        default: ++u.u;             // away-from-0
    }
    if (sign < 0) u.u |= 1ull<<63;  // add sign;
    if (bexp > -1021) u.u += (bexp + 1021ull) << 52;
    return u.d;
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
