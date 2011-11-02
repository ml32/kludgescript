#include "number.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "trigtable.h"
#include "logtable.h"

/* FIXME: detect overflow in conversions */

kl_number_t kl_strtoinum(char *str, int n) {
  kl_number_t result = kl_inttonum(0);
  kl_number_t value  = kl_inttonum(1);

  int start = (str[0] == '+' || str[0] == '-') ? 1 : 0;

  int digit;
  for (int i=n-1; i>=start; i--) {
    digit   = str[i] - '0';
    assert(digit >= 0 && digit <= 9);
    result += kl_num_mul(kl_inttonum(digit), value);
    value   = kl_num_mul(value, kl_inttonum(10));
  }

  if (str[0] == '-') result = -result;

  return result;
}

kl_number_t kl_strtofnum(char *str, int n) {
  kl_number_t result  = 0;
  kl_number_t divisor = 1;

  int digit;
  for (int i=0; i < n; i++) {
    digit    = str[i] - '0';
    assert(digit >= 0 && digit <= 9);
    result   = kl_num_mul(result, kl_inttonum(10));
    result  += digit;
    divisor  = kl_num_mul(divisor, kl_inttonum(10));
  }
  return kl_num_div(result, divisor);
}

kl_number_t kl_strtonum(char *str, int n) {
  int decimal = -1;
  for (int i=0; i<n; i++) {
    char c = str[i];
    if (c == '.') {
      decimal = i;
    } else assert(c >= '0' && c <= '9');
  }
  if (decimal < 1) {
    return kl_strtoinum(str, n);
  }
  return kl_strtoinum(str, decimal) + kl_strtoinum(str+(decimal+1), n);
}

kl_number_t kl_num_sin(kl_number_t theta) {
  kl_number_t x = kl_num_div(theta, KL_NUM_HALFPI);
  int         i = kl_floorint(x << 10);

  /* extract sign from quadrant */
  int s = i < 0;
  i  = abs(i);
  s ^= (i >> 11) & 1;

  /* clamp to range */
  i &= i & 0x07FF;

  /* mirror across x-axis */
  i = (i > 0x0400) ? 0x0800 - i : i;

  /* look up result */
  kl_number_t n = sine_values[i];

  return s ? -n : n;
}

kl_number_t kl_num_lb(kl_number_t x) {
  int n = 0;
  kl_number_t w = x;
  for (int i = sizeof(kl_number_t) * 8 / 2; i >= 0; i--) {
    if (w >= 1 << i) { w >>= i; n += i; }
  }
  n -= KL_NUM_FBITS;

  kl_number_t r = n < 0 ? KL_NUM_ONE >> -n : KL_NUM_ONE << n;

  kl_number_t y = kl_num_div(x, r);

  /* ten most signficant fractional bits used for course lookup */
  int i = (y >> (KL_NUM_FBITS - 10)) & 0x3FF;
  kl_number_t z0 = lb_values[i];
  kl_number_t z1 = lb_values[i+1];

  /* least significant bits used for linear interpolation */
  kl_number_t u = ((y << 10) & KL_NUM_FMASK);

  kl_number_t z = kl_num_mul(z1, u) + kl_num_mul(z0, KL_NUM_ONE - u);

  return kl_inttonum(n) + z;
}

/* taylor series expansion for sine (expensive!) */
/* DOES THIS STILL WORK WITH Q20? */
kl_number_t kl_num_taylor_sin(kl_number_t x) {
  if (x == KL_NUM_HALFPI) { return KL_NUM_ONE; }
  if (x == 0) { return KL_NUM_ZERO; }

  x <<= 4; /* Q16 to Q20 */

  int32_t y  = x;
  int64_t n  = x;
  int64_t x2 = ((int64_t)x * (int64_t)x) >> 20;

  n  = (n * x2) >> 20; /* x^3 */
  y -= n / 6;          /* x^3/3! */

  n  = (n * x2) >> 20; /* x^5 */
  y += n / 120;        /* x^5/5! */

  n  = (n * x2) >> 20; /* x^7 */
  y -= n / 5040;       /* x^7/7! */

  n  = (n * x2) >> 20; /* x^9 */
  y += n / 362880;     /* x^9/9! */

  return y >> 4;
}
