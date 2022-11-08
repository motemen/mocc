#ifndef _PARSE_H_
#define _PARSE_H_

typedef struct NodeList NodeList;
typedef struct Node Node;

#include "tokenize.h"
#include "type.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  ND_ADD,   // +
  ND_SUB,   // -
  ND_MUL,   // *
  ND_DIV,   // /
  ND_EQ,    // ==
  ND_NE,    // !=
  ND_LT,    // <
  ND_GE,    // >=
  ND_LOGOR, // ||
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
  ND_STRING,
  ND_BREAK,
  ND_CONTINUE,
  ND_NOP,
  ND_MEMBER,
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
  // ND_GVARDECL のときは initializer = { expr, ... } （rhs == NULL
  // のときにかぎる）
  NodeList *nodes;

  // あとで locals とかにしたいかも？？
  // すくなくとも LVar *にしたい気がする
  NodeList *args;

  // ND_CALL, ND_FUNCDECL のときだけ。関数名
  // そのうち関数の使用に宣言が必要になったら
  // これも Func * みたいなものになりそう (cf. lvar, gvar)
  Token *ident;

  int val;   // used when kind == ND_NUM
             // あと ND_STRING のとき str_lits の index
  Var *lvar; // ND_LVAR || ND_VARDECL
  Var *gvar; // ND_VARDECL かつトップレベル
  Var *locals;

  int label_index; // ND_WHILE || ND_FOR

  char *source_pos; // デバッグ用
  int source_len;   // デバッグ用
};

struct NodeList {
  Node *node;
  struct NodeList *next;
};

typedef struct Scope Scope;

struct Scope {
  Scope *parent;
  Node *node;
  // ここにブロック中のローカル変数も出てくるかもしれない
};

void parse_program();
extern Node *code[100];
char *node_kind_to_str(NodeKind kind);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
char *type_to_str(Type *type);

#endif