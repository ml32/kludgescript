#ifndef KL_OBJECT_H
#define KL_OBJECT_H

#include "number.h"

#define KL_OBJ_NUM 0x00;

typedef struct kl_object {
  int type;
  int gc;
} kl_object_t;

typedef struct kl_obj_num_t {
  kl_object_t obj;
  kl_number_t val;
} kl_obj_num_t;

#endif /* KL_OBJECT_H */
