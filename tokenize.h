#ifndef _TOKENIZE_H_
#define _TOKENIZE_H_

#include <stdbool.h>

typedef enum {
  TK_PUNCT,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_IDENT,
  TK_NUM,
  TK_EOF,
  TK_TYPE,
  TK_SIZEOF,
  TK_STRING,
  TK_BREAK,
  TK_CONTINUE,
  TK_STRUCT,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;

  int val;

  // ソースコード上の位置
  char *str;
  int len;
};

extern Token *curr_token;
extern Token *prev_token;

void tokenize(char *p);

Token *token_consume(TokenKind kind);
bool token_consume_punct(char *op);
bool token_consume_type(char *type);
bool token_at_eof();
void token_expect_punct(char *op);
int token_expect_number();

#endif