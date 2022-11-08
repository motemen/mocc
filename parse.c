#include "mocc.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 現在のローカル変数のスコープ
// いまのとこ ND_FUNCDECL のみ
// 必要になったら Scope にする
Node *curr_func_scope;

int label_index = 0;

char *node_kind_to_str(NodeKind kind) {
  switch (kind) {
  case ND_NUM:
    return "ND_NUM";
  case ND_LT:
    return "ND_LT";
  case ND_GE:
    return "ND_GE";
  case ND_ADD:
    return "ND_ADD";
  case ND_SUB:
    return "ND_SUB";
  case ND_MUL:
    return "ND_MUL";
  case ND_DIV:
    return "ND_DIV";
  case ND_EQ:
    return "ND_EQ";
  case ND_NE:
    return "ND_NE";
  case ND_LVAR:
    return "ND_LVAR";
  case ND_ASSIGN:
    return "ND_ASSIGN";
  case ND_RETURN:
    return "ND_RETURN";
  case ND_IF:
    return "ND_IF";
  case ND_WHILE:
    return "ND_WHILE";
  case ND_FOR:
    return "ND_FOR";
  case ND_BLOCK:
    return "ND_BLOCK";
  case ND_CALL:
    return "ND_CALL";
  case ND_FUNCDECL:
    return "ND_FUNCDECL";
  case ND_DEREF:
    return "ND_DEREF";
  case ND_ADDR:
    return "ND_ADDR";
  case ND_VARDECL:
    return "ND_VARDECL";
  case ND_GVARDECL:
    return "ND_GVARDECL";
  case ND_GVAR:
    return "ND_GVAR";
  case ND_STRING:
    return "ND_STRING";
  case ND_LOGOR:
    return "ND_LOGOR";
  case ND_BREAK:
    return "ND_BREAK";
  case ND_CONTINUE:
    return "ND_CONTINUE";
  case ND_NOP:
    return "ND_NOP";
  case ND_MEMBER:
    return "ND_MEMBER";
  case ND_NOT:
    return "ND_NOT";
  }

  return "(unknown)";
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->source_pos = prev_token->str;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM, NULL, NULL);
  node->val = val;
  return node;
}

///// Parser /////

// Syntax:
//    program     = (funcdecl | vardecl)*
//    funcdecl    = type ident "(" type expr ("," type expr)* ")"
//                  "{" stmt* "}"
//    type        = ("int" | "char") "*"*
//                | ident
//                | "struct" ident ("{" (type ident ";")* "}")?
//    stmt        = expr ";"
//                | "return" expr ";"
//                | "if" "(" expr ")" stmt ("else" stmt)?
//                | "while" "(" expr ")" stmt
//                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                | "{" stmt* "}"
//                | "break" ";"
//                | vardecl ";"
//    vardecl     = type ident ("[" num "]")? ("=" assign | initializer)?
//    expr        = assign
//    assign      = or ("=" assign)?
//    or          = equality ("||" equality)*
//    equality    = relational ("==" relational | "!=" relational)*
//    relational  = add ("<" add | "<=" add | ">" add | ">=" add)*
//    add         = mul ("+" mul | "-" mul)*
//    mul         = unary ("*" unary | "/" unary)*
//    unary       = ("+" | "-")? postfix
//                | "*" unary
//                | "&" unary
//                | "sizeof" unary
//                | postfix
//    postfix     = primary ("[" expr "]")*
//                | primary ("." ident | "->" ident)*
//    primary     = num | ident ("(" (expr ("," expr)*)? ")")? | "(" expr ")"
//                | string
//    ident	      = /[a-z][a-z0-9]*/
//    num         = [0-9]+
//    string      = /"[^"]*"/

Node *parse_expr();

static Node *parse_primary() {
  if (token_consume_punct("(")) {
    Node *node = parse_expr();
    token_expect_punct(")");
    return node;
  }

  // parse_ident 相当
  Token *tok = token_consume(TK_IDENT);
  if (tok != NULL) {
    if (token_consume_punct("(")) {
      // 関数呼び出しだった

      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_CALL;
      node->ident = tok;
      node->source_pos = tok->str;
      node->source_len = tok->len;

      if (token_consume_punct(")")) {
        // 引数ナシ
      } else {
        NodeList head = {};
        NodeList *cur = &head;
        while (true) {
          NodeList *node_item = calloc(1, sizeof(NodeList));
          node_item->node = parse_expr();
          cur->next = node_item;
          cur = cur->next;

          if (token_consume_punct(")")) {
            break;
          }

          token_expect_punct(",");
        }

        node->nodes = head.next;
      }

      return node;
    }

    Var *lvar = find_var(curr_func_scope->locals, tok->str, tok->len);
    if (lvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_LVAR;
      node->lvar = lvar;
      node->source_pos = tok->str;
      node->source_len = tok->len;

      // ここで生の配列を見つけた場合は &a を返す、というコードを書いていたが
      // int a[100]; のとき a は int へのポインタ int * だが
      // &a は int[100] へのポインタ int (*)[100] になり
      // 別物なのでやめた

      return node;
    }

    Var *gvar = find_var(&globals, tok->str, tok->len);
    if (gvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_GVAR;
      node->gvar = gvar;
      node->source_pos = tok->str;
      node->source_len = tok->len;
      return node;
    }

    error("variable not found: '%.*s'", tok->len, tok->str);
  }

  Token *tok_str = token_consume(TK_STRING);
  if (tok_str) {
    String *str_lit = add_string(tok_str->str, tok_str->len);
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_STRING;
    node->val = str_lit->index;
    node->source_pos = tok_str->str;
    node->source_len = tok_str->len;
    return node;
  }

  int val = token_expect_number();
  return new_node_num(val);
}

Type int_type = {TY_INT, NULL};
Type char_type = {TY_CHAR, NULL};

static Type *new_type_ptr_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_PTR;
  type->base = base;
  return type;
}

static Type *new_type_array_of(Type *base, int size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_ARRAY;
  type->base = base;
  type->array_size = size;
  return type;
}

Type *typeof_node(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    return &int_type;

  case ND_ADD:
  case ND_SUB: {
    Type *ltype = typeof_node(node->lhs);
    Type *rtype = typeof_node(node->rhs);

    if (ltype->ty == TY_ARRAY)
      ltype = new_type_ptr_to(ltype->base);
    if (rtype->ty == TY_ARRAY)
      rtype = new_type_ptr_to(rtype->base);

    if (ltype->ty == TY_INT && rtype->ty == TY_INT) {
      return &int_type;
    }
    if (ltype->ty == TY_PTR && rtype->ty == TY_INT) {
      return ltype;
    }
    if (ltype->ty == TY_INT && rtype->ty == TY_PTR) {
      return rtype;
    }

    error_at(node->source_pos,
             "invalid or unimplemented pointer arithmetic: %s and %s",
             type_to_string(ltype), type_to_string(rtype));
  }

  case ND_MUL:
  case ND_DIV:
    return &int_type;

  case ND_CALL:
    // TODO: 関数の戻り値の型を返す
    return &int_type;

  case ND_LVAR:
    return node->lvar->type;

  case ND_DEREF: {
    Type *type = typeof_node(node->lhs);
    if (type->ty == TY_PTR || type->ty == TY_ARRAY) {
      return type->base;
    }

    error_at(node->source_pos, "invalid dereference (or not implemented)");
  }

  case ND_ADDR:
    return new_type_ptr_to(typeof_node(node->lhs));

  case ND_GVAR:
    return node->gvar->type;

  case ND_MEMBER: {
    Type *type = typeof_node(node->lhs);
    if (type->ty != TY_STRUCT) {
      error_at(node->source_pos, "not a struct");
    }

    Var *member = find_var(type->members, node->ident->str, node->ident->len);
    return member->type;
  }

  default:
    error_at(node->source_pos, "typeof_node: unimplemented: %s",
             node_kind_to_str(node->kind));
  }
}

int sizeof_type(Type *type) {
  switch (type->ty) {
  case TY_CHAR:
    return 1;
  case TY_INT:
    return 4;
  case TY_PTR:
    return 8;
  case TY_ARRAY:
    return type->array_size * sizeof_type(type->base);
  case TY_STRUCT: {
    Var *m = type->members;
    if (m == NULL) {
      error("sizeof: empty struct");
    }
    while (m->next)
      m = m->next;
    return m ? m->offset + sizeof_type(m->type) : 0;
  }
  }
}

static Node *parse_postfix() {
  Node *node = parse_primary();

  for (;;) {
    if (token_consume_punct("[")) {
      Node *expr = parse_expr();
      token_expect_punct("]");
      node = new_node(ND_DEREF, new_node(ND_ADD, node, expr), NULL);
      continue;
    }

    if (token_consume_punct(".")) {
      Token *ident = token_consume(TK_IDENT);
      if (ident == NULL) {
        error("expected identifier after '.'");
      }
      node = new_node(ND_MEMBER, node, NULL);
      node->ident = ident;
      continue;
    }

    if (token_consume_punct("->")) {
      Token *ident = token_consume(TK_IDENT);
      if (ident == NULL) {
        error("expected identifier after '.'");
      }
      Node *lhs = new_node(ND_DEREF, node, NULL);
      node = new_node(ND_MEMBER, lhs, NULL);
      node->ident = ident;
      continue;
    }

    break;
  }

  return node;
}

static Node *parse_unary() {
  if (token_consume_punct("+")) {
    return parse_postfix();
  }

  if (token_consume_punct("-")) {
    return new_node(ND_SUB, new_node_num(0), parse_primary());
  }

  if (token_consume_punct("*")) {
    return new_node(ND_DEREF, parse_unary(), NULL);
  }

  if (token_consume_punct("&")) {
    Node *node = parse_unary();
    return new_node(ND_ADDR, node, NULL);
  }

  if (token_consume_punct("!")) {
    Node *node = parse_unary();
    return new_node(ND_NOT, node, NULL);
  }

  if (token_consume(TK_SIZEOF) != NULL) {
    Node *node = parse_unary();
    Type *type = typeof_node(node);
    int size = sizeof_type(type);
    return new_node_num(size);
  }

  return parse_postfix();
}

static Node *parse_mul() {
  Node *node = parse_unary();
  for (;;) {
    if (token_consume_punct("*")) {
      node = new_node(ND_MUL, node, parse_unary());
    } else if (token_consume_punct("/")) {
      node = new_node(ND_DIV, node, parse_unary());
    } else {
      return node;
    }
  }
}

static Node *parse_add() {
  Node *node = parse_mul();
  for (;;) {
    if (token_consume_punct("+")) {
      node = new_node(ND_ADD, node, parse_mul());
    } else if (token_consume_punct("-")) {
      node = new_node(ND_SUB, node, parse_mul());
    } else {
      return node;
    }
  }
}

static Node *parse_relational() {
  Node *node = parse_add();
  for (;;) {
    if (token_consume_punct("<")) {
      node = new_node(ND_LT, node, parse_add());
    } else if (token_consume_punct(">")) {
      node = new_node(ND_LT, parse_add(), node);
    } else if (token_consume_punct("<=")) {
      node = new_node(ND_GE, parse_add(), node);
    } else if (token_consume_punct(">=")) {
      node = new_node(ND_GE, node, parse_add());
    } else {
      return node;
    }
  }
}

static Node *parse_equality() {
  Node *node = parse_relational();
  for (;;) {
    if (token_consume_punct("==")) {
      node = new_node(ND_EQ, node, parse_relational());
    } else if (token_consume_punct("!=")) {
      node = new_node(ND_NE, node, parse_relational());
    } else {
      return node;
    }
  }
}

static Node *parse_or() {
  Node *node = parse_equality();
  for (;;) {
    if (token_consume_punct("||")) {
      node = new_node(ND_LOGOR, node, parse_equality());
    } else {
      return node;
    }
  }
}

static Node *parse_assign() {
  Node *node = parse_or();
  if (token_consume_punct("=")) {
    node = new_node(ND_ASSIGN, node, parse_assign());
  }
  return node;
}

Node *parse_expr() { return parse_assign(); }

Node *parse_stmt();

Node *parse_block() {
  if (token_consume_punct("{")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;

    NodeList head = {};
    NodeList *cur = &head;
    while (!token_consume_punct("}")) {
      NodeList *node_item = calloc(1, sizeof(NodeList));
      node_item->node = parse_stmt();
      cur->next = node_item;
      cur = cur->next;
    }

    node->nodes = head.next;
    return node;
  }

  return NULL;
}

Type *parse_type() {
  Type *type = calloc(1, sizeof(Type));

  if (token_consume_type("int")) {
    type->ty = TY_INT;
  } else if (token_consume_type("char")) {
    type->ty = TY_CHAR;
  } else if (token_consume(TK_STRUCT)) {
    type->ty = TY_STRUCT;

    Token *name = token_consume(TK_IDENT);
    if (name != NULL) {
      type->name = name->str;
      type->name_len = name->len;
    }

    if (token_consume_punct("{")) {
      // ND_FUNCDECL の場合と似てるかも
      type->members = calloc(1, sizeof(Var *));
      while (true) {
        Type *member_type = parse_type();
        if (!member_type) {
          error("expected member type");
        }

        Token *member_name = token_consume(TK_IDENT);
        if (!member_name) {
          error("expected member name");
        }

        add_var(type->members, member_name->str, member_name->len, member_type);

        token_expect_punct(";");

        if (token_consume_punct("}")) {
          break;
        }
      }

      if (name != NULL) {
        type = add_or_find_defined_type(type);
      }
    } else if (name != NULL) {
      // type = "struct A" という感じで本体がない
      // 先行定義？ か既存の構造体を参照しているかどっちか
      type = add_or_find_defined_type(type);
    } else {
      // type = "struct" という感じでおかしい
      error("either struct name nor members not given");
    }
  } else {
    return NULL;
  }

  while (token_consume_punct("*")) {
    Type *type_p = calloc(1, sizeof(Type));
    type_p->ty = TY_PTR;
    type_p->base = type;

    type = type_p;
  }

  return type;
}

Node *parse_stmt() {
  if (token_consume(TK_RETURN) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    node->lhs = parse_expr();
    token_expect_punct(";");
    return node;
  }

  if (token_consume(TK_IF) != NULL) {
    token_expect_punct("(");
    Node *expr = parse_expr();
    token_expect_punct(")");
    Node *stmt = parse_stmt();

    Node *else_stmt = NULL;
    if (token_consume(TK_ELSE) != NULL) {
      else_stmt = parse_stmt();
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    node->label_index = ++label_index;
    node->lhs = expr;
    node->rhs = stmt;
    node->node3 = else_stmt;
    node->source_pos = expr->source_pos;
    node->source_len = expr->source_len;
    return node;
  }

  if (token_consume(TK_WHILE) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    node->label_index = ++label_index;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct("(");
    node->lhs = parse_expr();
    token_expect_punct(")");
    node->rhs = parse_stmt();

    return node;
  }

  if (token_consume(TK_FOR) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    node->label_index = ++label_index;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct("(");

    if (!token_consume_punct(";")) {
      node->lhs = parse_expr();
      token_expect_punct(";");
    }

    if (!token_consume_punct(";")) {
      node->rhs = parse_expr();
      token_expect_punct(";");
    }

    if (!token_consume_punct(")")) {
      node->node3 = parse_expr();
      token_expect_punct(")");
    }

    if (!token_consume_punct(";")) {
      node->node4 = parse_stmt();
    }

    return node;
  }

  if (token_consume(TK_BREAK) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BREAK;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct(";");
    return node;
  }

  if (token_consume(TK_CONTINUE) != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CONTINUE;
    node->source_pos = prev_token->str;
    node->source_len = prev_token->len;
    token_expect_punct(";");
    return node;
  }

  // parse_vardecl
  Type *type = parse_type();
  if (type) {
    Token *tok_var = token_consume(TK_IDENT);
    if (!tok_var) {
      error("expected variable name");
    }

    while (token_consume_punct("[")) {
      int size = token_expect_number();
      token_expect_punct("]");

      type = new_type_array_of(type, size);
    }

    Var *lvar =
        add_var(curr_func_scope->locals, tok_var->str, tok_var->len, type);

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VARDECL;
    node->lvar = lvar;
    node->source_pos = tok_var->str;
    node->source_len = tok_var->len;

    if (token_consume_punct("=")) {
      node->rhs = parse_expr();
    }

    token_expect_punct(";");

    return node;
  }

  Node *block = parse_block();
  if (block) {
    return block;
  }

  Node *node = parse_expr();
  token_expect_punct(";");
  return node;
}

static Node *parse_decl() {
  Type *type = parse_type();
  if (!type) {
    error("expected type");
  }

  Token *ident = token_consume(TK_IDENT);
  if (!ident) {
    // struct S { int i; }; など識別子が登場しない場合、たぶん型の宣言
    //
    if (type->ty == TY_STRUCT) {
      token_expect_punct(";");

      // じつはここでもう処理は済んでしまっている
      // 適当に無害な Node を作って返す
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_NOP;
      return node;
    } else {
      error("expected declaration name");
    }
  }

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNCDECL;
  node->ident = ident;
  node->source_pos = ident->str;
  node->source_len = ident->len;
  node->locals = calloc(1, sizeof(Var *));

  curr_func_scope = node;

  if (token_consume_punct("(")) {
    if (token_consume_punct(")")) {
      // 引数ナシ
    } else {
      NodeList head = {};
      NodeList *cur = &head;
      while (true) {
        Type *type = parse_type();
        if (!type) {
          error("expected argument type");
        }

        Token *tok = token_consume(TK_IDENT);
        if (!tok) {
          error("expected argument name");
        }

        Node *ident = calloc(1, sizeof(Node));
        ident->kind = ND_LVAR;
        Var *lvar = add_var(curr_func_scope->locals, tok->str, tok->len, type);
        ident->lvar = lvar;
        ident->source_pos = tok->str;
        ident->source_len = tok->len;

        NodeList *node_item = calloc(1, sizeof(NodeList));
        node_item->node = ident;
        cur->next = node_item;
        cur = cur->next;

        if (token_consume_punct(")")) {
          break;
        }

        token_expect_punct(",");
      }

      node->args = head.next;
    }

    // TODO: 関数の型を～～

    Node *block = parse_block();
    if (!block) {
      error("expected block");
    }

    node->nodes = block->nodes;

    curr_func_scope = NULL;

    return node;
  }

  // こちらからグローバル変数になります
  node->kind = ND_GVARDECL;

  while (token_consume_punct("[")) {
    // TODO: ここが省略できることもある
    int size = token_expect_number();
    token_expect_punct("]");

    type = new_type_array_of(type, size);
  }

  if (token_consume_punct("=")) {
    // TODO: 文字列リテラルの場合は {} に読み替えることにする
    if (token_consume_punct("{")) {
      if (token_consume_punct("}")) {
        // 空の場合はゼロ初期化と同じなので何もしない
      } else {
        // ここは ND_CALL とよく似てるので parse_expr_list
        // とかにできるとよさそう
        NodeList head = {};
        NodeList *cur = &head;
        while (true) {
          NodeList *node_item = calloc(1, sizeof(NodeList));
          node_item->node = parse_expr();
          cur->next = node_item;
          cur = cur->next;

          if (token_consume_punct("}")) {
            break;
          }

          token_expect_punct(",");
        }

        node->nodes = head.next;
      }
    } else {
      node->rhs = parse_expr();
    }
  }

  token_expect_punct(";");

  Var *gvar = add_var(&globals, ident->str, ident->len, type);
  node->gvar = gvar;

  return node;
}

Node *code[100]; // FIXME

void parse_program() {
  int i = 0;
  while (!token_at_eof()) {
    code[i++] = parse_decl();
  }
  code[i] = NULL;

  if (curr_token != NULL && curr_token->kind != TK_EOF) {
    error("not all tokens are consumed");
  }
}