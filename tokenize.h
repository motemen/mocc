#include <stdbool.h>

typedef enum {
  TK_RESERVED, // これは TK_SYMBOL とかにしたい希ガス
  // "return", "if", ... とかは TK_RESERVED にしてもよい
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
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;

  int val; // kind == TK_NUM のときだけ。数値リテラル

  // ソースコード上の位置
  char *str;
  int len;
};

extern Token *token;

Token *tokenize(char *p);

bool token_consume(TokenKind kind);
Token *token_advance(TokenKind kind);
bool token_consume_reserved(char *op);
bool token_consume_type(char *type);
bool token_at_eof();
void token_expect(char *op);
int token_expect_number();

Token *token_consume_ident();