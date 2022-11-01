#include "parse.h"

bool codegen(Node *node);
void codegen_expr(Node *node);
// FIXME これ codegen_expr とか作るようにしたら不要になるはず
void codegen_pop_t0();
