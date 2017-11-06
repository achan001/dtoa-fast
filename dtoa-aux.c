struct dtoa_case {int head, len, dec; char str[LAST+8];};
static struct dtoa_case safe, edge;

static void dtoa_iround(char* str, int n, int *len, int *dec, int mode)
{
  int i = *len;
  if (n <= i) {*len = n; return;}   // no round
  mode += mode ? '5' : str[i];
  if (mode>'5' || (mode=='5' && (n>i+1 || (str[i-1]&1)))) {
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

static int dtoa_safe(int64_t low, unsigned gap, int dec)
{
  // safe = shortest digits between low, low + gap
  EPRINT("safe = %I64d to %I64d\n", low, low + gap);
  int d, i = LAST, bot = 0;

  // convert low = m2 BILLION + m1
  int m2 = lrint(1E-9 * (low - (int64_t) (5E8-1)));
  int m1 = low - (int64_t) 1E9 * m2;
  if ((unsigned) m1 >= (unsigned) 1E9)
    m1 += (m1 < 0) ? (m2--, (int) 1E9)
                   : (m2++, (int) -1E9);

  do {
    bot |= d = m1 % 10U;            // check bottom 0's
    edge.str[i] = d + '0';          // bottom edge digits
    if (i!=LAST-8) m1/=10U;         // remove last digit
    else m1=m2, m2=0;               // m1 used up, use m2
    gap = ((d += gap) > 9);         // do carry
    safe.str[i] = d + "0&"[gap];    // '&' = '0' - 10
  } while (i--, gap);

  int j = i+1;                      // now, same digits
  for( ; m1 ; m1/=10U) safe.str[i--] = m1 % 10U + '0';
  if (m2) {
    while (i > LAST-9) safe.str[i--] = '0';
    for(; m2; m2/=10U) safe.str[i--] = m2 % 10U + '0';
  }
  if (bot==0) {                     // bottom all 0's
    while(safe.str[--j] == '0') ;   // trim digits
  }
  safe.dec = dec - (safe.head = i);
  safe.len = j - i;
  return LAST - j;                  // shorten digits
}

static int dtoa_del_digit(int digit)
{
  int i = LAST;
  edge.dec = safe.dec;
  safe.str[safe.head] = 0;          // "close" string
  while(safe.str[--i] == digit) ;   // skip digit
  digit &= 1;                       // '0' -> 0, '9' -> 1
  if (i == safe.head) {             // nothing left
    edge.str[2] = '1';              // build 10^(n+1)
    edge.dec += 1;
    return edge.len = edge.head = digit;
  }
  edge.len = i;
  while((edge.str[i] = safe.str[i])) i--;
  edge.str[edge.len] += digit;      // do carry
  return edge.len -= (edge.head = i);
}

// just enough code to handle 0x1p-1022 to 0x1p1023
// round-to-nearest, range = (x-ulp/2, x+ulp)
// confirm by BRUTE FORCE w/ FE_PC53_ENV and FE_PC64_ENV
// confirm by BRUTE FORCE ALL 2046 cases == dtoa_gay

static struct dtoa_case *dtoa_boundary
(int64_t m, double frac, int dec, double ulp)
{
  if (ulp >= 20./3) {
    uint64_t m0 = m;
    m /= 10U;
    ulp *= 0.1; dec++;
    frac = (frac + (m0 - m * 10)) * 0.1;
  }
  else if (ulp < 2./3) {
    int d = (int) (frac *= 10);
    m = m * 10 + d;
    ulp *= 10; dec--;
    frac -= d;
  }
  EPRINT("\nmantissa => %I64u + %.17g", m, frac);
  EPRINT("\nsafe ulp => %.17g (-ulp/2, +ulp)\n", ulp);

  int i = lrint(frac - (ulp * 0.5 - 0.5));
  int j = lrint(frac + (ulp - 0.5));
  if (dtoa_safe(m + i, j - i, dec)) return &safe;
  if (i != j)
    safe.str[LAST] -= j - (frac > 0.5);
  return &safe;
}
