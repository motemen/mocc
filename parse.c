#include "9cv.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *token;
LVar *locals;

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
  }
}

// Consumes a token matching op operator
static bool token_consume(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

bool token_at_eof() { return token->kind == TK_EOF; }

static void token_expect(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    error("token_expect: not '%c'", *op);
  }

  token = token->next;
}

static int token_expect_number() {
  if (token->kind != TK_NUM) {
    error("not a number");
  }

  int val = token->val;
  token = token->next;
  return val;
}

static Token *token_consume_ident() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Token *tok = token;
  token = token->next;
  return tok;
}

static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;

  cur->next = tok;
  return tok;
}

static Token *new_token_num(Token *cur, int val) {
  Token *tok = new_token(TK_NUM, cur, NULL, 0);
  tok->val = val;
  return tok;
}

bool isident(char ch) { return isalnum(ch) || ch == '_'; }

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0 ||
        strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (('a' <= *p && *p <= 'z') || *p == '_') {
      int n = 1;
      while (*(p + n) && isident(*(p + n))) {
        n++;
      }

      cur = new_token(TK_IDENT, cur, p, n);
      p += n;

      continue;
    }

    if (isdigit(*p)) {
      cur = new_token_num(cur, strtol(p, &p, 10));
      continue;
    }

    error("cannot tokenize");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
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

LVar *find_lvar(char *name, int len) {
  LVar *last_var = locals;
  for (LVar *var = locals; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      return var;
    }
  }
  return NULL;
}

LVar *find_or_add_lvar(char *name, int len) {
  LVar *last_var = locals;
  int i;
  for (LVar *var = locals; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len)) {
      return var;
    }
    i++;
  }

  LVar *var = calloc(1, sizeof(LVar));
  var->name = name;
  var->len = len;
  var->offset = (i + 1) * 8;

  if (last_var) {
    last_var->next = var;
  } else {
    locals = var;
  }

  return var;
}

///// Parser /////

// syntax:
//    expr        = assign
//    assign      = equality ("=" assign)?
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? primary
//    primary     = num | ident | "(" expr ")"
//    ident	      = /[a-z][a-z0-9]*/

static Node *parse_primary() {
  if (token_consume("(")) {
    Node *node = parse_expr();
    token_expect(")");
    return node;
  }

  // parse_ident 相当
  Token *tok = token_consume_ident();
  if (tok != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar *lvar = find_or_add_lvar(tok->str, tok->len);
    node->lvar = lvar;
    node->source_pos = tok->str;
    node->source_len = tok->len;
    return node;
  }

  int val = token_expect_number();
  return new_node_num(val);
}

static Node *parse_unary() {
  if (token_consume("+")) {
    return parse_primary();
  }

  if (token_consume("-")) {
    return new_node(ND_SUB, new_node_num(0), parse_primary());
  }

  return parse_primary();
}

static Node *parse_mul() {
  Node *node = parse_unary();
  for (;;) {
    if (token_consume("*")) {
      node = new_node(ND_MUL, node, parse_unary());
    } else if (token_consume("/")) {
      node = new_node(ND_DIV, node, parse_unary());
    } else {
      return node;
    }
  }
}

static Node *parse_add() {
  Node *node = parse_mul();
  for (;;) {
    if (token_consume("+")) {
      node = new_node(ND_ADD, node, parse_mul());
    } else if (token_consume("-")) {
      node = new_node(ND_SUB, node, parse_mul());
    } else {
      return node;
    }
  }
}

static Node *parse_relational() {
  Node *node = parse_add();
  for (;;) {
    if (token_consume("<")) {
      node = new_node(ND_LT, node, parse_add());
    } else if (token_consume(">")) {
      node = new_node(ND_LT, parse_add(), node);
    } else if (token_consume("<=")) {
      node = new_node(ND_GE, parse_add(), node);
    } else if (token_consume(">=")) {
      node = new_node(ND_GE, node, parse_add());
    } else {
      return node;
    }
  }
}

static Node *parse_equality() {
  Node *node = parse_relational();
  for (;;) {
    if (token_consume("==")) {
      node = new_node(ND_EQ, node, parse_relational());
    } else if (token_consume("!=")) {
      node = new_node(ND_NE, node, parse_relational());
    } else {
      return node;
    }
  }
}

static Node *parse_assign() {
  Node *node = parse_equality();
  if (token_consume("=")) {
    node = new_node(ND_ASSIGN, node, parse_assign());
  }
  return node;
}

Node *parse_expr() { return parse_assign(); }

Node *parse_stmt() {
  Node *node = parse_expr();
  token_expect(";");
  return node;
}

Node *code[100];

void parse_program() {
  int i = 0;
  while (!token_at_eof()) {
    code[i++] = parse_stmt();
  }
  code[i] = NULL;
}