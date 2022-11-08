// 型と名前とスコープ

#include "mocc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Var globals;
String strings;
Type defined_types;

char *type_to_string(Type *type) {
  char *buf = calloc(80, sizeof(char));
  if (type->ty == TY_INT) {
    return "int";
  } else if (type->ty == TY_PTR) {
    snprintf(buf, 80, "ptr to %s", type_to_string(type->base));
    return buf;
  } else if (type->ty == TY_ARRAY) {
    snprintf(buf, 80, "array[%ld] of %s", type->array_size,
             type_to_string(type->base));
    return buf;
  }

  return "(unknown)";
}

Var *find_var_in_curr_scope(char *name, int len) {
  for (Scope *scope = curr_scope; scope; scope = scope->parent) {
    Var *var = find_var(scope->node->locals, name, len);
    if (var != NULL) {
      return var;
    }
  }

  return NULL;
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
  int offset = head->offset;
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

Type *add_or_find_defined_type(Type *type) {
  assert(type->ty == TY_STRUCT); // TODO: TYPEDEF とかもくる予定
  assert(type->name != NULL);

  Type *last = &defined_types;
  for (Type *it = last->next; it; last = it, it = it->next) {
    if (it->name_len == type->name_len &&
        strncmp(it->name, type->name, type->name_len) == 0) {
      if (type->members != NULL) {
        if (it->members != NULL) {
          error("type already defined: '%.*s'", type->name_len, type->name);
        }
        it->members = type->members;
      }
      return it;
    }
  }

  return last->next = type;
}

Var *find_member(Node *node) {
  Type *type = typeof_node(node->lhs);
  if (type->ty != TY_STRUCT) {
    error("not a struct: (%s)", type_to_string(type));
  }

  Var *member = find_var(type->members, node->ident->str, node->ident->len);
  if (member == NULL) {
    error("member not found: %.*s on (%s)", node->ident->len, node->ident->str,
          type_to_string(type));
  }
  return member;
}