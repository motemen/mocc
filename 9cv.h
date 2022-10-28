typedef enum {
  TK_RESERVED,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_IDENT,
  TK_NUM,
  TK_EOF,
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

typedef struct LVar LVar;

struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};

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
  ND_LVAR,
  ND_ASSIGN, // =
  ND_NUM,    // numbers
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_CALL,
} NodeKind;

struct NodeList;

struct Node {
  NodeKind kind;

  Node *lhs; // ND_IF, ND_WHILE のときは expr
  Node *rhs; // ND_IF, ND_WHILE のときは stmt
  Node *node3; // ND_IF のときは else, ND_FOR のときは i++ みたいなとこ
  Node *node4; // ND_FOR のときのみ stmt
  struct NodeList
      *nodes; // ND_BLOCK のとき: stmt, ... ND_CALL のとき: args = expr, ...

  // ND_CALL のときだけ。関数名
  char *name;
  int name_len;

  int val;    // used when kind == ND_NUM
  LVar *lvar; // used when kind == ND_LVAR

  char *source_pos; // デバッグ用
  int source_len;   // デバッグ用
};

typedef struct NodeList {
  Node *node;
  struct NodeList *next;
} NodeList;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

Token *tokenize(char *p);
Node *parse_expr();
void parse_program();
extern Node *code[100];
void codegen_visit(Node *node);
void codegen_pop_t0();
char *node_kind_to_str(NodeKind kind);

extern LVar *locals;