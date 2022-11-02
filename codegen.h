#include "parse.h"

void codegen_preamble();
bool codegen(Node *node);
void codegen_expr(Node *node);
// FIXME これ codegen_expr とか作るようにしたら不要になるはず
void codegen_pop_t0();
