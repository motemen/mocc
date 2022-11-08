#ifndef _TYPE_H_
#define _TYPE_H_

#include "parse.h"
#include <stddef.h>

typedef struct Type Type;
typedef struct Var Var;
typedef struct String String;

struct Type {
  enum { CHAR, INT, PTR, ARRAY, STRUCT } ty;
  Type *base;

  size_t array_size; // ty == ARRAY
  Var *members;      // ty == STRUCT
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

struct Var {
  Var *next;
  char *name;
  int len; // name の長さ
  Type *type;
  int offset; // メモリ上のオフセット。fp からの位置
};

// 文字列リテラル!!!
struct String {
  String *next;
  char *str;
  int len;
  int index;
};

extern Var globals;
extern String strings;

int sizeof_type(Type *type);
Type *typeof_node(Node *node);

char *type_to_string(Type *type);

Var *add_var(Var *head, char *name, int len, Type *type);
Var *find_var(Var *head, char *name, int len);

String *add_string(char *str, int len);

NamedType *add_named_type(NamedTypeKind kind, char *name, int len, Type *type);
NamedType *find_named_type(NamedTypeKind kind, char *name, int len);

#endif