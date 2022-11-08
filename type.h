#ifndef _TYPE_H_
#define _TYPE_H_

#include "parse.h"
#include <stddef.h>

typedef struct Type Type;
typedef struct LVar LVar;
typedef struct GVar GVar;
typedef struct StrLit StrLit;

struct Type {
  enum { CHAR, INT, PTR, ARRAY, STRUCT } ty;
  Type *base;

  size_t array_size; // ty == ARRAY
  LVar *members;     // ty == STRUCT
};

typedef struct NamedType NamedType;

typedef enum {
  NT_STRUCT,
} NamedTypeKind;

struct NamedType {
  NamedType *next;
  Type *type;

  char *name;
  int len; // name の長さ

  NamedTypeKind kind; // ここに enum とか typedef が登場する予定
};

struct LVar {
  LVar *next;
  char *name;
  int len; // name の長さ
  Node *scope;
  Type *type;
  int offset; // メモリ上のオフセット。fp からの位置
};

struct GVar {
  GVar *next;
  char *name;
  int len; // name の長さ
  Type *type;
};

// 文字列リテラル!!!
struct StrLit {
  StrLit *next;
  char *str;
  int len;
  int index;
};

extern LVar locals_head;
extern StrLit str_lits_head;

int sizeof_type(Type *type);
Type *typeof_node(Node *node);

char *type_to_str(Type *type);
LVar *find_lvar(LVar *head, Node *scope, char *name, int len);

LVar *add_lvar(LVar *head, Node *scope, char *name, int len, Type *type);
GVar *find_gvar(char *name, int len);
GVar *add_gvar(char *name, int len, Type *type);
StrLit *add_str_lit(char *str, int len);
NamedType *find_named_type(NamedTypeKind kind, char *name, int len);
NamedType *add_named_type(NamedTypeKind kind, char *name, int len, Type *type);

#endif