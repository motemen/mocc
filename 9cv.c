#include "9cv.h"
#include "codegen.h"
#include "tokenize.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *user_input;
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

  printf("  .global main\n");

  for (int i = 0; code[i]; i++) {
    if (codegen(code[i]))
      codegen_pop_t0();
  }

  return 0;
}
