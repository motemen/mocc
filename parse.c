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

LVar *locals;
char *context; // いまみてる関数名

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
  }

  return "(unknown)";
}

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->source_pos = token->str; // ホントは一個前のトークンの位置を入れたい
  return node;
}

static Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->source_pos = token->str;
  return node;
}

LVar *find_lvar(char *context, char *name, int len) {
  assert(context != NULL);

  LVar *last_var = locals;
  for (LVar *var = locals; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len) &&
        strcmp(var->context, context) == 0) {
      return var;
    }
  }

  error("variable not found: '%.*s' in %s", len, name, context);
}

LVar *add_lvar(char *context, char *name, int len, Type *type) {
  assert(context != NULL);

  LVar *last_var = locals;
  int offset = 8;
  for (LVar *var = locals; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len) &&
        strcmp(var->context, context) == 0) {
      error("variable already defined: '%.*s'", len, name);
    }
    if (strcmp(var->context, context) == 0) {
      if (var->type->ty == ARRAY) {
        offset += 8 * var->type->array_size;
      } else {
        offset += 8;
      }
    }
  }

  LVar *var = calloc(1, sizeof(LVar));
  var->name = name;
  var->len = len;
  var->context = context;
  var->offset = offset;
  var->type = type;

  if (last_var) {
    last_var->next = var;
  } else {
    locals = var;
  }

  return var;
}

///// Parser /////

// Syntax:
//    program     = funcdecl*
//    funcdecl    = "int" ident "(" "int" expr ("," "int" expr)* ")"
//                  "{" stmt* "}"
//    type        = "int" "*"*
//    stmt        = expr ";"
//                | "return" expr ";"
//                | "if" "(" expr ")" stmt ("else" stmt)?
//                | "while" "(" expr ")" stmt
//                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                | "{" stmt* "}"
//                | vardecl ";"
//    vardecl     = type ident ("[" num "]")?
//    expr        = assign
//    assign      = equality ("=" assign)?
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? primary
//                | "*" unary
//                | "&" unary
//                | "sizeof" unary
//    primary     = num | ident ("(" (expr ("," expr)*)? ")")? | "(" expr ")"
//    ident	      = /[a-z][a-z0-9]*/
//    num         = [0-9]+

Node *parse_expr();

bool parsing_sizeof_or_addr = false;

static Node *parse_primary() {
  if (token_consume_reserved("(")) {
    Node *node = parse_expr();
    token_expect(")");
    return node;
  }

  // parse_ident 相当
  Token *tok = token_consume_ident();
  if (tok != NULL) {
    if (token_consume_reserved("(")) {
      // 関数呼び出しだった

      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_CALL;
      node->name = tok->str;
      node->name_len = tok->len;
      node->source_pos = tok->str;
      node->source_len = tok->len;

      if (token_consume_reserved(")")) {
        // 引数ナシ
      } else {
        NodeList head = {};
        NodeList *cur = &head;
        while (true) {
          NodeList *node_item = calloc(1, sizeof(NodeList));
          node_item->node = parse_expr();
          cur->next = node_item;
          cur = cur->next;

          if (token_consume_reserved(")")) {
            break;
          }

          token_expect(",");
        }

        node->nodes = head.next;
      }

      return node;
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(context, tok->str, tok->len);
    node->lvar = lvar;
    node->source_pos = tok->str;
    node->source_len = tok->len;

    if (lvar->type->ty == ARRAY && !parsing_sizeof_or_addr) {
      // 添字なしの配列のときは配列へのポインタを返す
      return new_node(ND_ADDR, node, NULL);
      // TODO: sizeof と & の場合は別の対応する
      // TODO: 添字ありの場合はまだ実装してない
      // と思ったけど a[3] を *(a+3) にするんなら特に対応いらんのかな
    }

    return node;
  }

  int val = token_expect_number();
  return new_node_num(val);
}

Type int_type = {INT, NULL};

static Type *new_type_ptr_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = PTR;
  type->ptr_to = base;
  return type;
}

static Type *new_type_array_of(Type *base, int size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = ARRAY;
  type->ptr_to = base;
  type->array_size = size;
  return type;
}

static Type *inspect_type(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    return &int_type;

  case ND_ADD:
  case ND_SUB: {
    Type *ltype = inspect_type(node->lhs);
    Type *rtype = inspect_type(node->rhs);

    if (ltype->ty == INT && rtype->ty == INT) {
      return &int_type;
    }
    if (ltype->ty == PTR && rtype->ty == INT) {
      return ltype;
    }
    if (ltype->ty == INT && rtype->ty == PTR) {
      return rtype;
    }

    error_at(node->source_pos, "invalid or unimplemented pointer arithmetic");
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
    Type *type = inspect_type(node->lhs);
    if (type->ty == PTR) {
      // これはこれでいいのか…？
      // int arr[10] なとき *arr は *(<addr of arr>) みたいにパーズするので
      // 普通にやると *a が int[10] という型に見えちゃうけど
      // ここは int を返したい…わけです
      if (type->ptr_to->ty == ARRAY) {
        return type->ptr_to->ptr_to;
      }
      return type->ptr_to;
    }

    error_at(node->source_pos, "invalid dereference (or not implemented)");
  }

  case ND_ADDR:
    return new_type_ptr_to(inspect_type(node->lhs));

  default:
    error_at(node->source_pos, "sizeof: unimplemented");
  }
}

int sizeof_type(Type *type) {
  switch (type->ty) {
  case INT:
    return 4;
  case PTR:
    return 8;
  case ARRAY:
    return type->array_size * sizeof_type(type->ptr_to);
  }
}

static Node *parse_unary() {
  if (token_consume_reserved("+")) {
    parsing_sizeof_or_addr = false;
    return parse_primary();
  }

  if (token_consume_reserved("-")) {
    parsing_sizeof_or_addr = false;
    return new_node(ND_SUB, new_node_num(0), parse_primary());
  }

  if (token_consume_reserved("*")) {
    parsing_sizeof_or_addr = false;
    return new_node(ND_DEREF, parse_unary(), NULL);
  }

  if (token_consume_reserved("&")) {
    parsing_sizeof_or_addr = true;
    Node *node = parse_unary();
    parsing_sizeof_or_addr = false;
    return new_node(ND_ADDR, node, NULL);
  }

  if (token_consume(TK_SIZEOF)) {
    parsing_sizeof_or_addr = true;
    Node *node = parse_unary();
    // 式全体の型とかいうやつを知りたいですなあ
    Type *type = inspect_type(node);
    int size = sizeof_type(type);
    parsing_sizeof_or_addr = false;
    return new_node_num(size);
  }

  return parse_primary();
}

static Node *parse_mul() {
  Node *node = parse_unary();
  for (;;) {
    parsing_sizeof_or_addr = false;
    if (token_consume_reserved("*")) {
      node = new_node(ND_MUL, node, parse_unary());
    } else if (token_consume_reserved("/")) {
      node = new_node(ND_DIV, node, parse_unary());
    } else {
      return node;
    }
  }
}

static Node *parse_add() {
  Node *node = parse_mul();
  for (;;) {
    parsing_sizeof_or_addr = false;
    if (token_consume_reserved("+")) {
      node = new_node(ND_ADD, node, parse_mul());
    } else if (token_consume_reserved("-")) {
      node = new_node(ND_SUB, node, parse_mul());
    } else {
      return node;
    }
  }
}

static Node *parse_relational() {
  Node *node = parse_add();
  for (;;) {
    parsing_sizeof_or_addr = false;
    if (token_consume_reserved("<")) {
      node = new_node(ND_LT, node, parse_add());
    } else if (token_consume_reserved(">")) {
      node = new_node(ND_LT, parse_add(), node);
    } else if (token_consume_reserved("<=")) {
      node = new_node(ND_GE, parse_add(), node);
    } else if (token_consume_reserved(">=")) {
      node = new_node(ND_GE, node, parse_add());
    } else {
      return node;
    }
  }
}

static Node *parse_equality() {
  Node *node = parse_relational();
  for (;;) {
    parsing_sizeof_or_addr = false;
    if (token_consume_reserved("==")) {
      node = new_node(ND_EQ, node, parse_relational());
    } else if (token_consume_reserved("!=")) {
      node = new_node(ND_NE, node, parse_relational());
    } else {
      return node;
    }
  }
}

static Node *parse_assign() {
  Node *node = parse_equality();
  if (token_consume_reserved("=")) {
    parsing_sizeof_or_addr = false;
    node = new_node(ND_ASSIGN, node, parse_assign());
  }
  return node;
}

Node *parse_expr() { return parse_assign(); }

Node *parse_stmt();

Node *parse_block() {
  if (token_consume_reserved("{")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    node->source_pos = token->str;
    node->source_len = token->len;

    NodeList head = {};
    NodeList *cur = &head;
    while (!token_consume_reserved("}")) {
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
  if (token_consume_type("int")) {
    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;

    while (token_consume_reserved("*")) {
      Type *type_p = calloc(1, sizeof(Type));
      type_p->ty = PTR;
      type_p->ptr_to = type;

      type = type_p;
    }

    return type;
  }

  return NULL;
}

Node *parse_stmt() {
  if (token_consume(TK_RETURN)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->source_pos = token->str;
    node->source_len = token->len;
    node->lhs = parse_expr();
    token_expect(";");
    return node;
  }

  if (token_consume(TK_IF)) {
    token_expect("(");
    Node *expr = parse_expr();
    token_expect(")");
    Node *stmt = parse_stmt();

    Node *else_stmt = NULL;
    if (token_consume(TK_ELSE)) {
      else_stmt = parse_stmt();
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    node->lhs = expr;
    node->rhs = stmt;
    node->node3 = else_stmt;
    node->source_pos = expr->source_pos;
    node->source_len = expr->source_len; // ではないのだが FIXME
    return node;
  }

  if (token_consume(TK_WHILE)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    node->source_pos = token->str;
    node->source_len = token->len; // ちがうけど…
    token_expect("(");
    node->lhs = parse_expr();
    token_expect(")");
    node->rhs = parse_stmt();

    return node;
  }

  if (token_consume(TK_FOR)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    node->source_pos = token->str;
    node->source_len = token->len; // ちがうけど…
    token_expect("(");

    if (!token_consume_reserved(";")) {
      node->lhs = parse_expr();
      token_expect(";");
    }

    if (!token_consume_reserved(";")) {
      node->rhs = parse_expr();
      token_expect(";");
    }

    if (!token_consume_reserved(")")) {
      node->node3 = parse_expr();
      token_expect(")");
    }

    if (!token_consume_reserved(";")) {
      node->node4 = parse_stmt();
    }

    return node;
  }

  // parse_vardecl
  Type *type = parse_type();
  if (type) {
    Token *tok_var = token_consume_ident();
    if (!tok_var) {
      error("expected variable name");
    }

    if (token_consume_reserved("[")) {
      int size = token_expect_number();
      token_expect("]");

      type = new_type_array_of(type, size);
    }

    LVar *lvar = add_lvar(context, tok_var->str, tok_var->len, type);

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VARDECL;
    node->lvar = lvar;
    node->source_pos = tok_var->str;
    node->source_len = tok_var->len;

    token_expect(";");

    return node;
  }

  Node *block = parse_block();
  if (block) {
    return block;
  }

  Node *node = parse_expr();
  token_expect(";");
  return node;
}

Node *parse_funcdecl() {
  if (!token_consume(TK_TYPE)) {
    error("TK_TYPE expected");
  }

  Token *ident = token_consume_ident();
  if (!ident) {
    error("expected ident");
  }

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNCDECL;
  node->name = ident->str;
  node->name_len = ident->len;
  node->source_pos = ident->str;
  node->source_len = ident->len;

  context = strndup(ident->str, ident->len);

  token_expect("(");

  if (token_consume_reserved(")")) {
    // 引数ナシ
  } else {
    NodeList head = {};
    NodeList *cur = &head;
    while (true) {
      Type *type = parse_type();
      if (!type) {
        error("type expected");
      }

      Token *tok = token_consume_ident();
      if (!tok) {
        error("expected ident");
      }

      Node *ident = calloc(1, sizeof(Node));
      ident->kind = ND_LVAR;
      LVar *lvar = add_lvar(context, tok->str, tok->len, type);
      ident->lvar = lvar;
      ident->source_pos = tok->str;
      ident->source_len = tok->len;

      NodeList *node_item = calloc(1, sizeof(NodeList));
      node_item->node = ident;
      cur->next = node_item;
      cur = cur->next;

      if (token_consume_reserved(")")) {
        break;
      }

      token_expect(",");
    }

    node->args = head.next;
  }

  Node *block = parse_block();
  if (!block) {
    error("expected block");
  }

  node->nodes = block->nodes;

  return node;
}

Node *code[100];

void parse_program() {
  int i = 0;
  while (!token_at_eof()) {
    code[i++] = parse_funcdecl();
  }
  code[i] = NULL;
}