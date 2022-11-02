#include "tokenize.h"
#include "util.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Token *token;

// TODO: token_advance にする
bool token_consume(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  token = token->next;
  return true;
}

Token *token_advance(TokenKind kind) {
  if (token->kind != kind) {
    return NULL;
  }

  Token *result = token;
  token = token->next;
  return result;
}

// TK_RESERVED なものがあれば進んで true、なかったら false
bool token_consume_reserved(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

bool token_consume_type(char *type) {
  if (token->kind != TK_TYPE || token->len != strlen(type) ||
      strncmp(token->str, type, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

bool token_at_eof() { return token->kind == TK_EOF; }

void token_expect(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op) ||
      strncmp(token->str, op, token->len) != 0) {
    error("token_expect: not '%c'", *op);
  }

  token = token->next;
}

int token_expect_number() {
  if (token->kind != TK_NUM) {
    error("expected number");
  }

  int val = token->val;
  token = token->next;
  return val;
}

Token *token_consume_ident() {
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

static bool isident(char ch) { return isalnum(ch) || ch == '_'; }

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '"') {
      char *end = p + 1;
      while (*end != '"') {
        end++;
      }
      cur = new_token(TK_STRING, cur, p + 1, end - p - 1);
      p = end + 1;
      continue;
    }

    if (strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0 ||
        strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;{},&[]", *p)) {
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

    if (strncmp("sizeof", p, 6) == 0 && !isident(p[6])) {
      cur = new_token(TK_SIZEOF, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp("int", p, 3) == 0 && !isident(p[3])) {
      cur = new_token(TK_TYPE, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp("char", p, 4) == 0 && !isident(p[4])) {
      cur = new_token(TK_TYPE, cur, p, 4);
      p += 4;
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
