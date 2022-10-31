#include "9cv.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *token;
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

static bool token_consume(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  token = token->next;
  return true;
}

// TK_RESERVED なものがあれば進んで true、なかったら false
static bool token_consume_reserved(char *op) {
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
    error("expected number");
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

    if (strchr("+-*/()<>=;{},&", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !isident(p[6])) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !isident(p[2])) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !isident(p[4])) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !isident(p[5])) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (strncmp("for", p, 3) == 0 && !isident(p[3])) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp("int", p, 3) == 0 && !isident(p[3])) {
      cur = new_token(TK_TYPE, cur, p, 3);
      p += 3;
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

    error("cannot tokenize: '%c'", *p);
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

LVar *add_lvar(char *context, char *name, int len) {
  assert(context != NULL);

  LVar *last_var = locals;
  int i = 0;
  for (LVar *var = locals; var; last_var = var, var = var->next) {
    if (var->len == len && !strncmp(var->name, name, len) &&
        strcmp(var->context, context) == 0) {
      error("variable already defined: '%.*s'", len, name);
    }
    i++;
  }

  LVar *var = calloc(1, sizeof(LVar));
  var->name = name;
  var->len = len;
  var->context = context;
  var->offset = (i + 1) * 8;

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
//    stmt        = expr ";"
//                | "return" expr ";"
//                | "if" "(" expr ")" stmt ("else" stmt)?
//                | "while" "(" expr ")" stmt
//                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                | "{" stmt* "}"
//                | vardecl ";"
//    vardecl     = "int" ident
//    expr        = assign
//    assign      = equality ("=" assign)?
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? primary
//                | "*" unary
//                | "&" unary
//    primary     = num | ident ("(" (expr ("," expr)*)? ")")? | "(" expr ")"
//    ident	      = /[a-z][a-z0-9]*/

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
    return node;
  }

  int val = token_expect_number();
  return new_node_num(val);
}

static Node *parse_unary() {
  if (token_consume_reserved("+")) {
    return parse_primary();
  }

  if (token_consume_reserved("-")) {
    return new_node(ND_SUB, new_node_num(0), parse_primary());
  }

  if (token_consume_reserved("*")) {
    return new_node(ND_DEREF, parse_unary(), NULL);
  }

  if (token_consume_reserved("&")) {
    return new_node(ND_ADDR, parse_unary(), NULL);
  }

  return parse_primary();
}

static Node *parse_mul() {
  Node *node = parse_unary();
  for (;;) {
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

  if (token_consume(TK_TYPE)) {
    Token *tok_var = token_consume_ident();
    if (!tok_var) {
      error("expected variable name");
    }

    LVar *lvar = add_lvar(context, tok_var->str, tok_var->len);

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
      if (!token_consume(TK_TYPE)) {
        error("TK_TYPE expected");
      }

      Token *tok = token_consume_ident();
      if (!tok) {
        error("expected ident");
      }

      Node *ident = calloc(1, sizeof(Node));
      ident->kind = ND_LVAR;
      LVar *lvar = add_lvar(context, tok->str, tok->len);
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