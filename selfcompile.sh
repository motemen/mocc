#!/bin/bash

set -e

CC=${CC-cc}
MOCC=${MOCC-./mocc}

rm -rf self
mkdir self

for c in *.c; do
  echo cc -D__mocc_self__ -P -E "$c" -o "self/${c%.c}.c"
  cc -D__mocc_self__ -P -E "$c" -o "self/${c%.c}.c"
done

for c in self/*.c; do
  echo $MOCC "$c" "> ${c%.c}.s"
  $MOCC "$c" | perl -nle 'print unless $.==1 && /^bbl loader\r$/' > "${c%.c}.s"
done
