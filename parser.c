#include "parser.h"

#include "lexer.h"
#include "langdefs.h"

#include <string.h>
#include <stdio.h>

static int precedence(int op) {
  switch(op) {
    case KL_UADD:
    case KL_USUB:
    case KL_BITNOT:
    case KL_LOGNOT:
      return 1;
    case KL_MUL:
    case KL_DIV:
    case KL_FDIV:
    case KL_MOD:
      return 2;
    case KL_ADD:
    case KL_SUB:
      return 3;
    case KL_ASHFTL:
    case KL_ASHFTR:
    case KL_LSHFTL:
    case KL_LSHFTR:
      return 4;
    case KL_BITAND:
    case KL_BITOR:
    case KL_BITXOR:
      return 5;
    case KL_CMP:
    case KL_NEQ:
    case KL_EQ:
    case KL_LT:
    case KL_GT:
    case KL_LEQ:
    case KL_GEQ:
      return 6;
    case KL_LOGAND:
    case KL_LOGOR:
      return 7;
  }
  return 0;
}

static kl_expression_t* kl_expr_from_token(kl_token_t *token);
static int kl_expr_iscomplete(kl_expression_t* expr);
static void kl_parse_error(char *msg, int lineno);



kl_expression_t* kl_build_expr(kl_lexer_t* source) {
  list_t elist = LIST_INITIALIZER;

  kl_token_generic_t token;
  for (;;) {
    kl_lexer_next(source, &token);
    if (token.token.type == KL_NONE || token.token.type == KL_END) break;

    kl_expression_t* expr = kl_expr_from_token(&token.token);
    if (expr == NULL) continue; /* TODO: print some type of warning?? */

    list_insert_after(&elist, list_last(&elist), expr);
  }

  return kl_parse_expr(&elist);
}

kl_expression_t* kl_parse_expr(list_t* list) {
  if (list == NULL || list_isempty(list)) return NULL;

  list_node_t*     node = NULL;
  kl_expression_t* expr = NULL;

  list_node_t *cur;
  int maxp = -1;
  LIST_FOREACH(list, cur) {
    kl_expression_t* item = (kl_expression_t*)list_item(cur);

    if (kl_expr_iscomplete(item)) continue;

    if (item->type & KL_FLAG_GROUP) {
      node = cur;
      expr = item;
      break;
    }

    int p = precedence(item->type);
    if (p >= maxp) {
      maxp = p;
      node = cur;
      expr = item;
    }
  }

  /* misc operands */
  if (expr == NULL) {
    expr = list_pop(list);
    if (!list_isempty(list)) {
      kl_parse_error("Excess operands", expr->line);
      list_clear(list, (list_dealloc_cb)&kl_expr_free);
      kl_expr_free(expr);
      return NULL;
    }
    return expr;
  }

  /* parens */
  if (expr->type & KL_FLAG_GROUP) {
    if (expr->type == KL_RPAREN) {
      kl_parse_error("Unmatched right parentheses", expr->line);
      list_clear(list, (list_dealloc_cb)&kl_expr_free);
      return NULL;
    }

    list_node_t* start = node;
    list_node_t* end   = NULL;

    if (list_next(start) == NULL) {
      kl_parse_error("Unmatched left parentheses", expr->line);
      list_clear(list, (list_dealloc_cb)&kl_expr_free);
      return NULL;
    }
    int level = 1;
    for (cur = list_next(start); cur != NULL; cur = list_next(cur)) {
      kl_expression_t* item = (kl_expression_t*)list_item(cur);

      switch (item->type) {
        case KL_LPAREN:
          ++level; break;
        case KL_RPAREN:
          --level; break;
      }

      if (level <= 0) {
        end = cur; break;
      }
    }
    if (end == NULL) {
      kl_parse_error("Unmatched left parentheses", expr->line);
      list_clear(list, (list_dealloc_cb)&kl_expr_free);
      return NULL;
    }

    list_t left, center, right, temp;
    kl_expr_free(list_split(list, start, &left, &temp));
    kl_expr_free(list_split(&temp, end, &center, &right));

    kl_expression_t *subexpr = kl_parse_expr(&center);
    if (subexpr == NULL) {
      list_clear(&left,  (list_dealloc_cb)&kl_expr_free);
      list_clear(&right, (list_dealloc_cb)&kl_expr_free);
      return NULL;
    }
    list_init(&center);
    list_push(&center, subexpr);
    list_concat(&temp, &left, &center);
    list_concat(list, &temp, &right);

    return kl_parse_expr(list);
  }

  /* binary operators */
  if (expr->type & KL_FLAG_BINOP) {
    kl_expr_binop_t* bexpr = (kl_expr_binop_t*)expr;

    list_t listl, listr;
    list_split(list, node, &listl, &listr);
    if (list_isempty(&listl)) {
      kl_parse_error("Missing left operand", expr->line);
      list_clear(&listr, (list_dealloc_cb)&kl_expr_free);
      kl_expr_free(expr);
      return NULL;
    }
    if (list_isempty(&listr)) {
      kl_parse_error("Missing right operand", expr->line);
      list_clear(&listl, (list_dealloc_cb)&kl_expr_free);
      kl_expr_free(expr);
      return NULL;
    }

    kl_expression_t *rootl, *rootr;
    rootl = kl_parse_expr(&listl);
    rootr = kl_parse_expr(&listr);
    if (rootl == NULL || rootr == NULL) {
      kl_expr_free(rootl);
      kl_expr_free(rootr);
      kl_expr_free(expr);
      return NULL;
    }

    bexpr->args[0] = rootl;
    bexpr->args[1] = rootr;

    return expr;
  }

  /* unary operators */
  if (expr->type & KL_FLAG_UNOP) {
    kl_expr_unop_t* uexpr = (kl_expr_unop_t*)expr;

    list_t listl, listr;
    list_split(list, node, &listl, &listr);
    if (!list_isempty(&listl)) {
      kl_parse_error("Extra operand for unary operator", expr->line);
      list_clear(&listl, (list_dealloc_cb)&kl_expr_free);
      list_clear(&listr, (list_dealloc_cb)&kl_expr_free);
      kl_expr_free(expr);
      return NULL;
    }
    if (list_isempty(&listr)) {
      kl_parse_error("Missing right operand", expr->line);
      kl_expr_free(expr);
      return NULL;
    }

    kl_expression_t *arg;
    arg = kl_parse_expr(&listr);
    if (arg == NULL) {
      kl_expr_free(expr);
      return NULL;
    }

    uexpr->arg = arg;

    return expr;
  }

  kl_parse_error("Empty or unrecognised expression", expr->line);
  list_clear(list, (list_dealloc_cb)&kl_expr_free);
  return NULL;
}

void kl_expr_free(kl_expression_t *expr) {
  if (expr == NULL) return;
  if (expr->type & KL_FLAG_BINOP) {
    kl_expr_binop_t* bexpr = (kl_expr_binop_t*)expr;
    kl_expr_free(bexpr->args[0]);
    kl_expr_free(bexpr->args[1]);
  }
  if (expr->type & KL_FLAG_UNOP) {
    kl_expr_unop_t* uexpr = (kl_expr_unop_t*)expr;
    kl_expr_free(uexpr->arg);
  }
  free(expr);
}

static kl_expression_t* kl_expr_from_token(kl_token_t *token) {
  if (token->type & KL_FLAG_BINOP) {
    kl_expr_binop_t* bexpr = (kl_expr_binop_t*) malloc(sizeof(kl_expr_binop_t));

    bexpr->expr.type = token->type;
    bexpr->expr.line = token->line;

    bexpr->args[0] = NULL;
    bexpr->args[1] = NULL;

    return (kl_expression_t*)bexpr;
  }

  if (token->type & KL_FLAG_UNOP) {
    kl_expr_unop_t* uexpr = (kl_expr_unop_t*) malloc(sizeof(kl_expr_unop_t));

    uexpr->expr.type = token->type;
    uexpr->expr.line = token->line;

    uexpr->arg = NULL;

    return (kl_expression_t*)uexpr;
  }

  if (token->type == KL_LABEL) {
    kl_token_str_t* stoken = (kl_token_str_t*)token;
    int n = stoken->n;

    kl_expr_var_t* vexpr = (kl_expr_var_t*) malloc(sizeof(kl_expr_var_t) + n);

    vexpr->expr.type = token->type;
    vexpr->expr.line = token->line;

    vexpr->n = stoken->n;
    strncpy(vexpr->name, stoken->str, n);

    return (kl_expression_t*)vexpr;
  }

  if (token->type == KL_NUMBER) {
    kl_token_num_t* ntoken = (kl_token_num_t*)token;

    kl_expr_imm_t* iexpr = (kl_expr_imm_t*) malloc(sizeof(kl_expr_imm_t));

    iexpr->expr.type = token->type;
    iexpr->expr.line = token->line;

    iexpr->val = ntoken->val;

    return (kl_expression_t*)iexpr;
  }

  kl_expression_t* expr = (kl_expression_t*)malloc(sizeof(kl_expression_t));
  expr->type = token->type;
  expr->line = token->line;
  return expr;
}

static int kl_expr_iscomplete(kl_expression_t* expr) {
  if (expr->type & KL_FLAG_BINOP) {
    kl_expr_binop_t* bexpr = (kl_expr_binop_t*)expr;
    return (bexpr->args[0] != NULL && bexpr->args[1] != NULL);
  }

  if (expr->type & KL_FLAG_UNOP) {
    kl_expr_unop_t* uexpr = (kl_expr_unop_t*)expr;
    return (uexpr->arg != NULL);
  }

  if (expr->type & KL_FLAG_GROUP) {
    return 0;
  }

  return 1;
}

static void kl_parse_error(char *msg, int line) {
  fprintf(stderr, "KludgeScript -> Parse Error: %s on line %d\n", msg, line);
}
