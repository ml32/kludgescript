#include "compiler.h"

#include "lexer.h"
#include "langdefs.h"

#include "util/list.h"
#include "util/array.h"

#include <string.h>
#include <stdio.h>

static int precedence(int op) {
  switch(op) {
    case KL_UADD:
    case KL_USUB:
    case KL_BITNOT:
    case KL_LOGNOT:
    case KL_SINE:
    case KL_COSINE:
      return 7;
    case KL_MUL:
    case KL_DIV:
    case KL_FDIV:
    case KL_MOD:
      return 6;
    case KL_ADD:
    case KL_SUB:
      return 5;
    case KL_ASHFTL:
    case KL_ASHFTR:
    case KL_LSHFTL:
    case KL_LSHFTR:
      return 4;
    case KL_BITAND:
    case KL_BITOR:
    case KL_BITXOR:
      return 3;
    case KL_CMP:
    case KL_NEQ:
    case KL_EQ:
    case KL_LT:
    case KL_GT:
    case KL_LEQ:
    case KL_GEQ:
      return 2;
    case KL_LOGAND:
    case KL_LOGOR:
      return 1;
  }
  return 0;
}

kl_code_t* kl_compile(kl_lexer_t* source) {
  array_t code;
  array_init(&code, sizeof(kl_ins_t));
  list_t  stack = LIST_INITIALIZER;

  kl_token_t token;
  kl_ins_t ins;
  for (;;) {
    kl_lexer_next(source, &token);
    if (token.header.type == KL_NONE) break;
    if (token.header.type == KL_NUMBER) {
      ins.op          = KL_PUSH;
      ins.arg.ns      = KL_NS_IMMEDIATE;
      ins.arg.val.num = token.num.val;
      array_push(&code, &ins);
    } else if (token.header.type & KL_FLAG_UNOP || token.header.type & KL_FLAG_BINOP) {
      int pre = precedence(token.header.type);
      for (;;) {
        kl_token_t *top = list_peek(&stack);
        if (top == NULL) break;

        int associativity = token.header.type & KL_FLAG_ASSOCIATIVITY;
        if (precedence(top->header.type) > pre || (precedence(top->header.type) >= pre && !associativity)) {
          top = list_pop(&stack);

          ins.op          = top->header.type;
          ins.arg.ns      = KL_NS_IMMEDIATE;
          ins.arg.val.num = KL_NUM_ZERO;
          array_push(&code, &ins);

          free(top);
        } else break;
      }

      kl_token_t *t = malloc(sizeof(kl_token_t));
      memcpy(t, &token, sizeof(kl_token_t));
      list_push(&stack, t);
    } else if (token.header.type == KL_END) {
      while (!list_isempty(&stack)) {
        kl_token_t *top = list_peek(&stack);
        if (top == NULL) break;

        top = list_pop(&stack);

        ins.op          = top->header.type;
        ins.arg.ns      = KL_NS_IMMEDIATE;
        ins.arg.val.num = KL_NUM_ZERO;
        array_push(&code, &ins);

        free(top);
      }
      break;
    }
  }

  kl_code_t *c = malloc(sizeof(kl_code_t) + array_bytes(&code));
  c->n = array_size(&code);
  memcpy(c->ins, array_data(&code), array_bytes(&code));
  array_free(&code);

  return c;
}

void kl_code_print(kl_code_t *code) {
  for (int i=0; i < code->n; i++) {
    kl_ins_t *ins = &code->ins[i];
    if (ins->arg.ns == KL_NS_IMMEDIATE) {
      printf("%s: IMM, %.5f\n", kl_langdef_name(ins->op), kl_numtofloat(ins->arg.val.num));
    } else {
      printf("%s: %u, %u\n", kl_langdef_name(ins->op), ins->arg.ns, ins->arg.val.ref);
    }
  }
}
