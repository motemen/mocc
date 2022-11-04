#include "9cv.h"
#include "codegen.h"
#include "tokenize.h"
#include "util.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

char *read_file(char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
    exit(1);
  }

  if (fseek(fp, 0, SEEK_END) == -1) {
    fprintf(stderr, "%s: fseek: %s\n", path, strerror(errno));
    exit(1);
  }

  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1) {
    fprintf(stderr, "%s: fseek: %s\n", path, strerror(errno));
    exit(1);
  }

  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);
  fclose(fp);

  if (size == 0 || buf[size - 1] != '\n') {
    buf[size++] = '\n';
  }
  buf[size] = '\0';

  return buf;
}

char *read_stdin() {
  char *buf = NULL;
  size_t cap = 1024;
  size_t len = 0;
  for (;;) {
    buf = realloc(buf, cap);
    if (!buf) {
      fprintf(stderr, "realloc: %s\n", strerror(errno));
      exit(1);
    }

    size_t n = fread(buf + len, 1, cap - len - 2, stdin);
    if (n == 0) {
      if (ferror(stdin)) {
        fprintf(stderr, "fread: %s\n", strerror(errno));
        exit(1);
      }
    }
    if (feof(stdin)) {
      if (buf[len + n - 1] != '\n') {
        buf[len + n++] = '\n';
      }
      buf[len + n] = '\0';
      return buf;
    }

    len += n;
    cap *= 2;
  }
}

char *user_input;
char *input_filename;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Bad argnum\n");
    return 1;
  }

  input_filename = argv[1];

  if (strncmp(argv[1], "-", 1) == 0) {
    user_input = read_stdin();
  } else {
    user_input = read_file(argv[1]);
  }

  tokenize(user_input);

  parse_program();

  codegen();

  return 0;
}
