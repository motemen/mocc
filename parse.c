#include "parse.h"
#include "9cv.h"
#include "tokenize.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LVar locals_head;
GVar globals_head;
StrLit str_lits_head;
NamedType named_types_head;

// 現在のローカル変数のスコープ
// いまのとこ ND_FUNCDECL のみ
// 必要になったら Scope にする
Node *curr_func_scope;

int label_index = 0;

char *node_kind_to_str(NodeKind kind) {
  switch (kind) {
  case ND_NUM:
    return "ND_NUM";
  case ND_LT:
    return "ND_LT";
  case ND_GE:
    return "ND_GE";
  case ND_ADD:
    return "ND_ADD";
  case ND_SUB:
    return "ND_SUB";
  case ND_MUL:
    return "ND_MUL";
  case ND_DIV:
    return "ND_DIV";
  case ND_EQ:
    return "ND_EQ";
  case ND_NE:
    return "ND_NE";
  case ND_LVAR:
    return "ND_LVAR";
  case ND_ASSIGN:
    return "ND_ASSIGN";
  case ND_RETURN:
    return "ND_RETURN";
  case ND_IF:
    return "ND_IF";
  case ND_WHILE:
    return "ND_WHILE";
  case ND_FOR:
    return "ND_FOR";
  case ND_BLOCK:
    return "ND_BLOCK";
  case ND_CALL:
    return "ND_CALL";
  case ND_FUNCDECL:
    return "ND_FUNCDECL";
  case ND_DEREF:
    return "ND_DEREF";
  case ND_ADDR:
    return "ND_ADDR";
  case ND_VARDECL:
    return "ND_VARDECL";
  case ND_GVARDECL:
    return "ND_GVARDECL";
  case ND_GVAR:
    return "ND_GVAR";
  case ND_STRING:
    return "ND_STRING";
  case ND_LOGOR:
    return "ND_LOGOR";
  case ND_BREAK:
    return "ND_BREAK";
  case ND_CONTINUE:
    return "ND_CONTINUE";
  case ND_NOP:
    return "ND_NOP";
  }

  return "(unknown)";
}

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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->source_pos = prev_token->str;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM, NULL, NULL);
  node->val = val;
  return node;
}

static LVar *find_lvar(LVar *head, Node *scope, char *name, int len) {
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
static LVar *add_lvar(LVar *head, Node *scope, char *name, int len,
                      Type *type) {
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

static GVar *find_gvar(char *name, int len) {
  for (GVar *var = globals_head.next; var; var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      return var;
    }
  }

  return NULL;
}

static GVar *add_gvar(char *name, int len, Type *type) {
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

static StrLit *add_str_lit(char *str, int len) {
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

static NamedType *find_named_type(NamedTypeKind kind, char *name, int len) {
  NamedType *last = &named_types_head;
  for (NamedType *it = last->next; it; last = it, it = it->next) {
    if (it->kind == kind && it->len == len && !strncmp(it->name, name, len)) {
      return it;
    }
  }

  return NULL;
}

static NamedType *add_named_type(NamedTypeKind kind, char *name, int len,
                                 Type *type) {
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

///// Parser /////

// Syntax:
//    program     = (funcdecl | vardecl)*
//    funcdecl    = type ident "(" type expr ("," type expr)* ")"
//                  "{" stmt* "}"
//    type        = ("int" | "char") "*"*
//                | ident
//                | "struct" ident ("{" (type ident ";")* "}")?
//    stmt        = expr ";"
//                | "return" expr ";"
//                | "if" "(" expr ")" stmt ("else" stmt)?
//                | "while" "(" expr ")" stmt
//                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                | "{" stmt* "}"
//                | "break" ";"
//                | vardecl ";"
//    vardecl     = type ident ("[" num "]")? ("=" assign | initializer)?
//    expr        = assign
//    assign      = or ("=" assign)?
//    or          = equality ("||" equality)*
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? postfix
//                | "*" unary
//                | "&" unary
//                | "sizeof" unary
//                | postfix
//    postfix     = primary ("[" expr "]")*
//    primary     = num | ident ("(" (expr ("," expr)*)? ")")? | "(" expr ")"
//                | string
//    ident	      = /[a-z][a-z0-9]*/
//    num         = [0-9]+
//    string      = /"[^"]*"/

Node *parse_expr();

static Node *parse_primary() {
  if (token_consume_punct("(")) {
    Node *node = parse_expr();
    token_expect_punct(")");
    return node;
  }

  // parse_ident 相当
  Token *tok = token_consume(TK_IDENT);
  if (tok != NULL) {
    if (token_consume_punct("(")) {
      // 関数呼び出しだった

      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_CALL;
      node->ident = tok;
      node->source_pos = tok->str;
      node->source_len = tok->len;

      if (token_consume_punct(")")) {
        // 引数ナシ
      } else {
        NodeList head = {};
        NodeList *cur = &head;
        while (true) {
          NodeList *node_item = calloc(1, sizeof(NodeList));
          node_item->node = parse_expr();
          cur->next = node_item;
          cur = cur->next;

          if (token_consume_punct(")")) {
            break;
          }

          token_expect_punct(",");
        }

        node->nodes = head.next;
      }

      return node;
    }

    LVar *lvar = find_lvar(&locals_head, curr_func_scope, tok->str, tok->len);
    if (lvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_LVAR;
      node->lvar = lvar;
      node->source_pos = tok->str;
      node->source_len = tok->len;

      // ここで生の配列を見つけた場合は &a を返す、というコードを書いていたが
      // int a[100]; のとき a は int へのポインタ int * だが
      // &a は int[100] へのポインタ int (*)[100] になり
      // 別物なのでやめた

      return node;
    }

    GVar *gvar = find_gvar(tok->str, tok->len);
    if (gvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_GVAR;
      node->gvar = gvar;
      node->source_pos = tok->str;
      node->source_len = tok->len;
      return node;
    }

    error("variable not found: '%.*s'", tok->len, tok->str);
  }

  Token *tok_str = token_consume(TK_STRING);
  if (tok_str) {
    StrLit *str_lit = add_str_lit(tok_str->str, tok_str->len);
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_STRING;
    node->val = str_lit->index;
    node->source_pos = tok_str->str;
    node->source_len = tok_str->len;
    return node;
  }

  int val = token_expect_number();
  return new_node_num(val);
}

Type int_type = {INT, NULL};
Type char_type = {CHAR, NULL};

static Type *new_type_ptr_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = PTR;
  type->base = base;
  return type;
}

static Type *new_type_array_of(Type *base, int size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = ARRAY;
  type->base = base;
  type->array_size = size;
  return type;
}

Type *typeof_node(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    return &int_type;

  case ND_ADD:
  case ND_SUB: {
    Type *ltype = typeof_node(node->lhs);
    Type *rtype = typeof_node(node->rhs);

    if (ltype->ty == ARRAY)
      ltype = new_type_ptr_to(ltype->base);
    if (rtype->ty == ARRAY)
      rtype = new_type_ptr_to(rtype->base);

    if (ltype->ty == INT && rtype->ty == INT) {
      return &int_type;
    }
    if (ltype->ty == PTR && rtype->ty == INT) {
      return ltype;
    }
    if (ltype->ty == INT && rtype->ty == PTR) {
      return rtype;
    }

    error_at(node->source_pos,
             "invalid or unimplemented pointer arithmetic: %s and %s",
             type_to_str(ltype), type_to_str(rtype));
  }

  case ND_MUL:
  case ND_DIV:
    return &int_type;

  case ND_CALL:
    // TODO: 関数の戻り値の型を返す
    return &int_type;

  case ND_LVAR:
    return node->lvar->type;

  case ND_DEREF: {
    Type *type = typeof_node(node->lhs);
    if (type->ty == PTR || type->ty == ARRAY) {
      return type->base;
    }

    error_at(node->source_pos, "invalid dereference (or not implemented)");
  }

  case ND_ADDR:
    return new_type_ptr_to(typeof_node(node->lhs));

  case ND_GVAR:
    return node->gvar->type;

  default:
    error_at(node->source_pos, "typeof_node: unimplemented: %s",
             node_kind_to_str(node->kind));
  }
}

int sizeof_type(Type *type) {
  switch (type->ty) {
  case CHAR:
    return 1;
  case INT:
    return 4;
  case PTR:
    return 8;
  case ARRAY:
    return type->array_size * sizeof_type(type->base);
  case STRUCT: {
    LVar *m = type->members;
    while (m && m->next)
      m = m->next;
    return m ? m->offset + sizeof_type(m->type) : 0;
  }
  }
}

static Node *parse_postfix() {
  Node *node = parse_primary();

  while (token_consume_punct("[")) {
    Node *expr = parse_expr();
    token_expect_punct("]");
    node = new_node(ND_DEREF, new_node(ND_ADD, node, expr), NULL);
  }

  return node;
}

static Node *parse_unary() {
  if (token_consume_punct("+")) {
    return parse_postfix();
  }

  if (token_consume_punct("-")) {
    return new_node(ND_SUB, new_node_num(0), parse_primary());
  }

  if (token_consume_punct("*")) {
    return new_node(ND_DEREF, parse_unary(), NULL);
  }

  if (token_consume_punct("&")) {
    Node *node = parse_unary();
    return new_node(ND_ADDR, node, NULL);
  }

  if (token_consume(TK_SIZEOF) != NULL) {
    Node *node = parse_unary();
    Type *type = typeof_node(node);
    int size = sizeof_type(type);
    return new_node_num(size);
  }

  return parse_postfix();
}

static Node *parse_mul() {
  Node *node = parse_unary();
  for (;;) {
    if (token_consume_punct("*")) {
      node = new_node(ND_MUL, node, parse_unary());
    } else if (token_consume_punct("/")) {
      node = new_node(ND_DIV, node, parse_unary());
    } else {
      return node;
    }
  }
}

static Node *parse_add() {
  Node *node = parse_mul();
  for (;;) {
    if (token_consume_punct("+")) {
      node = new_node(ND_ADD, node, parse_mul());
    } else if (token_consume_punct("-")) {
      node = new_node(ND_SUB, node, parse_mul());
    } else {
      return node;
    }
  }
}

static Node *parse_relational() {
  Node *node = parse_add();
  for (;;) {
    if (token_consume_punct("<")) {
      node = new_node(ND_LT, node, parse_add());
    } else if (token_consume_punct(">")) {
      node = new_node(ND_LT, parse_add(), node);
    } else if (token_consume_punct("<=")) {
      node = new_node(ND_GE, parse_add(), node);
    } else if (token_consume_punct(">=")) {
      node = new_node(ND_GE, node, parse_add());
    } else {
      return node;
    }
  }
}

static Node *parse_equality() {
  Node *node = parse_relational();
  for (;;) {
    if (token_consume_punct("==")) {
      node = new_node(ND_EQ, node, parse_relational());
    } else if (token_consume_punct("!=")) {
      node = new_node(ND_NE, node, parse_relational());
    } else {
      return node;
    }
  }
}

static Node *parse_or() {
  Node *node = parse_equality();
  for (;;) {
    if (token_consume_punct("||")) {
      node = new_node(ND_LOGOR, node, parse_equality());
    } else {
      return node;
    }
  }
}

static Node *parse_assign() {
  Node *node = parse_or();
  if (token_consume_punct("=")) {
    node = new_node(ND_ASSIGN, node, parse_assign());
  }
  return node;
}

Node *parse_expr() { return parse_assign(); }

Node *parse_stmt();

Node *parse_block() {
  if (token_consume_punct("{")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;

    NodeList head = {};
    NodeList *cur = &head;
    while (!token_consume_punct("}")) {
      NodeList *node_item = calloc(1, sizeof(NodeList));
      node_item->node = parse_stmt();
      cur->next = node_item;
      cur = cur->next;
    }

    node->nodes = head.next;
    return node;
  }

  return NULL;
}

Type *parse_type() {
  Type *type = calloc(1, sizeof(Type));

  if (token_consume_type("int")) {
    type->ty = INT;
  } else if (token_consume_type("char")) {
    type->ty = CHAR;
  } else if (token_consume(TK_STRUCT)) {
    type->ty = STRUCT;
    Token *name = token_consume(TK_IDENT);
    if (token_consume_punct("{")) {
      if (token_consume_punct("}")) {
        // nop
      } else {
        // ND_FUNCDECL の場合と似てるかも
        type->members = calloc(1, sizeof(LVar *));
        while (true) {
          Type *member_type = parse_type();
          if (!member_type) {
            error("expected member type");
          }

          Token *member_name = token_consume(TK_IDENT);
          if (!member_name) {
            error("expected member name");
          }

          add_lvar(type->members, NULL, member_name->str, member_name->len,
                   member_type);

          token_expect_punct(";");

          if (token_consume_punct("}")) {
            break;
          }
        }

        if (name != NULL) {
          add_named_type(NT_STRUCT, name->str, name->len, type);
        }
      }
    } else if (name != NULL) {
      // type = "struct A" という感じの
      // 既存の構造体を参照している
      NamedType *named_type = find_named_type(NT_STRUCT, name->str, name->len);
      if (named_type == NULL) {
        error("struct %.*s is not defined", name->len, name->str);
      }
      type = named_type->type;
    } else {
      // type = "struct" という感じでおかしい
      error("either struct name nor members not given");
    }
  } else {
    return NULL;
  }

  while (token_consume_punct("*")) {
    Type *type_p = calloc(1, sizeof(Type));
    type_p->ty = PTR;
    type_p->base = type;

    type = type_p;
  }

  return type;
}

Node *parse_stmt() {
  if (token_consume(TK_RETURN) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    node->lhs = parse_expr();
    token_expect_punct(";");
    return node;
  }

  if (token_consume(TK_IF) != NULL) {
    token_expect_punct("(");
    Node *expr = parse_expr();
    token_expect_punct(")");
    Node *stmt = parse_stmt();

    Node *else_stmt = NULL;
    if (token_consume(TK_ELSE) != NULL) {
      else_stmt = parse_stmt();
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    node->label_index = ++label_index;
    node->lhs = expr;
    node->rhs = stmt;
    node->node3 = else_stmt;
    node->source_pos = expr->source_pos;
    node->source_len = expr->source_len;
    return node;
  }

  if (token_consume(TK_WHILE) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    node->label_index = ++label_index;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct("(");
    node->lhs = parse_expr();
    token_expect_punct(")");
    node->rhs = parse_stmt();

    return node;
  }

  if (token_consume(TK_FOR) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    node->label_index = ++label_index;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct("(");

    if (!token_consume_punct(";")) {
      node->lhs = parse_expr();
      token_expect_punct(";");
    }

    if (!token_consume_punct(";")) {
      node->rhs = parse_expr();
      token_expect_punct(";");
    }

    if (!token_consume_punct(")")) {
      node->node3 = parse_expr();
      token_expect_punct(")");
    }

    if (!token_consume_punct(";")) {
      node->node4 = parse_stmt();
    }

    return node;
  }

  if (token_consume(TK_BREAK) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BREAK;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct(";");
    return node;
  }

  if (token_consume(TK_CONTINUE) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CONTINUE;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct(";");
    return node;
  }

  // parse_vardecl
  Type *type = parse_type();
  if (type) {
    Token *tok_var = token_consume(TK_IDENT);
    if (!tok_var) {
      error("expected variable name");
    }

    while (token_consume_punct("[")) {
      int size = token_expect_number();
      token_expect_punct("]");

      type = new_type_array_of(type, size);
    }

    LVar *lvar = add_lvar(&locals_head, curr_func_scope, tok_var->str,
                          tok_var->len, type);

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VARDECL;
    node->lvar = lvar;
    node->source_pos = tok_var->str;
    node->source_len = tok_var->len;

    if (token_consume_punct("=")) {
      node->rhs = parse_expr();
    }

    token_expect_punct(";");

    return node;
  }

  Node *block = parse_block();
  if (block) {
    return block;
  }

  Node *node = parse_expr();
  token_expect_punct(";");
  return node;
}

static Node *parse_decl() {
  Type *type = parse_type();
  if (!type) {
    error("expected typezo");
  }

  Token *ident = token_consume(TK_IDENT);
  if (!ident) {
    // struct S { int i; }; など識別子が登場しない場合、たぶん型の宣言
    //
    if (type->ty == STRUCT) {
      token_expect_punct(";");

      // じつはここでもう処理は済んでしまっている
      // 適当に無害な Node を作って返す
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_NOP;
      return node;
    } else {
      error("expected declaration name");
    }
  }

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNCDECL;
  node->ident = ident;
  node->source_pos = ident->str;
  node->source_len = ident->len;

  curr_func_scope = node;

  if (token_consume_punct("(")) {
    if (token_consume_punct(")")) {
      // 引数ナシ
    } else {
      NodeList head = {};
      NodeList *cur = &head;
      while (true) {
        Type *type = parse_type();
        if (!type) {
          error("expected argument type");
        }

        Token *tok = token_consume(TK_IDENT);
        if (!tok) {
          error("expected argument name");
        }

        Node *ident = calloc(1, sizeof(Node));
        ident->kind = ND_LVAR;
        LVar *lvar =
            add_lvar(&locals_head, curr_func_scope, tok->str, tok->len, type);
        ident->lvar = lvar;
        ident->source_pos = tok->str;
        ident->source_len = tok->len;

        NodeList *node_item = calloc(1, sizeof(NodeList));
        node_item->node = ident;
        cur->next = node_item;
        cur = cur->next;

        if (token_consume_punct(")")) {
          break;
        }

        token_expect_punct(",");
      }

      node->args = head.next;
    }

    // TODO: 関数の型を～～

    Node *block = parse_block();
    if (!block) {
      error("expected block");
    }

    node->nodes = block->nodes;

    curr_func_scope = NULL;

    return node;
  }

  // こちらからグローバル変数になります
  node->kind = ND_GVARDECL;

  while (token_consume_punct("[")) {
    // TODO: ここが省略できることもある
    int size = token_expect_number();
    token_expect_punct("]");

    type = new_type_array_of(type, size);
  }

  if (token_consume_punct("=")) {
    // TODO: 文字列リテラルの場合は {} に読み替えることにする
    if (token_consume_punct("{")) {
      if (token_consume_punct("}")) {
        // 空の場合はゼロ初期化と同じなので何もしない
      } else {
        // ここは ND_CALL とよく似てるので parse_expr_list
        // とかにできるとよさそう
        NodeList head = {};
        NodeList *cur = &head;
        while (true) {
          NodeList *node_item = calloc(1, sizeof(NodeList));
          node_item->node = parse_expr();
          cur->next = node_item;
          cur = cur->next;

          if (token_consume_punct("}")) {
            break;
          }

          token_expect_punct(",");
        }

        node->nodes = head.next;
      }
    } else {
      node->rhs = parse_expr();
    }
  }

  token_expect_punct(";");

  GVar *gvar = add_gvar(ident->str, ident->len, type);
  node->gvar = gvar;

  return node;
}

Node *code[100]; // FIXME

void parse_program() {
  int i = 0;
  while (!token_at_eof()) {
    code[i++] = parse_decl();
  }
  code[i] = NULL;

  if (curr_token != NULL && curr_token->kind != TK_EOF) {
    error("not all tokens are consumed");
  }
}