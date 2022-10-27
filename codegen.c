#include "9cv.h"
#include <stdio.h>

void codegen_visit(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  li t0, %d\n", node->val);

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else if (node->kind == ND_LT) {
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    printf("  lw t1, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    // pop lhs -> t0
    printf("  lw t0, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    printf("  slt t0, t0, t1\n");

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else if (node->kind == ND_GE) {
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    printf("  lw t1, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    // pop lhs -> t0
    printf("  lw t0, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    printf("  slt t0, t0, t1\n");
    printf("  xori t0, t0, 1\n");

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else if (node->kind == ND_ADD || node->kind == ND_SUB ||
             node->kind == ND_MUL || node->kind == ND_DIV) {
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    printf("  lw t1, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    // pop lhs -> t0
    printf("  lw t0, 0(sp)\n");
    printf("  addi sp, sp, 4\n");

    if (node->kind == ND_ADD) {
      printf("  add t0, t0, t1\n");
    } else if (node->kind == ND_SUB) {
      printf("  sub t0, t0, t1\n");
    } else if (node->kind == ND_MUL) {
      printf("  mul t0, t0, t1\n");
    } else if (node->kind == ND_DIV) {
      printf("  div t0, t0, t1\n");
    } else {
      error("unreachable");
    }

    // push
    printf("  sw t0, -4(sp)\n");
    printf("  addi sp, sp, -4\n");
  } else {
    error("not implemented: %d", node->kind);
  }
}
