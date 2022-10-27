#include "9cv.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
