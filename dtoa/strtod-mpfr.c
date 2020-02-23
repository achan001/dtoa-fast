#include "mapm/mapm-dtoa.h"
#include "gmp/mpfr.h"

#ifdef __cplusplus
extern "C" {
#endif

double strtod_mpfr(const char *str, char **end, mpfr_rnd_t round)
{
    static mpfr_t z = {{_mpfr_prec : 0}};
    if (z->_mpfr_prec == 0) mpfr_init2(z, 54);

    if (mpfr_strtofr(z, str, end, 10, MPFR_RNDZ) == 0)
        return mpfr_get_d(z, round);

    int sign = z->_mpfr_sign;
    switch (round) {
        case MPFR_RNDU: if (sign < 0) round = MPFR_RNDZ; break;
        case MPFR_RNDD: if (sign > 0) round = MPFR_RNDZ; break;
        default: break;
    }

    int bexp = z->_mpfr_exp;        // |z| < |str|
    if (!INRANGE(bexp, -1073, 1024)) {
        switch (round) {            // handle edge cases
            case MPFR_RNDZ: return sign * ((bexp < 0) ? 0.0 : DBL_MAX);
            case MPFR_RNDN: if (bexp < -1074) return sign * 0.0;
                                    // fall thru to away-from-0
            default: return sign * ((bexp < 0) ? 0x1p-1074 : HUGE_VAL);
        }
    }

    union HexDouble u;              // build double
    u.u = (uint64_t) z->_mpfr_d[1] << 32 | z->_mpfr_d[0];

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

mpfr_rnd_t mpfr_rnd_mode()          // lookup rounding mode
{
    switch(fegetround()) {
      case FE_TOWARDZERO: return MPFR_RNDZ;
      case FE_TONEAREST: return MPFR_RNDN;
      case FE_DOWNWARD: return MPFR_RNDD;
      default:         return MPFR_RNDU;
    }
}

double strtod(const char *str, char **end)
{
    return strtod_mpfr(str, end, mpfr_rnd_mode());
}

#ifdef __cplusplus
}
#endif
