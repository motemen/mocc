// 型と名前とスコープ

#include "mocc.h"

Var globals;
Var constants;
String strings;
Type defined_types;
Func funcs;

char *type_to_string(Type *type) {
  char *buf = calloc(80, sizeof(char));
  if (type->ty == TY_INT) {
    return "int";
  } else if (type->ty == TY_CHAR) {
    return "char";
  } else if (type->ty == TY_PTR) {
    snprintf(buf, 80, "ptr to %s", type_to_string(type->base));
    return buf;
  } else if (type->ty == TY_ARRAY) {
    snprintf(buf, 80, "array[%ld] of %s", type->array_size,
             type_to_string(type->base));
    return buf;
  } else if (type->ty == TY_STRUCT) {
    snprintf(buf, 80, "struct %.*s", type->name_len, type->name);
    return buf;
  } else if (type->ty == TY_ENUM) {
    snprintf(buf, 80, "enum %.*s", type->name_len, type->name);
    return buf;
  } else if (type->ty == TY_TYPEDEF) {
    snprintf(buf, 80, "typedef %.*s for %s", type->name_len, type->name,
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

static int roundup_to_word(int size) {
  return (size + 3) / 4 * 4;
}
static int roundup_to_dword(int size) {
  return (size + 7) / 8 * 8;
}

Var *add_var(Var *head, char *name, int len, Type *type, bool is_extern,
             int scope_id) {
  if (type->ty == TY_VOID) {
    error("void is not a valid type");
  }

  bool is_struct_member = head->is_struct_member;

  int offset = head->offset;
  Var *last = head;
  for (Var *var = last->next; var; last = var, var = var->next) {
    if (var->scope_id == scope_id && var->len == len &&
        !strncmp(var->name, name, len)) {
      if (var->is_extern) {
        // TODO: 型のチェックだけしたい
        continue;
      }

      error("variable already defined: '%.*s'", len, name);
    }

    offset = var->offset; //+ sizeof_type(var->type);
  }

  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->len = len;
  var->scope_id = scope_id;
  var->is_extern = is_extern;
  if (is_struct_member) {
    // ここあまり自信がない
    if (last->type != NULL) {
      offset += sizeof_type(last->type);
    }
    var->offset = roundup_to_dword(offset);
  } else {
    var->offset = offset + roundup_to_dword(sizeof_type(type));
  }
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

Type *find_defined_type(char *name, int len) {
  Type *last = &defined_types;
  for (Type *type = last->next; type; type = type->next) {
    if (type->name_len == len && strncmp(type->name, name, len) == 0 &&
        type->ty == TY_TYPEDEF) {
      return type;
    }
  }

  return NULL;
}

// 既存の型で、空のものがあったらその中身を埋めて返す
Type *add_or_find_defined_type(Type *type) {
  assert(type->ty == TY_STRUCT || type->ty == TY_ENUM ||
         type->ty == TY_TYPEDEF);
  if (type->name == NULL) {
    error("unnamed type: (%s)", type_to_string(type));
  }

  Type *last = &defined_types;
  for (Type *it = last->next; it; last = it, it = it->next) {
    if (it->name_len == type->name_len &&
        strncmp(it->name, type->name, type->name_len) == 0 &&
        it->ty == type->ty) {
      if (type->ty == TY_STRUCT && type->members != NULL) {
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