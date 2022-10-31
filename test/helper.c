#include <stdio.h>
#include <stdlib.h>

void alloc4(int **p, int x1, int x2, int x3, int x4) {
  fprintf(stderr, "alloc4: p=%p, x1=%d, x2=%d, x3=%d, x4=%d\n", p, x1, x2, x3,
          x4);

  *p = (int *)malloc(4 * sizeof(int));
  fprintf(stderr, "malloc:%p\n", *p);

  (*p)[0] = x1;
  (*p)[1] = x2;
  (*p)[2] = x3;
  (*p)[3] = x4;
}