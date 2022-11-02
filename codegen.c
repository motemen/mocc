#include "codegen.h"
#include "9cv.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ターゲットの ISA は RISCV64GC ということにする
// https://riscv.org/wp-content/uploads/2015/01/riscv-calling.pdf
// int のサイズは 4 bytes
// void * のサイズは 8 bytes

// これ addi 4 にしたら死んだ
void codegen_pop_t0() {
  printf("  # pop t0 {{{\n");
  printf("  ld t0, 0(sp)\n");
  printf("  addi sp, sp, 8\n");
  printf("  # }}}\n");
}

void codegen_pop_t1() {
  printf("  # pop t1 {{{\n");
  printf("  ld t1, 0(sp)\n");
  printf("  addi sp, sp, 8\n");
  printf("  # }}}\n");
}

void codegen_pop_discard() {
  printf("  # pop\n");
  printf("  addi sp, sp, 8\n");
}

void codegen_push_t0() {
  printf("  # push t0 {{{\n");
  printf("  sd t0, -8(sp)\n");
  printf("  addi sp, sp, -8\n");
  printf("  # }}}\n");
}

static int max_lvar_offset(const char *context) {
  int max = 0;

  LVar *last_lvar = NULL;
  for (LVar *lvar = locals; lvar; lvar = lvar->next) {
    if (strcmp(context, lvar->context) == 0) {
      // 常にあとのほうが offset でかいはずなのでこれでよい
      last_lvar = lvar;
    }
  }

  if (!last_lvar) {
    return 0;
  }

  return last_lvar->offset;
}

static void codegen_prologue(char *context) {
  printf("  # Prologue\n");
  printf("  sd ra, 0(sp)\n");  // ra を保存
  printf("  sd fp, -8(sp)\n"); // fp を保存
  printf("  addi fp, sp, -8\n");
  // スタックポインタを移動。関数を抜けるまで動かない
  printf("  addi sp, sp, -%d\n",
         max_lvar_offset(context) + 8 /* for saved fp */);
  printf("\n");
}

// a0 に返り値を設定してから呼ぶこと
static void codegen_epilogue(char *context) {
  int num_locals = 0;
  LVar *l = locals;
  while (l) {
    num_locals++;
    l = l->next;
  }

  printf("\n");
  printf("  # Epilogue\n");
  // sp を戻す
  printf("  addi sp, sp, %d\n", max_lvar_offset(context) + 8);
  // fp も戻す
  // ここ lw にしたらおかしくなった
  printf("  ld fp, -8(sp)\n");
  // ra も戻す
  printf("  ld ra, 0(sp)\n");

  printf("  ret\n");
}

int label_index = 0;

// LVar である node のアドレスを push する
// または、ポインタの deref を計算してアドレスを push する
void codegen_push_lvalue(Node *node) {
  if (node->kind == ND_LVAR) {
    printf("  # (lvalue) address for '%.*s'\n", node->lvar->len,
           node->lvar->name);
    printf("  addi t0, fp, -%d\n", node->lvar->offset);
    codegen_push_t0();
    return;
  }

  // *y -> y の値をアドレスとして push する
  // y が配列のときは *y[0] みたいな感じであつかう
  // **z -> (*z) の値をアドレスとして push する
  if (node->kind == ND_DEREF) {
    printf("  # (lvalue) deref address of (%s)\n",
           type_to_str(typeof_node(node->lhs)));
    codegen_expr(node->lhs);
    return;
  }

  if (node->kind == ND_GVAR) {
    printf("  # (lvalue) address for global variable '%.*s'\n", node->gvar->len,
           node->gvar->name);
    printf("  lui t0, %%hi(%.*s)\n", node->gvar->len, node->gvar->name);
    printf("  addi t0, t0, %%lo(%.*s)\n", node->gvar->len, node->gvar->name);
    codegen_push_t0();
    return;
  }

  error_at(node->source_pos, "not an lvalue: %s", node_kind_to_str(node->kind));
}

void codegen_expr(Node *node) {
  int lindex;

  switch (node->kind) {
  case ND_NUM:
    printf("  # constant '%d'\n", node->val);
    printf("  li t0, %d\n", node->val);

    // push
    codegen_push_t0();
    return;

  case ND_LT:
    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");

    codegen_push_t0();
    return;

  case ND_GE:
    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  slt t0, t0, t1\n");
    printf("  xori t0, t0, 1\n");

    codegen_push_t0();
    return;

  case ND_ADD:
  case ND_SUB: {
    int ptr_size = 0;

    // TODO: ltype しかみてないけど rtype もみたいよね
    Type *ltype = typeof_node(node->lhs);
    if (ltype->ty == PTR || ltype->ty == ARRAY) {
      ptr_size = sizeof_type(ltype->base);
    }

    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

    printf("  # pointer arithmetic: ltype=(%s), ptr_size=%d\n",
           type_to_str(ltype), ptr_size);

    codegen_pop_t1();
    if (ptr_size > 1) {
      printf("  # do pointer arithmetic\n");
      printf("  li t3, %d\n", ptr_size);
      printf("  mul t1, t1, t3\n");
    }

    codegen_pop_t0();

    if (node->kind == ND_ADD) {
      printf("  add t0, t0, t1\n");
    } else if (node->kind == ND_SUB) {
      printf("  sub t0, t0, t1\n");
    } else {
      assert(0);
    }

    codegen_push_t0();
    return;
  }

  case ND_MUL:
  case ND_DIV:
    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    if (node->kind == ND_MUL) {
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
    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

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

  case ND_LOGOR:
    codegen_expr(node->lhs); // -> t0
    codegen_expr(node->rhs); // -> t1

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  or t0, t0, t1\n");

    codegen_push_t0();
    return;

  case ND_LVAR:
    // codegen_push_lvalue(node);
    // codegen_pop_t0();
    // printf("  ld t0, 0(t0)\n");
    // codegen_push_t0();
    if (node->lvar->type->ty == ARRAY) {
      // 配列の場合は先頭要素へのポインタに変換されるのでアドレスを push
      // sizeof, & の場合だけ例外だがそれはそちら側で処理されてる。はず。
      printf("  add t0, fp, -%d\n", node->lvar->offset);
    } else {
      printf("  ld t0, -%d(fp)\n", node->lvar->offset);
    }
    codegen_push_t0();
    return;

  case ND_ASSIGN:
    printf("  # ND_ASSIGN {{{\n");
    // -> t0
    codegen_push_lvalue(node->lhs);
    // -> t1
    codegen_expr(node->rhs);

    codegen_pop_t1();
    codegen_pop_t0();

    printf("  # assign to variable '%.*s'\n", node->lhs->source_len,
           node->lhs->source_pos);

    // Type *t = typeof_node(node->lhs);
    // char *loc = node->source_pos;
    // int pos = loc - user_input;
    // fprintf(stderr, "%s\n", user_input);
    // fprintf(stderr, "%*s", pos, " ");
    // fprintf(stderr, "^ ");
    // fprintf(stderr, "  # type: %d", t->ty);
    // fprintf(stderr, "  # sizeof: %d\n", sizeof_type(typeof_node(node->lhs)));

    switch (sizeof_type(typeof_node(node->lhs))) {
    case 1:
      printf("  sb t1, 0(t0)\n");
      break;
    case 4:
      printf("  sw t1, 0(t0)\n");
      break;
    case 8:
      printf("  sd t1, 0(t0)\n");
      break;
    default:
      error("unknown size of variable: (%s)",
            type_to_str(typeof_node(node->lhs)));
    }

    printf("  mv t0, t1\n");
    codegen_push_t0();

    printf("  # }}} ND_ASSIGN\n");
    return;

  case ND_CALL: {
    int arg_count = 0;
    for (NodeList *n = node->nodes; n; n = n->next) {
      codegen_expr(n->node);
      arg_count++;
    }

    while (arg_count > 0) {
      codegen_pop_t0();
      printf("  mv a%d, t0\n", --arg_count);
    }

    // これいるか？？ sp はどういう状態で引き渡せばいいんだ
    printf("  addi sp, sp, -8\n");
    printf("  call %.*s\n", node->name_len, node->name);
    printf("  addi sp, sp, 8\n");

    // 結果は a0 に入っているよな
    printf("  # push return value (a0) {{{\n");
    printf("  sd a0, -8(sp)\n");
    printf("  addi sp, sp, -8\n");
    printf("  # }}}\n");
    return;
  }

  case ND_DEREF: {
    codegen_expr(node->lhs);

    if (typeof_node(node)->ty == ARRAY) {
      // deref した結果が配列の場合はさらにポインタとしてあつかうので
      // push されたアドレスをそのまま返す
      // ややこしすぎでは。なんか別の箇所にまとめられそう
      return;
    }

    Type *type = typeof_node(node->lhs);
    int size = sizeof_type(type->base);

    codegen_pop_t0();

    printf("  # deref to get (%s)\n", type_to_str(type->base));
    switch (size) {
    case 1:
      printf("  lb t0, 0(t0)\n");
      break;
    case 4:
      printf("  lw t0, 0(t0)\n");
      break;
    case 8:
      printf("  ld t0, 0(t0)\n");
      break;

    default:
      error("unknown size of deref: (%s)", type_to_str(typeof_node(node->lhs)));
    }
    codegen_push_t0();

    return;
  }

  case ND_ADDR:
    // TODO: ARRAY のときなんかやる
    codegen_push_lvalue(node->lhs);

    return;

  case ND_GVAR:
    printf("  lui t0, %%hi(%.*s)\n", node->gvar->len, node->gvar->name);
    printf("  lw t0, %%lo(%.*s)(t0)\n", node->gvar->len, node->gvar->name);
    codegen_push_t0();

    return;

  case ND_STRING:
    printf("  lui t0, %%hi(.LC%d)\n", node->val);
    printf("  addi t0, t0, %%lo(.LC%d)\n", node->val);
    codegen_push_t0();

    return;

  case ND_RETURN:
  case ND_IF:
  case ND_WHILE:
  case ND_FOR:
  case ND_BLOCK:
  case ND_FUNCDECL:
  case ND_VARDECL:
  case ND_GVARDECL:
    break;
  }

  error_at(node->source_pos, "not an expression: %s",
           node_kind_to_str(node->kind));
}

void codegen_preamble() {
  printf("  .section .rodata\n");
  for (StrLit *lit = str_lits; lit; lit = lit->next) {
    printf(".LC%d:\n", lit->index);
    printf("  .string \"%.*s\"\n", lit->len, lit->str);
  }

  printf("\n");
  printf("  .global main\n");
}

bool codegen(Node *node) {
  int lindex;

  switch (node->kind) {
  case ND_NUM:
  case ND_LT:
  case ND_GE:
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LOGOR:
  case ND_LVAR:
  case ND_ASSIGN:
  case ND_CALL:
  case ND_DEREF:
  case ND_ADDR:
  case ND_GVAR:
  case ND_STRING:
    codegen_expr(node);
    return true;

  case ND_RETURN:
    printf("  # ND_RETURN LHS {\n");
    codegen_expr(node->lhs);
    printf("  # ND_RETURN LHS }\n");
    codegen_pop_t0();

    printf("  mv a0, t0\n");
    codegen_epilogue(context);

    return false;

  case ND_IF:
    lindex = ++label_index;

    printf("  # if {\n");

    codegen_expr(node->lhs);
    codegen_pop_t0();

    printf("  beqz t0, .Lelse%03d\n", lindex);

    // if {
    if (codegen(node->rhs))
      codegen_pop_discard();
    printf("  j .Lend%03d\n", lindex);
    // }

    // else {
    printf(".Lelse%03d:\n", lindex);
    if (node->node3) {
      codegen(node->node3);
      codegen_pop_discard();
    }
    // }

    printf(".Lend%03d:\n", lindex);

    printf("  # if }\n");
    printf("\n");

    return false;

  case ND_WHILE:
    lindex = ++label_index;

    printf(".Lbegin%03d:\n", lindex);

    codegen_expr(node->lhs);
    codegen_pop_t0();

    printf("  beqz t0, .Lend%03d\n", lindex);

    if (codegen(node->rhs))
      codegen_pop_discard();

    printf("  j .Lbegin%03d\n", lindex);

    printf(".Lend%03d:\n", lindex);

    return false;

  case ND_FOR:
    lindex = ++label_index;

    if (node->lhs) {
      if (codegen(node->lhs))
        codegen_pop_discard();
    }

    printf(".Lbegin%03d:\n", lindex);

    if (node->rhs) {
      codegen_expr(node->rhs);
      codegen_pop_t0();
      printf("  beqz t0, .Lend%03d\n", lindex);
    }

    if (node->node4) {
      if (codegen(node->node4)) // { ... }
        codegen_pop_discard();
    }

    // i++ みたいなとこ
    if (node->node3) {
      codegen_expr(node->node3);
      codegen_pop_discard();
    }

    printf("j .Lbegin%03d\n", lindex);

    printf(".Lend%03d:\n", lindex);

    return false;

  case ND_BLOCK:
    for (NodeList *n = node->nodes; n; n = n->next) {
      if (codegen(n->node))
        codegen_pop_discard();
    }

    return false;

  case ND_FUNCDECL:
    printf("\n");
    printf("  .text\n");
    printf("%.*s:\n", node->name_len, node->name);

    context = strndup(node->name, node->name_len);
    codegen_prologue(context);

    int arg_count = 0;
    for (NodeList *a = node->args; a; a = a->next) {
      // a0 を lvar xyz に代入するみたいなことをする
      printf("  # assign to argument '%.*s'\n", a->node->source_len,
             a->node->source_pos);
      printf("  sd a%d, -%d(fp)\n", arg_count, a->node->lvar->offset);

      arg_count++;
    }

    for (NodeList *n = node->nodes; n; n = n->next) {
      if (codegen(n->node))
        codegen_pop_t0();
    }

    printf("  mv a0, zero\n");
    codegen_epilogue(context);

    return false;

  case ND_VARDECL:
    // なにもしない
    printf("  # vardecl '%.*s' offset=%d size=%d\n", node->lvar->len,
           node->lvar->name, node->lvar->offset, sizeof_type(node->lvar->type));
    return false;

  case ND_GVARDECL:
    printf("\n");
    printf("  .data\n");
    printf("%.*s:\n", node->gvar->len, node->gvar->name);
    printf("  .zero %d\n", sizeof_type(node->gvar->type));
    return false;
  }

  error_at(node->source_pos, "codegen not implemented: %s",
           node_kind_to_str(node->kind));
}
