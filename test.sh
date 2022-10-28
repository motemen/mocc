#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  echo "# input: $input" >&2

  ./9cv "$input" > tmp.s || exit 1
  riscv64-$RISCV_HOST-gcc -static -o tmp tmp.s || exit 1

  spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 41 " 12 + 34 - 5 "
assert 77 "(77)"
assert 6 "1+(2+3)"
assert 0 "3-(2+1)"
assert 12 "3*4"
assert 33 "(3+8)*3"
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 5 '(-3*+5)+20'
assert 1 '1<(11*9)'
assert 0 '13<13'
assert 1 '(1+1)>1'
assert 0 '13 > 13'
assert 1 '13 >= 13'
assert 1 '13 <= 13'
assert 0 '42 >= 43'
assert 1 '123 == 123'
assert 0 '123 == 321'
assert 0 '123 != 123'
assert 1 '123 != 321'
assert 42 'a = 42'
assert 0 'x'

echo OK
