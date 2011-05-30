#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "number.h"

#include "lexer.h"
#include "parser.h"
#include "vm.h"

static int read() {
  return getchar();
}

static kl_vm_t vm = KL_VM_INITIALIZER;

static inline kl_valref_t kl_vm_stack_pop(kl_vm_t* vm) {
  int sp = --vm->sp;
  return vm->stack[sp];
}

int main() {
  /*
  FILE* f = fopen("trigtable.h", "wb");
  fprintf(f, "static int32_t sine_values[0x0401] = {\n");
  for (int i=0; i<=1024; i++) {
      int theta = kl_num_mul(kl_inttonum(i) >> 10, KL_NUM_HALFPI);
      fprintf(f, "\t0x%08x,\n", kl_num_taylor_sin(theta));
  }
  fprintf(f, "};\n");
  fclose(f);
  */
  /*
  kl_number_t i = 0;
  for (;;) {
      kl_number_t theta = kl_num_mul(KL_NUM_HALFPI, i);
      kl_number_t s     = kl_num_sin(theta);
      kl_number_t c     = kl_num_cos(theta);
      kl_number_t l     = kl_num_mul(s, s) + kl_num_mul(c, c);
      printf("\r%+1.2f: <%+1.5f, %+1.5f> %+1.5f",
             kl_numtodouble(i), kl_numtodouble(c), kl_numtodouble(s), kl_numtodouble(l));
      i += 0x00000040;
  }
  */
  kl_lexer_t source;

  kl_lexer_init(&source, read, NULL);

  kl_expression_t* expr;
  kl_code_t*       code;
  for (;;) {
    expr = kl_build_expr(&source);
    code = kl_vm_compile(expr);
    kl_vm_exec(&vm, code);

    kl_valref_t val = kl_vm_stack_pop(&vm);
    printf("result: %f\n", kl_numtofloat(val.val.num));

    kl_expr_free(expr);
    free(code);
  }
  return 0;
}
