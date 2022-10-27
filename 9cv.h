typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};

extern Token *token;

typedef struct Node Node;

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_GE,  // >=
  ND_NUM, // numbers
} NodeKind;

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val; // used when kind == ND_NUM
};

void error(char *fmt, ...);

Token *tokenize(char *p);
Node *expr();
void visit(Node *node);