#ifndef KL_PARSER_H
#define KL_PARSER_H

#include "util/list.h"

#include "number.h"
#include "lexer.h"

typedef struct kl_expression {
  int type;
  int line;
} kl_expression_t;

typedef struct kl_expr_var {
  kl_expression_t expr;
  int  n;
  char name[];
} kl_expr_var_t;

typedef struct kl_expr_imm {
  kl_expression_t expr;
  kl_number_t val;
} kl_expr_imm_t;

typedef struct kl_expr_binop {
  kl_expression_t expr;
  struct kl_expression* args[2];
} kl_expr_binop_t;

typedef struct kl_expr_unop {
  kl_expression_t expr;
  struct kl_expression* arg;
} kl_expr_unop_t;

typedef struct kl_expr_print {
  kl_expression_t expr;
  struct kl_expression* arg;
} kl_expr_print_t;

typedef struct kl_expr_block {
  kl_expression_t expr;
  list_t          list;
} kl_expr_block_t;

kl_expression_t* kl_parse(kl_lexer_t* source);
kl_expression_t* kl_build_expr(kl_lexer_t* source);
kl_expression_t* kl_parse_expr(list_t* list);
void kl_expr_free(kl_expression_t *expr);

#endif
