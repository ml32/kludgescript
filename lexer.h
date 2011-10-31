#ifndef KL_LEXER_H
#define KL_LEXER_H

#include <stdint.h>
#include "number.h"

typedef int  (*kl_lexer_read_cb)(void);
typedef void (*kl_lexer_err_cb)(char*);

typedef struct kl_lexer {
  kl_lexer_read_cb read;
  kl_lexer_err_cb  error;
  int cur;  /* current character */
  int line; /* current line */
  int last; /* type of last token */
} kl_lexer_t;

#define KL_TOKEN_SIZE   0x0100
#define KL_TOKEN_STRLEN 0x00FC
typedef struct kl_token_header {
  int type;
  int line;
} kl_token_header_t;

typedef struct kl_token_str {
  kl_token_header_t header;
  int32_t n;
  char    str[KL_TOKEN_STRLEN];
} kl_token_str_t;

typedef struct kl_token_num {
  kl_token_header_t header;
  kl_number_t val;
} kl_token_num_t;

typedef union kl_token {
  kl_token_header_t header;
  kl_token_str_t    str;
  kl_token_num_t    num;
} kl_token_t;

void kl_lexer_init(kl_lexer_t *source, kl_lexer_read_cb read, kl_lexer_err_cb error);
void kl_lexer_next(kl_lexer_t *source, kl_token_t *token);

#endif
