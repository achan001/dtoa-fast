import numpy as np

def scale_func(m, term=0, const=0):
    def func((x, err), chop=1):
        frac = x * m
        k = (frac < 0.5) + 1    # top=1, bot=2
        frac *= k               # normalized
        if term: chop += k * term
        return frac, chop / frac + err
    if not const: return func   # factor <= 96 bits
    return lambda (x,err): func((x, err + const))

s73 = scale_func(1e-73/2**-242, 0.2208, 0.2987)
s60 = scale_func(1e+60/2**+200, 0.0994, 0.2798)
s25 = scale_func(1e+25/2**84, 0.2503)
s12 = scale_func(1e+12/2**40)
si  = np.frexp(10. ** np.arange(1, 13))[0]

def abs_err((x, err), scales=()):
    frac = x
    for mi in scales:
        xi = x * mi
        xi *= (xi < 0.5) + 1
        frac = xi if frac is x else np.maximum(frac, xi)
    err = err * frac
    # for i in err: print i
    return np.max(err)

def df_err(x, m0=1e28/2**94):   # chopped 30+ digits
    return (4 - 2*(x<m0)) / x

def scale8(x, df_err=df_err):
    scale6 = s25(s73(s73(s73(s73(s73((x, df_err(x))))))))
    e1 = abs_err(s12(scale6), si)       # n = [-327, -316]
    e2 = abs_err(s25(scale6), si[:9])   # n = [-314, -306]
    return max(e1, e2) + 1

def max_ulp(x, n=None, df_err=df_err):
    if n is None: return scale8(x, df_err)
    y = x, (0 if n>280 else df_err(x))
    while (n < 0)  : n+=73; y = s73(y)
    while (n >= 60): n-=60; y = s60(y)
    while (n >= 25): n-=25; y = s25(y)
    while (n >= 13): n-=12; y = s12(y)
    if n==0: return abs_err(y)
    return abs_err(y, [si[n-1]]) + 1    # scale away n

if __name__ == '__main__':
    from sys import argv
    x = np.arange(0.5, 1, .5/500)
    n = [None]
    try:
        n = [ int(argv[1]) ]
        n = xrange(int(argv[1]), int(argv[2]))
    except: pass
    dfe = df_err(x)
    for i in n:
        ulp = max_ulp(x, i, lambda x: dfe)
        print '%s\t%.3f = %d ULP' % (i, ulp, ulp)

# To generate max_ulp distribution plot
# run max_ulp.py -352 309 > data
# gnuplot> plot 'data'
#
# To generate plot for err vs x (0.5 to 1):
# sed "s/# for/for/" < max_ulp.py > abs_err.py
#
# abs_err.py > data             # max 8 scalings + max_ulp
# gnuplot> plot 'data' every ::::499, '' every ::500
#
# abs_err.py -306 > data        # where the peak occurs
# gnuplot> plot 'data'
