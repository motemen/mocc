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

  parse_program();

  if (token != NULL && token->kind != TK_EOF) {
    error("not all tokens are consumed");
  }

  printf(".global main\n");
  printf("main:\n");

  // Prologue
  int num_locals = 0;
  LVar *l = locals;
  while (l) {
    num_locals++;
    l = l->next;
  }

  printf("  # Prologue\n");
  printf("  sd fp, -8(sp)\n"); // fp を保存
  printf("  addi fp, sp, -8\n");
  // スタックポインタを移動。関数を抜けるまで動かない
  printf("  addi sp, sp, -%d\n", 8 * num_locals + 8 /* for saved fp */);

  for (int i = 0; code[i]; i++) {
    codegen_visit(code[i]);
    codegen_pop_t0();
  }

  // Epilogue

  printf("  # Epilogue\n");
  // sp を戻す
  printf("  addi sp, sp, %d\n", 8 * num_locals + 8);
  // fp も戻す
  printf("  ld fp, -8(sp)\n");

  printf("  # Set return value\n");
  printf("  mv a0, t0\n");
  printf("  ret\n");
  return 0;
}
