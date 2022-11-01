#include <stdbool.h>
#include <stddef.h>

typedef struct Type Type;
typedef struct NodeList NodeList;
typedef struct Node Node;
typedef struct LVar LVar;
typedef struct GVar GVar;

struct Type {
  enum { CHAR, INT, PTR, ARRAY } ty;
  Type *base;

  size_t array_size; // ty==ARRAY のときのみ
};

struct LVar {
  LVar *next;
  char *name;
  int len;       // name の長さ
  char *context; // この lvar が定義されている関数名
  Type *type;
  int offset; // メモリ上のオフセット。fp からの位置
};

struct GVar {
  GVar *next;
  char *name;
  int len; // name の長さ
  Type *type;
};

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
  ND_GVARDECL,
  ND_GVAR,
} NodeKind;

struct Node {
  NodeKind kind;

  Node *lhs; // ND_IF, ND_WHILE のときは expr
  Node *rhs; // ND_IF, ND_WHILE のときは stmt
  Node *node3; // ND_IF のときは else, ND_FOR のときは i++ みたいなとこ
  Node *node4; // ND_FOR のときのみ stmt

  // ND_BLOCK のとき: stmt, ...
  // ND_CALL のとき: params = expr, ...
  // ND_FUNCDECL のとき: args = ident, ...
  NodeList *nodes;

  // あとで locals とかにしたいかも？？
  // すくなくとも LVar *にしたい気がする
  NodeList *args;

  // ND_CALL のときだけ。関数名
  char *name;
  int name_len;

  int val;    // used when kind == ND_NUM
  LVar *lvar; // ND_LVAR || ND_VARDECL
  GVar *gvar; // ND_VARDECL かつトップレベル

  // ソースコード由来でなく、コンパイラの都合で生成されたノード
  bool is_synthetic_ptr;

  char *source_pos; // デバッグ用
  int source_len;   // デバッグ用
};

struct NodeList {
  Node *node;
  struct NodeList *next;
};

void parse_program();
extern Node *code[100];
char *node_kind_to_str(NodeKind kind);
int sizeof_type(Type *type);
Type *typeof_node(Node *node);

extern LVar *locals;

extern char *context;