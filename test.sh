#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  ./9cv "$input" > tmp.s
  riscv64-unknown-elf-gcc -o tmp tmp.s
  spike "$(brew --prefix riscv-pk)/riscv64-unknown-elf/bin/pk" ./tmp
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

echo OK
