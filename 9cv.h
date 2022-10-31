typedef struct Type {
  enum { INT, PTR } ty;
  struct Type *ptr_to;
} Type;

typedef struct LVar LVar;

struct LVar {
  LVar *next;
  char *name;
  int len;       // name の長さ
  char *context; // この lvar が定義されている関数名
  Type *type;
  int offset; // メモリ上のオフセット。fp からの位置
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
  ND_FUNCDECL,
  ND_DEREF,   // unary *
  ND_ADDR,    // unary &
  ND_VARDECL, //
} NodeKind;

struct NodeList;

struct Node {
  NodeKind kind;

  Node *lhs; // ND_IF, ND_WHILE のときは expr
  Node *rhs; // ND_IF, ND_WHILE のときは stmt
  Node *node3; // ND_IF のときは else, ND_FOR のときは i++ みたいなとこ
  Node *node4; // ND_FOR のときのみ stmt

  // ND_BLOCK のとき: stmt, ...
  // ND_CALL のとき: params = expr, ...
  // ND_FUNCDECL のとき: args = ident, ...
  struct NodeList *nodes;

  // あとで locals とかにしたいかも？？
  // すくなくとも LVar *にしたい気がする
  struct NodeList *args;

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

Node *parse_expr();
void parse_program();
extern Node *code[100];
void codegen_visit(Node *node);
void codegen_pop_t0();
char *node_kind_to_str(NodeKind kind);

extern char *user_input;

extern LVar *locals;