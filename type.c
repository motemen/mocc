// 型と名前とスコープ

#include "parse.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Var globals;
String strings;
NamedType named_types;

char *type_to_string(Type *type) {
  char *buf = calloc(80, sizeof(char));
  if (type->ty == INT) {
    return "int";
  } else if (type->ty == PTR) {
    snprintf(buf, 80, "ptr to %s", type_to_string(type->base));
    return buf;
  } else if (type->ty == ARRAY) {
    snprintf(buf, 80, "array[%ld] of %s", type->array_size,
             type_to_string(type->base));
    return buf;
  }

  return "(unknown)";
}

Var *find_var(Var *head, char *name, int len) {
  for (Var *var = head->next; var; var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      return var;
    }
  }

  return NULL;
}

// locals を共有するために scope を置いてたけど、head を切り替える（そもそも
// scope ごとに locals を持つ） ことで不要になる予感
Var *add_var(Var *head, char *name, int len, Type *type) {
  int offset = 8;

  Var *last = head;
  for (Var *var = last->next; var; last = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len))
      error("variable already defined: '%.*s'", len, name);

    offset = var->offset;
  }

  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->len = len;
  var->offset = offset + (sizeof_type(type) + 7) / 8 * 8;
  var->type = type;

  last->next = var;

  return var;
}

String *add_string(char *str, int len) {
  int index = 0;
  String *last = &strings;
  for (String *lit = last->next; lit; last = lit, lit = lit->next) {
    if (lit->len == len && !strncmp(lit->str, str, len)) {
      return lit;
    }
    index++;
  }

  String *lit = calloc(1, sizeof(String));
  lit->str = str;
  lit->len = len;
  lit->index = index;

  return last->next = lit;
}

NamedType *find_named_type(NamedTypeKind kind, char *name, int len) {
  NamedType *last = &named_types;
  for (NamedType *it = last->next; it; last = it, it = it->next) {
    if (it->kind == kind && it->len == len && !strncmp(it->name, name, len)) {
      return it;
    }
  }

  return NULL;
}

NamedType *add_named_type(NamedTypeKind kind, char *name, int len, Type *type) {
  NamedType *last = &named_types;
  for (NamedType *it = last->next; it; last = it, it = it->next) {
    if (it->kind == kind && it->len == len && !strncmp(it->name, name, len)) {
      error("type already defined: '%.*s'", len, name);
    }
  }

  NamedType *named_type = calloc(1, sizeof(NamedType));
  named_type->kind = kind;
  named_type->name = name;
  named_type->len = len;
  named_type->type = type;

  return last->next = named_type;
}

Var *find_member(Node *node) {
  Type *type = typeof_node(node->lhs);
  if (type->ty != STRUCT) {
    error("not a struct: (%s)", type_to_string(type));
  }

  Var *member = find_var(type->members, node->ident->str, node->ident->len);
  if (member == NULL) {
    error("member not found: %.*s on (%s)", node->ident->len, node->ident->str,
          type_to_string(type));
  }
  return member;
}