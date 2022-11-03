int board[64];
int check[64];

int pos_to_index(int x, int y) { return y * 8 + x; }

int _check_board_by(int x, int y, int mark, int inc) {
  board[pos_to_index(x, y)] = mark;

  int _x;
  int _y;

  for (_x = 0; _x < 8; _x = _x + 1) {
    check[pos_to_index(_x, y)] = check[pos_to_index(_x, y)] + inc;
  }

  for (_y = 0; _y < 8; _y = _y + 1) {
    check[pos_to_index(x, _y)] = check[pos_to_index(x, _y)] + inc;
  }

  for (_x = 0; _x < 8; _x = _x + 1) {
    for (_y = 0; _y < 8; _y = _y + 1) {
      if (x + y == _x + _y) {
        check[pos_to_index(_x, _y)] = check[pos_to_index(_x, _y)] + inc;
      }
      if (x - y == _x - _y) {
        check[pos_to_index(_x, _y)] = check[pos_to_index(_x, _y)] + inc;
      }
    }
  }
}

int check_board(int x, int y) { _check_board_by(x, y, 1, 1); }
int uncheck_board(int x, int y) { _check_board_by(x, y, 0, -1); }

int print_board() {
  int x;
  int y;
  for (y = 0; y < 8; y = y + 1) {
    for (x = 0; x < 8; x = x + 1) {
      if (board[pos_to_index(x, y)] == 1) {
        putchar(81);
      } else {
        putchar(46);
      }
    }
    putchar(10);
  }
}

int print_check() {
  int x;
  int y;
  for (y = 0; y < 8; y = y + 1) {
    for (x = 0; x < 8; x = x + 1) {
      putchar(48 + check[pos_to_index(x, y)]);
    }
    putchar(10);
  }
}

int solve(int x) {
  if (x == 8) {
    print_board();
    putchar(10);
    return 0;
  }

  int y;
  for (y = 0; y < 8; y = y + 1) {
    if (check[pos_to_index(x, y)] == 0) {
      check_board(x, y);
      solve(x + 1);
      uncheck_board(x, y);
    }
  }
}

int main() {
  int x;
  int y;
  for (y = 0; y < 8; y = y + 1) {
    for (x = 0; x < 8; x = x + 1) {
      board[pos_to_index(x, y)] = 0;
      check[pos_to_index(x, y)] = 0;
    }
  }

  solve(0);
}