#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Bad argnum\n");
    return 1;
  }

  printf(".global main\n");
  printf("main:\n");
  printf("  addi a0, zero, %d\n", atoi(argv[1]));
  printf("  ret\n");
  return 0;
}
