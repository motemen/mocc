#ifndef _TYPE_H_
#define _TYPE_H_

#include "parse.h"
#include <stddef.h>

typedef struct Type Type;
typedef struct Var Var;
typedef struct String String;

typedef enum TypeKind { CHAR, INT, PTR, ARRAY, STRUCT } TypeKind;

struct Type {
  TypeKind ty;
  Type *base;
  Type *next;

  size_t array_size; // ty == ARRAY
  Var *members;      // ty == STRUCT

  // 定義されたものはこれ
  char *name;
  int name_len; // name の長さ
};

extern Type defined_types;

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

Type *add_or_find_defined_type(Type *type);

#endif