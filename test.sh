#!/bin/bash

riscv_cc=riscv64-$RISCV_HOST-gcc

run_program() {
  input="$1"
  echo "# input: $input" >&2

  ./9cv "$input" > tmp.s || exit 1
  $riscv_cc -static -o tmp tmp.s || exit 1

  spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp
  return "$?"
}

assert_program_lives() {
  input="$1"
  run_program "$input"
  # 255 は segmentation fault とかなので
  if [ "$?" -eq 255 ]; then
    echo "Program exited with 255" >&2
    exit 1
  fi
}

assert_compile_error() {
  error="$1"
  input="$2"

  echo "# input: $input" >&2

  error_got=$(./9cv "$input" 2>&1)
  
  if [ $? -ne 1 ]; then
    echo "$input -> compile error $error expected, but unexpectedly succeeded"
    exit 1
  fi

  if echo "$error_got" | grep "$error"; then
    echo "$input -> compile error $error"
  else
    echo "$input -> compile error $error expected, but got $error_got"
    exit 1
  fi
}

assert_program() {
  expected="$1"
  input="$2"

  run_program "$2"
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert_expr() {
  assert_program "$1" "int main() { return $2 }"
}

assert() {
  assert_program "$1" "int main() { $2 }"
}

# おかしなテストだった
assert_program 3 'int main() { int x; x = 3; int y; y = &x; return *y; }'

assert_program 3 'int main() { int x; int *y; y = &x; *y = 3; return x; }'
assert_program 5 'int main() { int x; int *p; p = &x; x = 5; return *p; }'
assert_program 9 'int main() { int x; int *p; int **pp; p = &x; pp = &p; x = 9; return **pp; }'
assert_program 11 'int main() { int x; int *p; int **pp; p = &x; pp = &p; x = 11; return *p; }'
assert_program 7 'int main() { int x; int *p; int **pp; p = &x; pp = &p; (**pp) = 7; return x; }'

assert_compile_error "variable not found: 'x'" 'int foo() { int x; } int main() { return x; }'
assert_program 99 'int foo() { int x; x = 1; bar(); } int bar() { int x; x = 2; } int main() { int x; x = 99; foo(); return x; }'

assert_compile_error "variable not found: 'a'" 'int main() { return a; }'
assert_compile_error "variable not found: 'a'" 'int main() { a = a + 1; int a; }'
assert_compile_error "variable already defined: 'x'" 'int main() { int x; int x; }'
assert_compile_error "variable already defined: 'num'" 'int foo(int num) { int num; }'

assert_program 0 'int main() {}'
assert_program 0 'int main() { return 0; }'
assert_program 3 'int main() { int x; x = 3; return x; }'
assert_program_lives 'int main() { int x; x = 3; return &x; }'

assert_expr 0 '0;'
assert_expr 42 '42;'
assert_expr 21 "5+20-4;"
assert_expr 41 " 12 + 34 - 5 ; "
assert_expr 77 "(77);"
assert_expr 6 "1+(2+3);"
assert_expr 0 "3-(2+1);"
assert_expr 12 "3*4;"
assert_expr 33 "(3+8)*3;"
assert_expr 47 '5+6*7;'
assert_expr 15 '5*(9-6);'
assert_expr 4 '(3+5)/2;'
assert_expr 5 '(-3*+5)+20;'
assert_expr 1 '1<(11*9);'
assert_expr 0 '13<13;'
assert_expr 1 '(1+1)>1;'
assert_expr 0 '13 > 13;'
assert_expr 1 '13 >= 13;'
assert_expr 1 '13 <= 13;'
assert_expr 0 '42 >= 43;'
assert_expr 1 '123 == 123;'
assert_expr 0 '123 == 321;'
assert_expr 0 '123 != 123;'
assert_expr 1 '123 != 321;'

assert 42 'int a; return a = 42;'
assert 99 'int a; a = 99; return a;'
assert 100 'int a; a = 99; return a+1;'
assert 2 'int x; int y; int z; x = 2; y = 200; z = 99; return y-z*x;'
assert 14 'int a; int b; a = 3; b = 5 * 6 - 8; return a + b / 2;'
assert 9 'int foo; int bar; int baz; foo = 1; bar = foo + 2; baz = bar * 3; return baz;'
assert 198 'int x_1; int x_2; x_1 = x_2 = 99; return x_1 + x_2;'
assert 5 'int a; int b; a = 1; b = a * 2; return b + 3; 999;'
assert 0 'if (0) return 1; return 0;'
assert 1 'if (1) return 1; return 0;'
assert 2 'if (0) return 1; else return 2; return 0;'
assert 6 'int a; a = 0; while (a < 6) a = a + 1; return a;'
assert 7 'int a; for (a = 0; a < 7; a = a + 1) ; return a;'
assert 8 'int a; a = 0; for (; a < 8; ) a = a + 1; return a;'
assert 10 '{ int a; int b; int c; int d; a = 1; b = 2; c = 3; d = 4; return a + b + c + d; }'
assert 55 'int sum; sum = 0; int i; i = 1; while (i <= 10) { sum = sum + i; i = i + 1; } return sum;'
assert 55 'int i; int sum; for (i = 1; i <= 10; i = i + 1) { sum = sum + i; } return sum;'
assert 1 'int a; int b; if (1) { a = 1; return a; } else { b = 2; return b; }'
assert 2 'int a; int b; if (0) { a = 1; return a; } else { b = 2; return b; }'

assert_program 3 'int foo() { return 1; } int main() { return 2+foo(); }'
assert_program 8 'int double(int x) { return x*2; } int main() { return 2+double(3); }'
assert_program 100 'int add(int x, int y) { return x+y; } int main() { return add(1, 99); }'
assert_program 120 'int fact(int n) { if (n == 0) return 1; else return n*fact(n-1); } int main() { return fact(5); }'
assert_program 0 'int f(int x) { return 0; } int main() { int y; f(1); y = 2; return 0; }'
assert_program 8 'int fib(int n) { if (n <= 1) return 1; return fib(n-1) + fib(n-2); } int main() { return fib(5); }'
assert_program 0 'int add(int x,int y) { return x+y; } int main() { int x; x = 0; return x; }'
assert_program 43 'int inc(int x) { return x+1; } int main() { int result; int x; x = 42; result = inc(x); return result; }'
assert_program 0 'int add(int x,int y) { return x+y; } int main() { int x; int y; x = 0; y = add(1, 99); return x; }'
assert_program 0 'int f(int x) {} int main() { int y; y = 2; return 0; }'
assert_program 0 'int f(int x) {} int main() { f(0); return 0; }'
assert_program 0 'int f(int x) {} int main() { f(0); }'
assert_program 0 'int f(int x) {} int main() { int x; int y; x; y; return 0; }'
assert_program 0 'int f() {} int main() { int y; y=1; f(); return 0; }'
assert_program 0 'int f() {} int main() { f(); return 0; }'
assert_program 0 'int f() {} int main() { return 0; }'
assert_program 0 'int f() {} int main() { int y; y = f(); return 0; }'
assert_program 0 'int f() {} int main() { int y; f(); y; }'
assert_program 0 'int f() {} int main() { f(); }'
assert_program 0 'int f() {} int main() { int y; f(); y=1; return 0; }'

printf '#include <stdio.h>\nvoid foo() { printf("foo called!!!\\n"); } void foo2(int x, int y) { printf("foo2 called!! %%d %%d\\n", x, y); }' | $riscv_cc -xc - -c -o foo.o || exit 1

./9cv "int main () { foo(); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo called!!!'; then
  echo "calling foo() failed"
  exit 1
fi

./9cv "int main() { foo2(42, 990+9); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo2 called!! 42 999'; then
  echo "calling foo2(42, 990+9) failed"
  exit 1
fi

./9cv "int main () { int a; a = 9; foo2(a, a*a); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo2 called!! 9 81'; then
  echo "calling foo2 failed"
  exit 1
fi


echo OK
