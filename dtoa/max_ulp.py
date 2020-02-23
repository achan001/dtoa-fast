# Option    NOTE:
# 'f'       search full range of n = -352 to 308
# 'z'       assumed mantissa <= 28 digits
# 'e'       generate abs_err for each fractional mantissa value
#           Default is 500 points between 0.5 and 1
#           => fractional mantissa = (500 + points) / 1000
# Example:
#
# max_ulp.py                # search for n with max of 8 scales
# max_ulp.py 300            # max ulp if n = 300
# max_ulp.py 10 20          # n = range(10, 20) = 10 to 19

# To generate max_ulp distribution plot
#
# max_ulp.py f > data       # == max_ulp.py -352 309 > data
# gnuplot> plot 'data' w l  # show n vs max_ulp
#
# max_ulp.py fz > data      # same as above but assumed
# gnuplot> plot 'data' w l  # mantissa <= 28 digits
#
# max_ulp.py e > data       # max 8 scalings both branches 1E12,1E25
# gnuplot> plot 'data' every ::::499 w l, '' every ::500 w l
#
# max_ulp.py -319 e > data  # max 95 ULP, VERY sharp peak!
# gnuplot> plot 'data' w l

SHOW_ABS_ERR = 0
import numpy as np

def scale_func(m, term=0, const=0):
    def func((x, err), chop=1):
        frac = x * m
        k = (frac < 0.5) + 1            # top=1, bot=2
        frac *= k                       # normalized
        if term: chop += k * term
        return frac, chop / frac + err
    if not const: return func           # factor <= 96 bits
    return lambda (x,err): func((x, err + const))

s73 = scale_func(1e-73/2**-242, 0.2208, 0.2987)
s60 = scale_func(1e+60/2**+200, 0.0994, 0.2798)
s25 = scale_func(1e+25/2**84, 0.2503)
s12 = scale_func(1e+12/2**40)
si  = np.frexp(10. ** np.arange(1, 13))[0]

def abs_err((x, err), scales, x0_dec_err = None):
    frac = x
    for mi in scales:
        xf = x * mi
        xf *= (xf < 0.5) + 1
        frac = xf if frac is x else np.maximum(frac, xf)
    err = err * frac            # rel err -> abs err
    chop = bool(len(scales))
    if x0_dec_err is not None:  # Add dec_err generated ULP
        err = err + np.floor(x0_dec_err * frac)
    if SHOW_ABS_ERR:
        for i in err: print i + chop
    return np.max(err) + chop

def dec_err(x, m0=1e27/2**90):          # chopped 29+ digits
    return 32 * (2 - (x<m0)) / x        # max = 64/m0 = 79.228

def scale8(x, dec_err=None):            # likely max ULPs range
    if dec_err: dec_err = dec_err(x)    # save for last
    scale6 = s25(s73(s73(s73(s73(s73((x, 0)))))))
    e1 = abs_err(s12(scale6), si,     dec_err)  # n = range(-327, -315)
    e2 = abs_err(s25(scale6), si[:9], dec_err)  # n = range(-314, -305)
    return max(e1, e2)

def max_ulp(x, n=None, dec_err=None):
    if n is None: return scale8(x, dec_err)
    if n > 280: dec_err = None          # no room for dec_err
    if dec_err: dec_err = dec_err(x)    # need final x to calc ULPs
    x = (x, 0)
    while (n < 0)  : n+=73; x = s73(x)
    while (n >= 60): n-=60; x = s60(x)
    while (n >= 25): n-=25; x = s25(x)
    while (n >= 13): n-=12; x = s12(x)
    return abs_err(x, si[n-1:n], dec_err)

if __name__ == '__main__':
    from sys import argv
    start, stop, steps = .5, 1, 500
    x = np.arange(start, stop, (stop-start)/steps)
    n = [None]
    try:
        options = argv[-1].lower()
        if 'e' in options: SHOW_ABS_ERR = 1
        if 'z' in options: dec_err = None
        if 'f' in options: n = xrange(-352, 309); raise
        n = [ int(argv[1]) ]
        n = xrange(int(argv[1]), int(argv[2]))
    except: pass
    for i in n:
        ulp = max_ulp(x, i, dec_err)
        if not SHOW_ABS_ERR:
            print '%s\t%6.3f = %2d ulp' % (i, ulp, ulp)
