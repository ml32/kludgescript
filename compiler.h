#ifndef KL_COMPILER_H
#define KL_COMPILER_H

#include "number.h"
#include "lexer.h"

typedef struct kl_valref {
  uint32_t ns;  /* namespace/object id */
  union {
    uint32_t    ref; /* reference */
    kl_number_t num; /* immediate-mode value */
  } val;
} kl_valref_t;

#define KL_NS_IMMEDIATE 0xFFFFFFFF

typedef struct kl_ins {
  uint32_t    op;
  kl_valref_t arg;
} kl_ins_t;

typedef struct kl_code {
  int      n;
  kl_ins_t ins[];
} kl_code_t;

kl_code_t* kl_compile(kl_lexer_t* source);
void kl_code_print(kl_code_t *code);

#endif
