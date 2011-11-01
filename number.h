#ifndef KL_NUMBER_H
#define KL_NUMBER_H

#include <stdint.h>

typedef int32_t kl_number_t;
#define KL_NUM_FBITS  16
#define KL_NUM_FMASK  0x0000FFFF
#define KL_NUM_FDIV   0x00010000
#define KL_NUM_MSB    0x80000000
#define KL_NUM_ONE    0x00010000
#define KL_NUM_ZERO   0x00000000
#define KL_NUM_HALF   0x00008000
#define KL_NUM_TENTH  0x0000199A
#define KL_NUM_PI     0x0003243F
#define KL_NUM_HALFPI (KL_NUM_PI >> 1)
#define KL_NUM_E      0x0002b7e1
#define KL_NUM_LB_E   0x00017154 /* base-2 logarithm of 'e' */
#define KL_NUM_LB_TEN 0x00035269

kl_number_t kl_strtoinum(char *str, int n); /* integer part */
kl_number_t kl_strtofnum(char *str, int n); /* fractional part, no sign allowed */
kl_number_t kl_strtonum(char *str, int n); /* complete fixed-point number */

/* FIXME: these macros ASSUME that the compiler uses arithmetic shift for signed types
 * and logical shift for unsigned types */

#define kl_num_mul(a, b) \
  (((int64_t)(a) * (int64_t)(b)) >> KL_NUM_FBITS)
#define kl_num_div(a, b) \
  (((int64_t)(a) << KL_NUM_FBITS) / (b))

#define kl_num_ashftl(a, b) \
  ((a) << ((b) >> KL_NUM_FBITS))
#define kl_num_ashftr(a, b) \
  ((a) >> ((b) >> KL_NUM_FBITS))
#define kl_num_lshftl(a, b) \
  ((a) << ((b) >> KL_NUM_FBITS))
static inline kl_number_t kl_num_lshftr(kl_number_t a, kl_number_t b) {
  uint32_t c = *((uint32_t*)&a);
  int      d = b >> KL_NUM_FBITS;
  c >>= d;
  return *((kl_number_t*)&c);
}

#define kl_inttonum(a) \
  ((kl_number_t)(a) << KL_NUM_FBITS)
#define kl_floorint(a) \
  ((int)((a) >> KL_NUM_FBITS))
#define kl_roundint(a) \
  ((int)( (((a) < 0) ? (a) - KL_NUM_HALF : (a) + KL_NUM_HALF) >> KL_NUM_FBITS))

#define kl_numtofloat(a) \
  ((float)(a) / (float)(KL_NUM_FDIV))
#define kl_numtodouble(a) \
  ((double)(a) / (double)(KL_NUM_FDIV))

kl_number_t kl_num_sin(kl_number_t theta);
static inline kl_number_t kl_num_cos(kl_number_t theta) {
  return kl_num_sin(KL_NUM_HALFPI - theta);
}
kl_number_t kl_num_taylor_sin(kl_number_t x);

kl_number_t kl_num_lb(kl_number_t x);
static inline kl_number_t kl_num_ln(kl_number_t x) {
  return kl_num_div(kl_num_lb(x), KL_NUM_LB_E);
}
static inline kl_number_t kl_num_lg(kl_number_t x) {
  return kl_num_div(kl_num_lb(x), KL_NUM_LB_TEN);
}

#endif
