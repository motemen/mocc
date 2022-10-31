#include "9cv.h"
#include "tokenize.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

_Noreturn void verror_at(char *loc, char *fmt, va_list ap) {
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

_Noreturn void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  verror_at(loc, fmt, ap);
}

_Noreturn void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (!token) {
    verror_at(NULL, fmt, ap);
  } else {
    verror_at(token->str, fmt, ap);
  }
}
