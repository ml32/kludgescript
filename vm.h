#ifndef KL_VM_H
#define KL_VM_H

#include "parser.h"

#include <stdint.h>

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

#define KL_VM_STACKSIZE 0x00100000 /* in elements, not bytes */

typedef struct kl_vm {
  int         sp;
  kl_valref_t stack[KL_VM_STACKSIZE];
} kl_vm_t;

#define KL_VM_INITIALIZER \
  { .sp = 0 }

static inline void kl_vm_init(kl_vm_t* vm) {
  vm->sp = 0;
}

kl_code_t* kl_vm_compile(kl_expression_t* root);
void kl_vm_exec(kl_vm_t* vm, kl_code_t* code);

#endif /* KL_VM_H */
