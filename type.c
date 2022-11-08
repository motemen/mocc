// 型と名前とスコープ

#include "parse.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LVar locals_head;
GVar globals_head;
StrLit str_lits_head;
NamedType named_types_head;

char *type_to_str(Type *type) {
  char *buf = calloc(100, sizeof(char));
  if (type->ty == INT) {
    return "int";
  } else if (type->ty == PTR) {
    snprintf(buf, 100, "ptr to %s", type_to_str(type->base));
    return buf;
  } else if (type->ty == ARRAY) {
    snprintf(buf, 100, "array[%ld] of %s", type->array_size,
             type_to_str(type->base));
    return buf;
  }

  return "(unknown)";
}

LVar *find_lvar(LVar *head, Node *scope, char *name, int len) {
  for (LVar *var = head->next; var; var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len) &&
        var->scope == scope) {
      return var;
    }
  }

  return NULL;
}

// locals を共有するために scope を置いてたけど、head を切り替える（そもそも
// scope ごとに locals を持つ） ことで不要になる予感
LVar *add_lvar(LVar *head, Node *scope, char *name, int len, Type *type) {
  int offset = 8;

  LVar *last = head;
  for (LVar *var = last->next; var; last = var, var = var->next) {
    if (var->scope == scope) {
      if (var->len == len && !strncmp(var->name, name, len))
        error("variable already defined: '%.*s'", len, name);

      offset = var->offset;
    }
  }

  LVar *var = calloc(1, sizeof(LVar));
  var->name = name;
  var->len = len;
  var->scope = scope;
  var->offset = offset + (sizeof_type(type) + 7) / 8 * 8;
  var->type = type;

  last->next = var;

  return var;
}

GVar *find_gvar(char *name, int len) {
  for (GVar *var = globals_head.next; var; var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      return var;
    }
  }

  return NULL;
}

GVar *add_gvar(char *name, int len, Type *type) {
  GVar *last_var = NULL;
  for (GVar *var = globals_head.next; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      error("global variable already defined: '%.*s'", len, name);
    }
  }

  GVar *var = calloc(1, sizeof(GVar));
  var->name = name;
  var->len = len;
  var->type = type;

  if (last_var) {
    last_var->next = var;
  } else {
    globals_head.next = var;
  }

  return var;
}

StrLit *add_str_lit(char *str, int len) {
  int index = 0;
  StrLit *last = NULL;
  for (StrLit *lit = str_lits_head.next; lit; last = lit, lit = lit->next) {
    if (lit->len == len && !strncmp(lit->str, str, len)) {
      return lit;
    }
    index++;
  }

  StrLit *lit = calloc(1, sizeof(StrLit));
  lit->str = str;
  lit->len = len;
  lit->index = index;

  if (last) {
    last->next = lit;
  } else {
    str_lits_head.next = lit;
  }

  return lit;
}

NamedType *find_named_type(NamedTypeKind kind, char *name, int len) {
  NamedType *last = &named_types_head;
  for (NamedType *it = last->next; it; last = it, it = it->next) {
    if (it->kind == kind && it->len == len && !strncmp(it->name, name, len)) {
      return it;
    }
  }

  return NULL;
}

NamedType *add_named_type(NamedTypeKind kind, char *name, int len, Type *type) {
  NamedType *last = &named_types_head;
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
