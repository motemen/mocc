#!/bin/bash

CC=${CC-cc}

rm -rf self
mkdir self

for c in *.c; do
  echo cc -D__mocc_self__ -P -E "$c" -o "self/${c%.c}.c"
  cc -D__mocc_self__ -P -E "$c" -o "self/${c%.c}.c"
done

for c in self/*.c; do
  echo ./mocc "$c" "> ${c%.c}.s"
  ./mocc "$c" > ${c%.c}.s
done
