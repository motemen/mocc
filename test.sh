#!/bin/bash

riscv_cc=riscv64-$RISCV_HOST-gcc

assert_program() {
  expected="$1"
  input="$2"

  echo "# input: $input" >&2

  ./9cv "$input" > tmp.s || exit 1
  $riscv_cc -static -o tmp tmp.s || exit 1

  spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert() {
  assert_program "$1" "main() { $2 }"
}

assert 0 '0;'
assert 42 '42;'
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ; "
assert 77 "(77);"
assert 6 "1+(2+3);"
assert 0 "3-(2+1);"
assert 12 "3*4;"
assert 33 "(3+8)*3;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 5 '(-3*+5)+20;'
assert 1 '1<(11*9);'
assert 0 '13<13;'
assert 1 '(1+1)>1;'
assert 0 '13 > 13;'
assert 1 '13 >= 13;'
assert 1 '13 <= 13;'
assert 0 '42 >= 43;'
assert 1 '123 == 123;'
assert 0 '123 == 321;'
assert 0 '123 != 123;'
assert 1 '123 != 321;'
assert 42 'a = 42;'
# assert 0 'x;'
assert 99 'a = 99; a;'
assert 100 'a = 99; a+1;'
assert 2 'x = 2; y = 200; z = 99; y-z*x;'
assert 14 'a = 3; b = 5 * 6 - 8; a + b / 2;'
assert 9 'foo = 1; bar = foo + 2; baz = bar * 3; baz;'
assert 198 'x_1 = x_2 = 99; x_1 + x_2;'
assert 5 'a = 1; b = a * 2; return b + 3; 999;'
assert 0 'if (0) return 1; return 0;'
assert 1 'if (1) return 1; return 0;'
assert 2 'if (0) return 1; else return 2; return 0;'
assert 6 'a = 0; while (a < 6) a = a + 1; return a;'
assert 7 'for (a = 0; a < 7; a = a + 1) ; return a;'
assert 8 'a = 0; for (; a < 8; ) a = a + 1; return a;'
assert 10 '{ a = 1; b = 2; c = 3; d = 4; return a + b + c + d; }'
assert 55 'sum = 0; i = 1; while (i <= 10) { sum = sum + i; i = i + 1; } return sum;'
assert 55 'for (i = 1; i <= 10; i = i + 1) { sum = sum + i; } return sum;'
assert 1 'if (1) { a = 1; return a; } else { b = 2; return b; }'
assert 2 'if (0) { a = 1; return a; } else { b = 2; return b; }'

assert_program 3 'foo() { return 1; } main() { return 2+foo(); }'

printf '#include <stdio.h>\nvoid foo() { printf("foo called!!!\\n"); } void foo2(int x, int y) { printf("foo2 called!! %%d %%d\\n", x, y); }' | $riscv_cc -xc - -c -o foo.o || exit 1

./9cv "main () { foo(); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo called!!!'; then
  echo "calling foo() failed"
  exit 1
fi

./9cv "main() { foo2(42, 990+9); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo2 called!! 42 999'; then
  echo "calling foo2(42, 990+9) failed"
  exit 1
fi

./9cv "main () { a = 9; foo2(a, a*a); 0; }" > tmp.s || exit 1
$riscv_cc -static tmp.s foo.o -o tmp
if ! spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp | tee /dev/stdout | grep --silent 'foo2 called!! 9 81'; then
  echo "calling foo2 failed"
  exit 1
fi


echo OK
