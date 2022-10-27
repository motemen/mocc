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

echo OK
