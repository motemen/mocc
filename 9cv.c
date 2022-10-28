#include "9cv.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *user_input;

void verror_at(char *loc, char *fmt, va_list ap) {
  if (loc) {
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
  }

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
  if (!token) {
    verror_at(NULL, fmt, ap);
  } else {
    verror_at(token->str, fmt, ap);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Bad argnum\n");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);

  parse_program();

  if (token != NULL && token->kind != TK_EOF) {
    error("not all tokens are consumed");
  }

  printf(".global main\n");

  for (int i = 0; code[i]; i++) {
    codegen_visit(code[i]);
    codegen_pop_t0();
  }

  return 0;
}
