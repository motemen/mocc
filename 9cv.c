#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};

Token *token;

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // numbers
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val; // used when kind == ND_NUM
};

char *user_input;

void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  verror_at(loc, fmt, ap);
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(token->str, fmt, ap);
}

// Consumes a token matching op operator
bool consume(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

void expect(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    error("not '%c'", op);
  }

  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM) {
    error("not a number");
  }

  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() { return token->kind == TK_EOF; }

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;

  cur->next = tok;
  return tok;
}

Token *new_token_num(Token *cur, int val) {
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

    if (strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

// syntax:
//    expr    = mul ("+" mul | "-" mul)*
//    mul     = unary ("*" unary | "/" unary)*
//    unary   = ("+" | "-")? primary
//    primary = num | "(" expr ")"

Node *expr();

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  int val = expect_number();
  return new_node_num(val);
}

Node *unary() {
  if (consume("+")) {
    return primary();
  }

  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

Node *expr() {
  Node *node = mul();
  for (;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

void visit(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  li t0, %d\n", node->val);

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else if (node->kind == ND_ADD || node->kind == ND_SUB ||
             node->kind == ND_MUL || node->kind == ND_DIV) {
    visit(node->lhs);
    visit(node->rhs);

    // pop rhs -> t1
    printf("  lw t1, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    // pop lhs -> t0
    printf("  lw t0, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    if (node->kind == ND_ADD) {
      printf("  add t0, t0, t1\n");
    } else if (node->kind == ND_SUB) {
      printf("  sub t0, t0, t1\n");
    } else if (node->kind == ND_MUL) {
      printf("  mul t0, t0, t1\n");
    } else if (node->kind == ND_DIV) {
      printf("  div t0, t0, t1\n");
    } else {
      error("not implemented: %d", node->kind);
    }

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else {
    error("not implemented: %d", node->kind);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Bad argnum\n");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  if (token != NULL && token->kind != TK_EOF) {
    error("not all tokens are consumed");
  }

  printf(".global main\n");
  printf("main:\n");

  visit(node);

  // pop -> t0
  printf("  lw t0, 0(sp)\n");
  printf("  addi sp, sp, 4\n");

  printf("  mv a0, t0\n");
  printf("  ret\n");
  return 0;
}
