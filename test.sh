#!/bin/bash

set -e

assert() {
  expected="$1"
  input="$2"

  ./9cv "$input" > tmp.s
  riscv64-$RISCV_HOST-gcc -static -o tmp tmp.s

  set +e
  spike "$RISCV/riscv64-$RISCV_HOST/bin/pk" ./tmp
  actual="$?"
  set -e

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

echo OK
