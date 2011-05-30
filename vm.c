#include "vm.h"

#include "langdefs.h"

#include <assert.h>
#include <stdio.h>

static inline void kl_ins_copy(kl_ins_t *src, kl_ins_t *dst) {
  dst->op  = src->op;
  dst->arg = src->arg;
}

static void kl_vm_compile_recursive(list_t* istack, kl_expression_t* root) {
  if (root==NULL) return;

  kl_ins_t *ins;
  if (root->type & KL_FLAG_BINOP) {
    ins = (kl_ins_t*)malloc(sizeof(kl_ins_t));
    ins->op  = root->type;
    ins->arg.ns      = KL_NS_IMMEDIATE;
    ins->arg.val.num = 0;
    list_push(istack, ins);

    kl_expr_binop_t* bexpr = (kl_expr_binop_t*)root;
    kl_vm_compile_recursive(istack, bexpr->args[0]);
    kl_vm_compile_recursive(istack, bexpr->args[1]);
    return;
  }
  if (root->type & KL_FLAG_UNOP) {
    ins = (kl_ins_t*)malloc(sizeof(kl_ins_t));
    ins->op  = root->type;
    ins->arg.ns      = KL_NS_IMMEDIATE;
    ins->arg.val.num = 0;
    list_push(istack, ins);

    kl_expr_unop_t* uexpr = (kl_expr_unop_t*)root;
    kl_vm_compile_recursive(istack, uexpr->arg);
    return;
  }
  switch (root->type) {
    case KL_NUMBER:
      ins = (kl_ins_t*)malloc(sizeof(kl_ins_t));
      kl_expr_imm_t* iexpr = (kl_expr_imm_t*)root;
      ins->op          = KL_PUSH;
      ins->arg.ns      = KL_NS_IMMEDIATE;
      ins->arg.val.num = iexpr->val;
      list_push(istack, ins);
      return;
  }
}

kl_code_t* kl_vm_compile(kl_expression_t* root) {
  list_t istack;
  kl_vm_compile_recursive(&istack, root);
  int n = list_count(&istack);

  int bytes = sizeof(kl_code_t) + n * sizeof(kl_ins_t);
  kl_code_t* code = (kl_code_t*)malloc(bytes);

  code->n = n;

  kl_ins_t *ins; int i = 0;
  while ((ins = list_pop(&istack)) != NULL) {
    kl_ins_copy(ins, code->ins + i);
    i++;
  }

  return code;
}

static inline void kl_vm_stack_push(kl_vm_t* vm, kl_valref_t valref) {
  int sp = vm->sp;
  ++vm->sp;
  assert(sp < KL_VM_STACKSIZE);
  vm->stack[sp] = valref;
}

static inline kl_valref_t kl_vm_stack_pop(kl_vm_t* vm) {
  int sp = --vm->sp;
  assert(sp >= 0);
  return vm->stack[sp];
}

static inline kl_valref_t kl_vm_add(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num + y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_sub(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num - y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_uadd(kl_valref_t x) {
  if (x.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_usub(kl_valref_t x) {
  if (x.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = -x.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_mul(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_mul(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_div(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_div(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_fdiv(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_div(x.val.num, y.val.num) & ~KL_NUM_FMASK };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_mod(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num % y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_ashftl(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_ashftl(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_ashftr(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_ashftr(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_lshftl(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_lshftl(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_lshftr(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_num_lshftr(x.val.num, y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_bitand(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num & y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_bitor(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num | y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_bitxor(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = x.val.num ^ y.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_bitnot(kl_valref_t x) {
  if (x.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = ~x.val.num };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_logand(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num && y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_logor(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num || y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_lognot(kl_valref_t x) {
  if (x.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(!x.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}


static inline kl_valref_t kl_vm_eq(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num == y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_neq(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num != y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_lt(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num < y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_gt(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num > y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_leq(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num <= y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

static inline kl_valref_t kl_vm_geq(kl_valref_t x, kl_valref_t y) {
  if (x.ns == KL_NS_IMMEDIATE && y.ns == KL_NS_IMMEDIATE) {
    return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = kl_inttonum(x.val.num >= y.val.num) };
  }
  return (kl_valref_t){ .ns = KL_NS_IMMEDIATE, .val.num = 0 };
}

#define KL_VM_BINOP(func) \
  x = kl_vm_stack_pop(vm);\
  y = kl_vm_stack_pop(vm);\
  z = (func)(x, y);\
  kl_vm_stack_push(vm, z);

#define KL_VM_UNOP(func) \
  x = kl_vm_stack_pop(vm);\
  z = (func)(x);\
  kl_vm_stack_push(vm, z);

void kl_vm_exec(kl_vm_t* vm, kl_code_t* code) {
  int ip = 0;
  while (ip < code->n) {
    kl_ins_t* ins = code->ins + ip;

    kl_valref_t x, y, z;
    switch (ins->op) {
      case KL_ADD:
        KL_VM_BINOP(kl_vm_add)
        break;
      case KL_UADD:
        KL_VM_UNOP(kl_vm_uadd)
        break;
      case KL_SUB:
        KL_VM_BINOP(kl_vm_sub)
        break;
      case KL_USUB:
        KL_VM_UNOP(kl_vm_usub)
        break;
      case KL_MUL:
        KL_VM_BINOP(kl_vm_mul)
        break;
      case KL_DIV:
        KL_VM_BINOP(kl_vm_div)
        break;
      case KL_FDIV:
        KL_VM_BINOP(kl_vm_fdiv)
        break;
      case KL_MOD:
        KL_VM_BINOP(kl_vm_mod)
        break;
      case KL_ASHFTL:
        KL_VM_BINOP(kl_vm_ashftl)
        break;
      case KL_ASHFTR:
        KL_VM_BINOP(kl_vm_ashftr)
        break;
      case KL_LSHFTL:
        KL_VM_BINOP(kl_vm_lshftl)
        break;
      case KL_LSHFTR:
        KL_VM_BINOP(kl_vm_lshftr)
        break;
      case KL_BITAND:
        KL_VM_BINOP(kl_vm_bitand)
        break;
      case KL_BITOR:
        KL_VM_BINOP(kl_vm_bitor)
        break;
      case KL_BITXOR:
        KL_VM_BINOP(kl_vm_bitxor)
        break;
      case KL_BITNOT:
        KL_VM_UNOP(kl_vm_bitnot)
        break;
      case KL_LOGAND:
        KL_VM_BINOP(kl_vm_logand)
        break;
      case KL_LOGOR:
        KL_VM_BINOP(kl_vm_logor)
        break;
      case KL_LOGNOT:
        KL_VM_UNOP(kl_vm_lognot)
        break;

      case KL_EQ:
        KL_VM_BINOP(kl_vm_eq)
        break;
      case KL_NEQ:
        KL_VM_BINOP(kl_vm_neq)
        break;
      case KL_LT:
        KL_VM_BINOP(kl_vm_lt)
        break;
      case KL_GT:
        KL_VM_BINOP(kl_vm_gt)
        break;
      case KL_LEQ:
        KL_VM_BINOP(kl_vm_leq)
        break;
      case KL_GEQ:
        KL_VM_BINOP(kl_vm_geq)
        break;

      case KL_PUSH:
        kl_vm_stack_push(vm, ins->arg);
        break;
    }

    ip++;
  }
}
