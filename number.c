#include "number.h"

#include <stdio.h>
#include <stdlib.h>

#include "trigtable.h"

kl_number_t kl_strtoinum(char *str, int n) {
  kl_number_t result = kl_inttonum(0);
  kl_number_t value  = kl_inttonum(1);

  int start = (str[0] == '+' || str[0] == '-') ? 1 : 0;

  int digit;
  for (int i=n-1; i>=start; i--) {
    digit   = str[i] - '0';
    if (digit < 0 || digit > 9) { return 0; }
    result += kl_num_mul(kl_inttonum(digit), value);
    value   = kl_num_mul(value, kl_inttonum(10));
  }

  if (str[0] == '-') result = -result;

  return result;
}
kl_number_t kl_strtofnum(char *str, int n) {
  kl_number_t result  = kl_inttonum(0);
  kl_number_t divisor = kl_inttonum(10);
  int      digit;
  for (int i=0; i<n; i++) {
    digit   = str[i] - '0';
    if (digit < 0 || digit > 9) { return 0; }
    result += kl_num_div(kl_inttonum(digit), divisor);
    divisor = kl_num_mul(divisor, kl_inttonum(10));
  }
  return result;
}
kl_number_t kl_strtonum(char *str, int n) {
  int decimal = -1;
  for (int i=0; i<n; i++) {
    char c = str[i];
    if (c == '.') {
      decimal = i;
    } else if (c < '0' || c > '9') {
      return 01;
    }
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
