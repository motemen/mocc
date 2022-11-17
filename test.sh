#!/bin/bash

riscv_cc=riscv64-$RISCV_HOST-gcc

test_count=0

$riscv_cc -c test/helper.c -o test/helper.o

ok() {
  echo "ok - $*"
}

not_ok() {
  echo "not ok - $*"
  if [ "$TAP_BAIL" = 1 ]; then
    exit 1
  fi
}

compile_program() {
  input="$1"

  ./mocc - <<<"$input" > tmp.s || return $?
  $riscv_cc -static tmp.s test/helper.o -o tmp
}

run_program() {
  input="$1"

  if ! compile_program "$input"; then
    echo "# could not compile $input"
    return 255
  fi

  spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | perl -ne 'print unless $.==1 && /^bbl loader\r$/'
  return "${PIPESTATUS[0]}"
}

assert_program_lives() {
  input="$1"
  run_program "$input"
  # 255 は segmentation fault とかなので
  if [ "$?" -eq 255 ]; then
    (( test_count++ ))
    not_ok "Program exited with 255"
  fi
}

assert_compile_error() {
  error="$1"
  input="$2"

  compile_program "$input" 2> tmp.err

  if [ "$?" -eq 0 ]; then
    (( test_count++ ))
    not_ok "$input -> compile error $error expected, but unexpectedly succeeded"
    return
  fi

  if grep --silent "$error" tmp.err; then
    (( test_count++ ))
    ok "$input -> compile error $error"
  else
    (( test_count++ ))
    not_ok "$input -> compile error $error expected, but got $(cat tmp.err)"
  fi
}

assert_program() {
  expected="$1"
  input="$2"

  run_program "$2"
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    (( test_count++ ))
    ok "$input => $actual"
  else
    (( test_count++ ))
    not_ok "$input => $expected expected, but got $actual"
  fi
}

assert_program_output() {
  string="$1"
  input="$2"

  output=$(run_program "$input")

  if [ $? -ne 0 ]; then
    not_ok "program '$input' unexpectedly failed: $output"
    return
  fi

  if echo "$output" | grep --silent "$string"; then
    (( test_count++ ))
    ok "$input => has output '$string': $output"
  else
    (( test_count++ ))
    not_ok "$input => output '$string' expected, but got '$output'"
  fi
}

assert_expr() {
  assert_program "$1" "int main() { return $2 }"
}

assert() {
  assert_program "$1" "int main() { $2 }"
}

# assert_program 4 '
# int main() {
#   int a;
#   a = 1;
#   // a = 2;
#   a = /* 3 + */ 4;
#   return a;
# }
# '
# assert_program 0 'int main() { if (0) { } else { } return 0; } '
# assert_program 56 'int v; int main() { v = 3456; return v - 3400; }'
# assert_program 0 'int n; int main() { n = 222; }'
# assert_program 0 'int a[64]; int main() { a[0] = 222; }'
# assert_program 0 'int a[64]; int main() { a[10] = 222; }'
# assert_program 0 'int a[64]; int main() { a[63] = 222; }'

# assert_program 0 '
# int map[64];
# int xy_to_index(int x, int y) { return y * 8 + x; }
# int main() {
#   int x;
#   int y;
#   for (y = 0; y < 8; y = y+1) {
#     for (x = 0; x < 8; x = x+1) {
#       map[xy_to_index(x, y)] = x + y;
#     }
#   }
# }
# '

# assert_program 111 'int main() { int x; int y; x = 1; y = 0; if (x || y) return 111; }'
# assert_program 101 'int main() { int a[100]; *a = 1; *(a+99) = 100; return a[0] + a[99]; }'
# assert_program 100 'int main() { int mat[5][3]; *(*(mat+3)+2) = 100; return *(*(mat+3)+2); }'
# assert_program 100 'int main() { int mat[5][3]; mat[3][2] = 100; return mat[3][2]; }'
# assert_program 4 'int add(int x, int y) { return x + y; } int inc(int x) { return x + 1; } int main() { return add(inc(0), inc(inc(inc(0)))); }'
# assert_program 120 'int fact(int n) { if (n == 1) return 1; return n * fact(n-1); } int main() { return fact(5); }'

assert_program_output 'id=5' 'int id(int n) { return n; } int main() { printf("id=%d\n", id(5)); }'
assert_program_output 'fact=120' 'int fact(int n) { if (n == 1) return 1; return n * fact(n-1); } int main() { printf("fact=%d\n", fact(5)); }'
# assert_program 72 'int main() { char *s; s = "Hello, world!"; return s[0]; }'
# assert_program 101 'int main() { char *s; s = "Hello, world!"; return s[1]; }'
# assert_program 0 'int main() { int arr[5]; arr[4]; }'
# assert_program 0 'int main() { int arr[5]; arr[4] = 0; }'
# assert_program 3 'int main() { int x; x = 3; int *y; y = &x; return *y; }'

# assert_program 0 'int main() { int arr[10]; int x; x = arr[9]; }'
# assert_program 0 'int main() { int arr[2]; int i; for (i = 0; i < 2; i = i+1) { arr[i] = 0; } return arr[0]; }'
# assert_program 0 'int main() { int arr[3]; int i; for (i = 0; i < 3; i = i+1) { arr[i] = 0; } return arr[0]; }'
# assert_program 0 'int main() { int arr[4]; int i; for (i = 0; i < 4; i = i+1) { arr[i] = 0; } return arr[0]; }'
# assert_program 0 'int main() { int arr[5]; int i; for (i = 0; i < 5; i = i+1) ; return arr[0]; }'
# assert_program 0 'int main() { int arr[5]; return arr[0]; }'
# assert_program 0 'int main() { int arr[5]; arr[2] = 0; }'
# assert_program 0 'int main() { int arr[5]; arr[3] = 0; }'
# assert_program 0 'int main() { int arr[5]; int i; for (i = 0; i < 5; i = i+1) { arr[i] = 0; } return arr[0]; }'
# assert_program 18 'int main() { int arr[10]; arr[1] = arr[0] = 9; return arr[0] + arr[1]; }'
# assert_program 18 'int main() { int arr[10]; arr[0] = arr[1] = 9; return arr[0] + arr[1]; }'
# assert_program 18 'int main() { int arr[10]; arr[0] = arr[1] = 9; return arr[1] + arr[0]; }'
# assert_program 2 'int main() { int arr[10]; arr[0] = arr[1] = 1; arr[2] = arr[1] + arr[0]; return arr[2]; }'
# assert_program 0 'int main() { int fib[10]; fib[0] = fib[1] = 1; int i; for (i = 2; i < 10; i = i+1) { fib[i] = fib[i-1] + fib[i-2]; } int x; x = fib[9]; }'
# assert_program 55 'int main() { int fib[10]; fib[0] = fib[1] = 1; int i; for (i = 2; i < 10; i = i+1) { fib[i] = fib[i-1] + fib[i-2]; } return fib[9]; }'
# assert_program 5 'int main() { char x[3]; x[0] = 5; x[1] = 9; x[2] = 13; return x[0]; }'
# assert_program 9 'int main() { char x[3]; x[0] = 5; x[1] = 9; x[2] = 13; return x[1]; }'
# assert_program 13 'int main() { char x[3]; x[0] = 5; x[1] = 9; x[2] = 13; return x[2]; }'

# assert_program 5 'int main() { char x[3]; x[2] = 13; x[1] = 9; x[0] = 5; return x[0]; }'
# assert_program 9 'int main() { char x[3]; x[2] = 13; x[1] = 9; x[0] = 5; return x[1]; }'
# assert_program 13 'int main() { char x[3]; x[2] = 13; x[1] = 9; x[0] = 5; return x[2]; }'
# assert_program 13 'int main() { char x[3]; x[2] = 13; x[1] = 9; x[0] = 5; return x[2]; }'

# assert_program 16 'int main() { int a[4]; return sizeof(a); }'
# assert_program 8 'int main() { int a[4]; return sizeof(&a); }'
# assert_program 4 'int main() { int a[4]; return sizeof(a[0]); }'

# assert_program 4 'int main() { char a[4]; return sizeof(a); }'
# assert_program 8 'int main() { char a[4]; return sizeof(&a); }'
# assert_program 1 'int main() { char a[4]; return sizeof(a[0]); }'

# assert_program 0 'int main() { int a[2]; }'
# assert_program 0 'int main() { int a[2]; *a = 1; }'
# assert_program 1 'int main() { int a[2]; *a = 1; return *a; }'
# assert_program 2 'int main() { int a[2]; *a = 1; *(a + 1) = 2; return *(a + 1); }'
# assert_program 1 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p; } '
# assert_program 2 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *(a+1); } '
# assert_program 3 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *a + *(a+1); } '
# assert_program 2 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *(p+1); } '

# assert_program 0 'int main() { int a[2]; *a = 34; }'
# assert_program 0 'int main() { int a[2]; return *a; }'
# assert_program 34 'int main() { int a[2]; *a = 34; return *a; }'
# assert_program 39 'int main() { int a[2]; *(a+0) = 39; return *a; }'

# assert_program 0 'int main() { int a[2]; *(a+0) = 34; *(a+1) = 35; }'
# assert_program 69 'int main() { int a[2]; *(a+0) = 34; *(a+1) = 35; return *(a+0) + *(a+1); }'
# assert_program 71 'int main() { int a[2]; a[0] = 35; a[1] = 36; return a[0] + a[1]; }'
# assert_program 73 'int main() { int a[2]; 0[a] = 36; 1[a] = 37; return 0[a] + 1[a]; }'
# assert_program 0 'int main() { char x[3]; x[2] = -4; *(x+1) = 2; *x = -1; int y; y = 4; return x[2] + y; }'
# assert_program 3 'int main() { char x[3]; x[0] = -1; x[1] = 2; x[2] = -4; int y; y = 4; return x[0] + y; }'
# assert_program 7 'int main() { char s[7]; return sizeof s; }'

# assert_program 0 'int x; int main() { return x; }'
# assert_program 1 'int x; int main() { x = 1; return x; }'
# assert_program 3 'int x; int y; int main() { x = 1; y = 2; return x + y; }'
# assert_program 59 'int x; int y; int main() { int z; x = 3; y = 14; z = 42; return x + y + z; }'
# assert_program 88 'int foo; int main() { int foo; foo = 88; return foo; }'

# assert_program 3 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'
# assert_program 1 'int main() { int a[2]; *a = 1; return *a; }'
# assert_program 2 'int main() { int a[2]; *a = 1; *(a + 1) = 2; return *(a + 1); }'
# assert_program 3 'int main() { int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1); }'
# assert_program 1 'int main() { int x; x = 1; int *p; p = &x; return *p; }'
# assert_program 99 'int main() { int a[1]; *a = 99; int *p; p = a; return *p; }'
# assert_program 98 'int foo() {} int main() { int x; x = 98; foo(); return x; }'

# assert_program 1 'int main() { int a[200]; *a = 1; return *a; }'
# assert_program 1 'int main() { int x; x = 1; int a[10]; return x; }'
# assert_program 40 'int main() { int a[10]; return sizeof a; }'
# assert_program 40 'int main() { int a[10]; return sizeof(a); }'
# assert_program 4 'int main() { int a[10]; return sizeof(*a); }'

# assert_program 5 'int main() { int *p; alloc4(&p, 1, 3, 5, 7); return *(p+2); }'

# assert_program 4 'int main() { int x; return sizeof(x); }'
# assert_program 8 'int main() { int *ptr; return sizeof(ptr); }'
# assert_program 8 'int main() { int x; return sizeof(&x); }'

# assert_program 3 'int main() { int x; int *y; y = &x; *y = 3; return x; }'
# assert_program 5 'int main() { int x; int *p; p = &x; x = 5; return *p; }'
# assert_program 9 'int main() { int x; int *p; int **pp; p = &x; pp = &p; x = 9; return **pp; }'
# assert_program 11 'int main() { int x; int *p; int **pp; p = &x; pp = &p; x = 11; return *p; }'
# assert_program 7 'int main() { int x; int *p; int **pp; p = &x; pp = &p; (**pp) = 7; return x; }'

assert_compile_error "variable not found: 'x'" 'int foo() { int x; } int main() { return x; }'
# assert_program 99 'int foo() { int x; x = 1; bar(); } int bar() { int x; x = 2; } int main() { int x; x = 99; foo(); return x; }'

assert_compile_error "variable not found: 'a'" 'int main() { return a; }'
assert_compile_error "variable not found: 'a'" 'int main() { a = a + 1; int a; }'
assert_compile_error "variable already defined: 'x'" 'int main() { int x; int x; }'
assert_compile_error "variable already defined: 'num'" 'int foo(int num) { int num; }'

assert_compile_error "too many elements in array initializer" 'int a[3] = {1, 2, 3, 4};'
assert_compile_error "array initializer specified but declared is not an array" 'int n = {1, 2, 3, 4};'

assert_compile_error "not in while or for loop" 'int main() { continue; }'

assert_compile_error "either struct name nor members not" 'struct;'
assert_compile_error "empty struct" 'struct A { int a; }; int main() { struct Z a; }'
assert_compile_error "already defined" 'struct A { int a; }; struct A { int b; }; int main() {}'
assert_compile_error "not a valid type" 'void a;'
assert_compile_error "must return value" 'int f() { return; }'

# assert_program 0 'int main() {}'
# assert_program 0 'int main() { return 0; }'
# assert_program 3 'int main() { int x; x = 3; return x; }'
# assert_program_lives 'int main() { int x; x = 3; return &x; }'

# assert_expr 0 '0;'
# assert_expr 42 '42;'
# assert_expr 21 "5+20-4;"
# assert_expr 41 " 12 + 34 - 5 ; "
# assert_expr 77 "(77);"
# assert_expr 6 "1+(2+3);"
# assert_expr 0 "3-(2+1);"
# assert_expr 12 "3*4;"
# assert_expr 33 "(3+8)*3;"
# assert_expr 47 '5+6*7;'
# assert_expr 15 '5*(9-6);'
# assert_expr 4 '(3+5)/2;'
# assert_expr 5 '(-3*+5)+20;'
# assert_expr 1 '1<(11*9);'
# assert_expr 0 '13<13;'
# assert_expr 1 '(1+1)>1;'
# assert_expr 0 '13 > 13;'
# assert_expr 1 '13 >= 13;'
# assert_expr 1 '13 <= 13;'
# assert_expr 0 '42 >= 43;'
# assert_expr 1 '123 == 123;'
# assert_expr 0 '123 == 321;'
# assert_expr 0 '123 != 123;'
# assert_expr 1 '123 != 321;'

# assert 42 'int a; return a = 42;'
# assert 99 'int a; a = 99; return a;'
# assert 100 'int a; a = 99; return a+1;'
# assert 2 'int x; int y; int z; x = 2; y = 200; z = 99; return y-z*x;'
# assert 14 'int a; int b; a = 3; b = 5 * 6 - 8; return a + b / 2;'
# assert 9 'int foo; int bar; int baz; foo = 1; bar = foo + 2; baz = bar * 3; return baz;'
# assert 198 'int x_1; int x_2; x_1 = x_2 = 99; return x_1 + x_2;'
# assert 5 'int a; int b; a = 1; b = a * 2; return b + 3; 999;'
# assert 0 'if (0) return 1; return 0;'
# assert 1 'if (1) return 1; return 0;'
# assert 2 'if (0) return 1; else return 2; return 0;'
# assert 6 'int a; a = 0; while (a < 6) a = a + 1; return a;'
# assert 7 'int a; for (a = 0; a < 7; a = a + 1) ; return a;'
# assert 8 'int a; a = 0; for (; a < 8; ) a = a + 1; return a;'
# assert 10 '{ int a; int b; int c; int d; a = 1; b = 2; c = 3; d = 4; return a + b + c + d; }'
# assert 55 'int sum; sum = 0; int i; i = 1; while (i <= 10) { sum = sum + i; i = i + 1; } return sum;'
# assert 55 'int i; int sum; for (i = 1; i <= 10; i = i + 1) { sum = sum + i; } return sum;'
# assert 1 'int a; int b; if (1) { a = 1; return a; } else { b = 2; return b; }'
# assert 2 'int a; int b; if (0) { a = 1; return a; } else { b = 2; return b; }'

# assert_program 3 'int foo() { return 1; } int main() { return 2+foo(); }'
# assert_program 8 'int double(int x) { return x*2; } int main() { return 2+double(3); }'
# assert_program 100 'int add(int x, int y) { return x+y; } int main() { return add(1, 99); }'
# assert_program 120 'int fact(int n) { if (n == 0) return 1; else return n*fact(n-1); } int main() { return fact(5); }'
# assert_program 0 'int f(int x) { return 0; } int main() { int y; f(1); y = 2; return 0; }'
# assert_program 8 'int fib(int n) { if (n <= 1) return 1; return fib(n-1) + fib(n-2); } int main() { return fib(5); }'
# assert_program 0 'int add(int x,int y) { return x+y; } int main() { int x; x = 0; return x; }'
# assert_program 43 'int inc(int x) { return x+1; } int main() { int result; int x; x = 42; result = inc(x); return result; }'
# assert_program 0 'int add(int x,int y) { return x+y; } int main() { int x; int y; x = 0; y = add(1, 99); return x; }'
# assert_program 0 'int f(int x) {} int main() { int y; y = 2; return 0; }'
# assert_program 0 'int f(int x) {} int main() { f(0); return 0; }'
# assert_program 0 'int f(int x) {} int main() { f(0); }'
# assert_program 0 'int f(int x) {} int main() { int x; int y; x; y; return 0; }'
# assert_program 0 'int f() {} int main() { int y; y=1; f(); return 0; }'
# assert_program 0 'int f() {} int main() { f(); return 0; }'
# assert_program 0 'int f() {} int main() { return 0; }'
# assert_program 0 'int f() {} int main() { int y; y = f(); return 0; }'
# assert_program 0 'int f() {} int main() { int y; f(); y; }'
# assert_program 0 'int f() {} int main() { f(); }'
# assert_program 0 'int f() {} int main() { int y; f(); y=1; return 0; }'

assert_program_output "func1 called" "int main() { func1(); }"
assert_program_output "func2 called 42 + 999 = 1041" "int main() { func2(42, 990+9); }"
assert_program_output "func2 called 9 + 81 = 90" "int main() { int a; a = 9; func2(a, a*a); }"

echo
echo "1..$test_count"
