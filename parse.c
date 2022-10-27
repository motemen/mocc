#include "9cv.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Token *token;

// Consumes a token matching op operator
static bool token_consume(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

static void token_expect(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    error("not '%c'", op);
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

    if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
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
  return node;
}

static Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

// syntax:
//    expr        = equality
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? primary
//    primary     = num | "(" expr ")"

static Node *parse_primary() {
  if (token_consume("(")) {
    Node *node = parse_expr();
    token_expect(")");
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

Node *parse_expr() { return parse_equality(); }
