#include "9cv.h"
#include <assert.h>
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

void codegen_pop_discard() {
  printf("  # pop\n");
  printf("  addi sp, sp, 8\n");
}

void codegen_push_t0() {
  printf("  # push t0\n");
  printf("  sw t0, -8(sp)\n");
  printf("  addi sp, sp, -8\n");
}

void codegen_push_dummy() {
  printf("  # push dummy\n");
  printf("  addi sp, sp, -8\n");
}

// TODO: ローカル変数をなんとかしないといけないのだが…
void codegen_prologue() {
  int num_locals = 0;
  LVar *l = locals;
  while (l) {
    num_locals++;
    l = l->next;
  }

  printf("  # Prologue\n");
  printf("  sw ra, 0(sp)\n");  // ra を保存
  printf("  sw fp, -8(sp)\n"); // fp を保存
  printf("  addi fp, sp, -8\n");
  // スタックポインタを移動。関数を抜けるまで動かない
  printf("  addi sp, sp, -%d\n", 8 * num_locals + 8 /* for saved fp */);
  printf("\n");
}

void codegen_epilogue() {
  int num_locals = 0;
  LVar *l = locals;
  while (l) {
    num_locals++;
    l = l->next;
  }

  printf("\n");
  printf("  # Epilogue\n");
  // sp を戻す
  printf("  addi sp, sp, %d\n", 8 * num_locals + 8);
  // fp も戻す
  printf("  ld fp, -8(sp)\n");
  // ra も戻す
  printf("  ld ra, 0(sp)\n");

  // これは ND_RETURN のときだけにすべき!!!
  printf("  # Set return value\n");
  printf("  mv a0, t0\n");

  printf("  ret\n");
}

int label_index = 0;

void codegen_visit(Node *node) {
  int lindex;

  switch (node->kind) {
  case ND_NUM:
    printf("  # constant '%d'\n", node->val);
    printf("  li t0, %d\n", node->val);

    // push
    codegen_push_t0();
    return;

  case ND_LT:
    codegen_visit(node->lhs); // -> t0
    codegen_visit(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");

    codegen_push_t0();
    return;

  case ND_GE:
    codegen_visit(node->lhs); // -> t0
    codegen_visit(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");
    printf("  xori t0, t0, 1\n");

    codegen_push_t0();
    return;

  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
    codegen_visit(node->lhs); // -> t0
    codegen_visit(node->rhs); // -> t1

    codegen_pop_t1();
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
      assert(0);
    }

    codegen_push_t0();
    return;

  case ND_EQ:
  case ND_NE:
    codegen_visit(node->lhs); // -> t0
    codegen_visit(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  xor t0, t0, t1\n");
    if (node->kind == ND_EQ) {
      printf("  seqz t0, t0\n");
    } else {
      printf("  snez t0, t0\n");
    }

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
    printf("  # ND_RETURN LHS {\n");
    codegen_visit(node->lhs);
    printf("  # ND_RETURN LHS }\n");
    codegen_pop_t0();

    codegen_epilogue();

    return;

  case ND_IF:
    lindex = ++label_index;

    printf("  # if {\n");

    codegen_visit(node->lhs);
    codegen_pop_t0();

    printf("  beqz t0, .Lelse%03d\n", lindex);

    // if {
    codegen_visit(node->rhs);
    codegen_pop_discard();
    printf("  j .Lend%03d\n", lindex);
    // }

    // else {
    printf(".Lelse%03d:\n", lindex);
    if (node->node3) {
      codegen_visit(node->node3);
      codegen_pop_discard();
    }
    // }

    printf(".Lend%03d:\n", lindex);

    codegen_push_dummy();

    printf("  # if }\n");
    printf("\n");

    return;

  case ND_WHILE:
    lindex = ++label_index;

    printf(".Lbegin%03d:\n", lindex);

    codegen_visit(node->lhs);
    codegen_pop_t0();

    printf("  beqz t0, .Lend%03d\n", lindex);

    codegen_visit(node->rhs); // { ... }
    codegen_pop_discard();

    printf("  j .Lbegin%03d\n", lindex);

    printf(".Lend%03d:\n", lindex);

    codegen_push_dummy();

    return;

  case ND_FOR:
    lindex = ++label_index;

    if (node->lhs) {
      codegen_visit(node->lhs);
      codegen_pop_discard();
    }

    printf(".Lbegin%03d:\n", lindex);

    if (node->rhs) {
      codegen_visit(node->rhs);
      codegen_pop_t0();
      printf("  beqz t0, .Lend%03d\n", lindex);
    }

    if (node->node4) {
      codegen_visit(node->node4); // { ... }
      codegen_pop_discard();
    }

    // i++ みたいなとこ
    if (node->node3) {
      codegen_visit(node->node3);
      codegen_pop_discard();
    }

    printf("j .Lbegin%03d\n", lindex);

    printf(".Lend%03d:\n", lindex);

    codegen_push_dummy();

    return;

  case ND_BLOCK:
    for (NodeList *n = node->nodes; n; n = n->next) {
      codegen_visit(n->node);
      codegen_pop_discard();
    }

    codegen_push_dummy();

    return;

  case ND_CALL: {
    int arg_count = 0;
    for (NodeList *n = node->nodes; n; n = n->next) {
      codegen_visit(n->node);
      codegen_pop_t0();
      printf("  mv a%d, t0\n", arg_count++);
    }

    // これいるか？？ sp はどういう状態で引き渡せばいいんだ
    printf("  addi sp, sp, -8\n");
    printf("  call %.*s\n", node->name_len, node->name);
    printf("  addi sp, sp, 8\n");

    // 結果は a0 に入っているよな
    printf("  # push a0\n");
    printf("  sw a0, -8(sp)\n");
    printf("  addi sp, sp, -8\n");
    return;
  }

  case ND_FUNCDECL:
    printf("\n");
    printf("%.*s:\n", node->name_len, node->name);

    codegen_prologue();

    int arg_count = 0;
    for (NodeList *a = node->args; a; a = a->next) {
      // a0 を lvar xyz に代入するみたいなことをする
      printf("  # assign to argument '%.*s'\n", a->node->source_len,
             a->node->source_pos);
      printf("  sw a%d, -%d(fp)\n", arg_count, a->node->lvar->offset);

      arg_count++;
    }

    for (NodeList *n = node->nodes; n; n = n->next) {
      codegen_visit(n->node);
      codegen_pop_t0();
    }
    codegen_epilogue();
    return;
  }

  error_at(node->source_pos, "codegen not implemented: %s",
           node_kind_to_str(node->kind));
}
