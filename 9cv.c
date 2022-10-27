#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Bad argnum\n");
    return 1;
  }

  char *p = argv[1];

  printf(".global main\n");
  printf("main:\n");
  printf("  li t0, %ld\n", strtol(p, &p, 10));

  while (*p) {
    if (*p == '+') {
      printf("  addi t0, t0, %ld\n", strtol(p, &p, 10));
      continue;
    }
    if (*p == '-') {
      printf("  addi t0, t0, %ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "Bad char: '%c'\n", *p);
    return 1;
  }

  printf("  mv a0, t0\n");
  printf("  ret\n");
  return 0;
}
