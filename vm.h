#ifndef KL_VM_H
#define KL_VM_H

#include "compiler.h"

#include <stdint.h>

#define KL_VM_STACKSIZE 0x00100000 /* in elements, not bytes */

typedef struct kl_vm {
  int         sp;
  int         fp;
  kl_valref_t stack[KL_VM_STACKSIZE];
} kl_vm_t;

#define KL_VM_INITIALIZER \
  { .sp = -1 }

static inline void kl_vm_init(kl_vm_t* vm) {
  vm->sp = 0;
}

void kl_vm_exec(kl_vm_t* vm, kl_code_t* code);

#endif /* KL_VM_H */
