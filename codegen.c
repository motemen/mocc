#include "9cv.h"
#include <stdio.h>

// これ addi 4 にしたら死んだ
void codegen_pop_t0() {
  printf("  # pop t0\n");
  printf("  lw t0, 0(sp)\n");
  printf("  addi sp, sp, 8\n");
}

void codegen_pop_t1() {
  printf("  # pop t1\n");
  printf("  lw t1, 0(sp)\n");
  printf("  addi sp, sp, 8\n");
}

void codegen_push_t0() {
  printf("  # push t0\n");
  printf("  sw t0, -8(sp)\n");
  printf("  addi sp, sp, -8\n");
}

int label_index = 0;

void codegen_visit(Node *node) {
  int lindex;

  switch (node->kind) {
  case ND_NUM:
    printf("  li t0, %d\n", node->val);

    // push
    codegen_push_t0();
    return;

  case ND_LT:
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    codegen_pop_t1();
    // pop lhs -> t0
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");

    // push
    codegen_push_t0();
    return;

  case ND_GE:
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    codegen_pop_t1();

    // pop lhs -> t0
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");
    printf("  xori t0, t0, 1\n");

    // push
    codegen_push_t0();
    return;

  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    codegen_pop_t1();
    // pop lhs -> t0
    codegen_pop_t0();

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
    codegen_push_t0();
    return;

  case ND_EQ:
  case ND_NE:
    codegen_visit(node->lhs);
    codegen_visit(node->rhs);

    // pop rhs -> t1
    codegen_pop_t1();
    // pop lhs -> t0
    codegen_pop_t0();

    printf("  xor t0, t0, t1\n");
    if (node->kind == ND_EQ) {
      printf("  seqz t0, t0\n");
    } else {
      printf("  snez t0, t0\n");
    }

    // push
    codegen_push_t0();
    return;

  case ND_LVAR:
    printf("  # read variable '%.*s'\n", node->source_len, node->source_pos);
    printf("  lw t0, -%d(fp)\n", node->lvar->offset);
    codegen_push_t0();
    return;

  case ND_ASSIGN:
    if (node->lhs->kind != ND_LVAR) {
      error_at(node->lhs->source_pos, "not an lvalue: %s",
               node_kind_to_str(node->lhs->kind));
    }

    codegen_visit(node->rhs);

    codegen_pop_t0();

    printf("  # assign to variable '%.*s'\n", node->lhs->source_len,
           node->lhs->source_pos);
    printf("  sw t0, -%d(fp)\n", node->lhs->lvar->offset);

    codegen_push_t0();

    return;

  case ND_RETURN:
    codegen_visit(node->lhs);

    codegen_pop_t0();

    printf("  # Set return value\n");
    printf("  mv a0, t0\n");
    printf("  ret\n");

    return;

  case ND_IF:
    lindex = ++label_index;

    codegen_visit(node->lhs);

    codegen_pop_t0();

    printf("  beqz t0, .Lelse%03d\n", lindex);

    // if {
    codegen_visit(node->rhs);
    printf("j .Lend%03d\n", lindex);
    // }

    // else {
    printf(".Lelse%03d:\n", lindex);
    if (node->else_stmt) {
      codegen_visit(node->else_stmt);
    }
    // }

    printf(".Lend%03d:\n", lindex);

    return;

  case ND_WHILE:
    lindex = ++label_index;

    printf(".Lbegin%03d:\n", lindex);

    codegen_visit(node->lhs);
    codegen_pop_t0();
    printf("  beqz t0, .Lend%03d\n", lindex);

    codegen_visit(node->rhs);

    printf("  j .Lbegin%03d\n", lindex);

    printf(".Lend%03d:\n", lindex);

    return;
  }

  error_at(node->source_pos, "codegen not implemented: %s",
           node_kind_to_str(node->kind));
}
