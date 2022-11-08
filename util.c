#include "mocc.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

_Noreturn void verror_at(char *loc, char *fmt, va_list ap) {
  if (loc) {
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
      line--;
    }

    char *end = loc;
    while (*end != '\n') {
      end++;
    }

    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
      if (*p == '\n') {
        line_num++;
      }
    }

    int indent = fprintf(stderr, "%s:%d: ", input_filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = loc - line + indent;
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
  if (!curr_token) {
    verror_at(NULL, fmt, ap);
  } else {
    verror_at(curr_token->str, fmt, ap);
  }
}
