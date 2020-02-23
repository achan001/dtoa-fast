struct dtoa_case {int head, len, dec; char str[LAST+9];};
static struct dtoa_case safe, edge;

static void dtoa_iround(char* str, int n, int *len, int *dec, int mode)
{
  int i = *len;
  if (n <= i) {*len = n; return;}   // no round
  mode += mode ? '5' : str[i];
  if (mode>'5' || (mode=='5' && (n>i+1 || (str[i-1] & 1)))) {
    while(i-- && ++str[i] > '9') ;  // round up
    if (i < 0) {str[i = 0] = '1'; ++*dec;}
  } else {
    while (str[--i] == '0') ;       // round down
  }
  *len = i + 1;
}
#if USE_MAPM
static void dtoa_M_2str(char *s, UCHAR *M_d, int n)
{
  n = (n+2) >> 1;
  for(int i=0; i<n; i++) {          // build 1+ extra digits
    *s++ = M_d[i] / 10 + '0';       // bad extra digits OK
    *s++ = M_d[i] % 10 + '0';       // all will rounded away
  }
}
#endif
static void dtoa_hard(struct dtoa_case *c, uint64_t m, int bexp, int mode)
{
#if USE_MAPM
  M_APM x = M_get_stack_var();
  m_apm_set_uint64_t(x, m);
  m_apm_ishift(x, bexp);
  int n  = x->m_apm_datalength;
  c->dec = x->m_apm_exponent;
  dtoa_M_2str(c->str+1, x->m_apm_data, c->len);
  dtoa_iround(c->str+1, n, &c->len, &c->dec, mode);
  M_restore_stack(1);
#else /* Default = GMP */
  char str[770];
  strtod_scale(str, &c->dec, m, bexp);
  int n = strlen(str);
  c->dec += n;
  while (str[--n] == '0') ;
  dtoa_iround(str, n+1, &c->len, &c->dec, mode);
  memcpy(c->str+1, str, c->len);
#endif
}

static int dtoa_safe(int64_t hi, int gap, int dec)
{
  EPRINT("safe = %I64d to %I64d\n", hi - gap, hi);

  // hi = m2 1E9 + m1, m1 = [0, 1E9)
  int m2 = lrint(1E-9 * (hi - (int64_t) 5E8));
  int m1 = hi - (int64_t) 1E9 * m2;
  if ((unsigned) m1 >= (unsigned) 1E9)
    m1 += (m1 < 0) ? (m2--, (int) 1E9) : (m2++, (int) -1E9);

  int i = LAST, j = LAST;
  memset(safe.str, '0', LAST+1);    // build hi digits
  for(;;) {
    for(; m1 > 9; m1 /= 10U) safe.str[i--] += m1 % 10U;
    safe.str[i--] += m1;
    if (m2 == 0) break;
    m1 = m2; m2 = 0; i = LAST-9;    // build billions
  }

  safe.str[0] = safe.str[j] - gap;  // low last digit
  if (safe.str[0] < '1')            // hi too long
    while(safe.str[--j] == '0') ;

  safe.dec = dec - (safe.head = i);
  safe.len = j - i;
  return (safe.str[0] < '1');       // hi shortened
}

static int dtoa_edge(int digit)
{
  int i = LAST;
  while(safe.str[--i] == digit) ;   // skip digit
  edge.str[i] = safe.str[i] + (digit & 1);
  edge.len = i;
  while(--i > safe.head) edge.str[i] = safe.str[i];
  edge.len -= i;
  edge.dec = (safe.dec + safe.head) - (edge.head = i);
  return safe.len - 1;
}

// round-to-nearest, range = (x-ulp/2, x+ulp)
// MATCHES dtoa.c results for 2^n, n=[-1022, 1023]

static struct dtoa_case *dtoa_boundary
(int64_t m, double frac, int dec, double ulp)
{
  if (ulp >= 20./3) {
    int64_t m0 = m;
    ulp *= 0.1; dec++;
    frac = (frac + (m0 - 10 * (m/=10U))) * 0.1;
  }
  else if (ulp < 2./3) {
    int d = (int) (frac *= 10);
    m = m * 10 + d;
    ulp *= 10; dec--;
    frac -= d;
  }
  int i = lrint(frac - (ulp * 0.5 - 0.5));
  int j = lrint(frac + (ulp - 0.5));
  EPRINT("\nmantissa => %I64u + %.17g", m, frac);
  EPRINT("\nsafe ulp => %.17g (-ulp/2, +ulp)\n", ulp);

  if (!dtoa_safe(m + j, j - i, dec))
    if (i != j)
      safe.str[LAST] -= j - (frac > 0.5);
  return &safe;
}
