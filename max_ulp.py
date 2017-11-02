# To generate max_ulp distribution plot
# max_ulp.py -352 309 > data
# gnuplot> plot 'data'
#
# To generate plot for err vs x (0.5 to 1):
# sed "s/# for/for/" < max_ulp.py > abs_err.py
#
# abs_err.py > data             # max 8 scalings + max_ulp
# gnuplot> plot 'data' every ::::499, '' every ::500::999
#
# abs_err.py -306 > data        # where the peak occurs
# gnuplot> plot 'data' every ::::499 u (0.5+$0/1000):1

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
    # for i in err: print i + 1   # +1 for last chop
    return np.max(err)

def df_err(x, m0=1e28/2**94):   # chopped 30+ digits
    return (4 - 2*(x<m0)) / x   # if x<m0, scale by 2**95

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
    for i in n:
        ulp = max_ulp(x, i, df_err)
        print '%s\t%.3f = %d ULP' % (i, ulp, ulp)

# PROVE: max ulp at most 25 ulp
# =============================
# 
# Scale 7 times or less:
# 
#     If nexp >= 0:
#     worst case: 4x 1e60 + 2x 1e25 + final
#     max rel error = 2.4786*4 + 2.9026*2 + 7.9228 + 1/m
#     max abs error = 23.6424 m + 1 = 24 ULP
# 
#     If nexp < 0:
#     worst case = (scale 8 times) - (scale 1E-73 TOP)
#     max rel error = 26.6254 + 1/m - 2.7402
#     max abs error = 23.8852 m + 1 = 24 ULP
#     
# Scale 8 times = 1e-73 5 times + 1e25 + (1e25 or 1e12) + final
# 
#     The trick to the prove is follow normalization branches
#     and to split m0 = [0.5, 1) into 3 regions
# 
#     1e-73 5 times:
#     1e-73 * 2^242 = 0.70674
#     0.70674^5 = 0.17632 = 0.70527/4 = 1.41054/8
#     -> If m0 < 0.5/0.70527 = 0.70895, THEN 3 bot else 2 bot
# 
#     1e+25 2 times:
#     1e+25 / 2^84 = 0.51699 = 1.03398/2
#     0.51699^2 = 0.26728 = 0.53456/2
#     -> If m5 < 0.5/0.53456 = 0.93536, THEN 2 bot else 1 bot
# 
#     Range m0 = [0.5, 0.66312)
#     1E-73 rel err = 2.7402*2 + 2.3384*3 = 12.4956
#     After 1E-73 5x, m5 = 1.41054 * m0 = [0.70527, 0.93536)
#     1E25 bot x2:
#         m6 = 1.03398 m5 = [0.72924, 0.96714)
#         m7 = 1.03398 m6 = [0.75402, 1)
#         m6 err = 1.50063/m6 <= 1.50063/0.72924 = 2.0578
#         m7 err = 1.50063/m7 <= 1.50063/0.75402 = 1.9902
#     IF 1E12 used to scale m6 instead, m7 err  <= 2
#     max rel err = 7.9228 + 12.4956 + 2.0578 + 2 + 1/m
#     max abs err = 24.4762 m + 1 = 25 ULP
# 
#     Range m0 = [0.66312, 0.70895)
#     After 1E-73 5x, m5 = 1.41054 * m0 = [0.93536, 1)
#     1E+25 rel err = 2.5006 + 2.9026 = 5.4032
#     m0 * 2^94 = [1.3134E+028, 1.4042E+028]   
#     dec-bin error = 7.9228/1.3134 = 6.0323
#     max rel error = 6.0323 + 12.4956 + 5.4032 + 1/m
#     max abs error = 23.9311 m + 1 = 24 ULP
#     
#     Range: m0 = [0.70895, 1)
#     1E-73 rel err = 2.7402*3 + 2.3384*2 = 12.8974
#     1E+25 rel err = 2.9026*2 = 5.8052
#     m0 * 2^94 = [1.4042E+028, 1.9807E+028]
#     dec-bin error = 7.9228/1.4042 = 5.6422 
#     max rel error = 5.6422 + 12.8974 + 5.8052 + 1/m
#     max abs error = 24.3448 m + 1 = 25 ULP
# 
# QED
