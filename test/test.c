/*
 * mocc でコンパイルして実行されるテスト
 */

int test_count = 0;
int fail_count = 0;

int is(int expected, int actual, char *message) {
  if (ok(actual == expected, message) == 0) {
    printf("# expected %d but got %d\n", expected, actual);
  }
}

int ok(int ok, char *message) {
  test_count = test_count + 1;
  if (ok) {
    printf("ok - %s\n", message);
    return 1;
  } else {
    printf("not ok - %s\n", message);
    fail_count = fail_count + 1;
    return 0;
  }
}

int main() {
  test_arithmetic();
  test_func();
  test_array();
  test_string_literal();
  test_sizeof();
  test_for_while();
  test_pointer();
  test_global_var();
  test_var();
  test_struct();

  printf("1..%d\n", test_count);

  return fail_count;
}

int test_arithmetic() {
  printf("# arithmetic\n");
  is(1, 1, "1");
  is(36, 4 * 9, "4 * 9");
  is(-3, 1 - 6 * 2 / 3, "1 - 6 * 2 / 3");
  is(0, 0, "0");
  is(42, 42, "42");
  is(21, 5 + 20 - 4, "5+20-4");
  is(41, 12 + 34 - 5, " 12 + 34 - 5 ");
  is(77, (77), "(77)");
  is(6, 1 + (2 + 3), "1+(2+3)");
  is(0, 3 - (2 + 1), "3-(2+1)");
  is(12, 3 * 4, "3*4");
  is(33, (3 + 8) * 3, "(3+8)*3");
  is(47, 5 + 6 * 7, "5+6*7");
  is(15, 5 * (9 - 6), "5*(9-6)");
  is(4, (3 + 5) / 2, "(3+5)/2");
  is(5, (-3 * +5) + 20, "(-3*+5)+20");
  is(1, 1 < (11 * 9), "1<(11*9)");
  is(0, 13 < 13, "13<13");
  is(1, (1 + 1) > 1, "(1+1)>1");
  is(0, 13 > 13, "13 > 13");
  is(1, 13 >= 13, "13 >= 13");
  is(1, 13 <= 13, "13 <= 13");
  is(0, 42 >= 43, "42 >= 43");
  is(1, 123 == 123, "123 == 123");
  is(0, 123 == 321, "123 == 321");
  is(0, 123 != 123, "123 != 123");
  is(1, 123 != 321, "123 != 321");

  is(0, 65536 * 65536, "65536 * 65536 overflows");

  is(0, !42, "!42");
  is(1, !!42, "!!42");
}

int test_func() {
  printf("# func\n");
  is(1, fact(0), "fact(0)");
  is(120, fact(5), "fact(5)");
  is(3628800, fact(10), "fact(10)");

  is(1, fib(1), "fib(1)");
  is(2, fib(2), "fib(2)");
  is(3, fib(3), "fib(3)");
  is(89, fib(10), "fib(10)");

  is(66666, add(33333, 33333), "add(33333, 33333)");
  is(-66666, add(33333, -99999), "add(33333, -99999)");

  is(50000, f_if(1, 50000, 60000), "f_if(1, 50000, 60000)");
  is(60000, f_if(0, 50000, 60000), "f_if(1, 50000, 60000)");

  is(2, f_return_return(1), "f_return_return(1)");

  is(13, f_comment(), "f_comment()");
}

int fact(int n) {
  if (n == 1 || n == 0)
    return 1;
  return n * fact(n - 1);
}

int fib(int n) {
  if (n == 1 || n == 0)
    return 1;
  return fib(n - 2) + fib(n - 1);
}

int add(int x, int y) { return x + y; }

int f_if(int b, int x, int y) {
  if (b) {
    return x;
  } else {
    return y;
  }
}

int f_return_return(int val) {
  return val * 2;
  return val;
}

int f_comment() {
  return 1 + /* 2 + */ 3 // ;
                 * 4;
}

int test_array() {
  printf("# array\n");

  int a[100];

  a[49] = 99;
  a[50] = 999;
  a[51] = 9999;
  is(999, a[50], "a[50] = 999; a[50]");
  is(999, *(a + 50), "a[50] = 999; *(a+50)");

  *a = 12345;
  is(12345, a[0], "*a = 12345; a[0]");
  is(12345, *a, "*a = 12345; *a");

  int *p = a;
  is(12345, *p, "int *p = a; *p");
  is(999, *(p + 50), "int *p = a; *(p+50)");

  int mat[11][13];
  mat[9][11] = 4567;
  is(4567, mat[9][11], "mat[9][11] = 4567; mat[9][11]");
  is(4567, *(mat[9] + 11), "mat[9][11] = 4567; *(mat[9]+11)");
  is(4567, *(*(mat + 9) + 11), "mat[9][11] = 4567; *(*(mat+9)+11)");

  *(*(mat + 5) + 7) = 8901;
  is(8901, mat[5][7], "*(*(mat+5)+7) = 8901; mat[5][7]");
  is(8901, *(mat[5] + 7), "*(*(mat+5)+7) = 8901; *(mat[5]+7)");
  is(8901, *(*(mat + 5) + 7), "*(*(mat+5)+7) = 8901; *(*(mat+5)+7)");

  (*(mat + 2))[4] = 1111;
  is(1111, mat[2][4], "(*(mat+2)[4]) = 1111; mat[2][4]");

  *(mat[8] + 12) = 9999;
  is(9999, mat[8][12], "*(mat[8]+12) = 9999; mat[8][12]");
}

int test_string_literal() {
  printf("# string literal\n");

  char *s = "Hello, world!";

  is(s[0], 72, "s[0] == 72");
  is(s[13], 0, "s[13] == 0");
}

int test_sizeof() {
  printf("# sizeof\n");

  int x;
  char c;
  int a[10];
  int *p;
  int mat[10][10];

  is(4, sizeof(x), "sizeof(x)");
  is(1, sizeof(c), "sizeof(c)");
  is(40, sizeof(a), "sizeof(a)");
  is(8, sizeof(p), "sizeof(p)");
  is(400, sizeof(mat), "sizeof(mat)");
  is(40, sizeof(mat[0]), "sizeof(mat[0])");
  is(8, sizeof(&mat), "sizeof(mat&)");
}

int test_for_while() {
  printf("# for while\n");

  int sum = 0;
  int n;
  for (n = 1; n <= 10; n = n + 1) {
    sum = sum + n;
  }
  is(55, sum, "for 1+..+10");

  int x = 1;
  for (; x < 1000;)
    x = x * 2;
  is(1024, x, "for(;x<1000;) x=x*2;");

  sum = 0;
  for (n = 1; n < 10; n = n + 1) {
    sum = sum + n;
    if (sum > 10)
      break;
  }
  is(15, sum, "for 1+..+9, break at > 10");

  sum = 0;
  n = 1;
  while (n <= 10) {
    sum = sum + n;
    n = n + 1;
  }
  is(55, sum, "while 1+..+10");

  sum = 0;
  n = 1;
  while (n <= 10) {
    sum = sum + n;
    n = n + 1;
    if (n == 10)
      break;
  }
  is(45, sum, "while 1+..+10, break at n == 10");

  sum = 0;
  for (n = 1; n < 10; n = n + 1) {
    if (n == 3)
      continue;
    sum = sum + n;
  }
  is(42, sum, "for 1+..+9, skip 3");
}

int test_pointer() {
  int x;
  int *p = &x;
  int **pp = &p;

  *p = 5656;
  is(5656, x, "*p = 5656; x");

  **pp = 8989;
  is(8989, x, "**pp = 8989; x");

  x = 7777;
  is(7777, *p, "x = 7777; *p");
  is(7777, **pp, "x = 7777; **pp");

  ok(&x > 0, "&x > 0");
}

int gvar_1 = 42;
int gvar_2 = -1;
int gvar_3; // 初期化子なし
int gvar_4[5] = {1, 2, 3};

int test_global_var() {
  is(42, gvar_1, "gvar_1 = 42");
  is(-1, gvar_2, "gvar_2 = -1");
  is(0, gvar_3, "gvar_3");
  is(1, gvar_4[0], "gvar_4 = {1, 2, 3}; gvar_4[0]");
  is(0, gvar_4[4], "gvar_4 = {1, 2, 3}; gvar_4[4]");
}

int test_var() {
  int a = 2;
  int b = a * 3;
  is(2, a, "int a = 2");
  is(6, b, "int b = a * 3");
}

struct Struct1 {
  int a;
  int b;
  char *s;
};

struct Struct2;

struct Struct2 {
  char c;
};

int test_struct() {
  struct Struct1 s1;
  struct Struct1 *sp = &s1;
  struct Struct2 s2;
  s2.c = 123;

  is(0, s1.a, "struct Struct1 s1; s1.a");
  s1.a = 100;
  is(100, s1.a, "s1.a = 100; s1.a");
  s1.s = "foobar";
  is(98, s1.s[3], "s1.s = 'foobar'; s1.s[3]");
  is(100, sp->a, "sp->a");

  char *p = s1.s;
  is(111, p[1], "p = s1.s; p[1]");
}
