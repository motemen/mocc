#!/bin/bash

set -e

CC=${CC-cc}
STAGE=${STAGE-1}

MOCC=./mocc
if [ $STAGE -gt 1 ]; then
  MOCC="spike $RISCV/riscv64-$RISCV_HOST/bin/pk ./self.$((STAGE-1))/mocc"
fi

outdir="self.$STAGE"

rm -rf "$outdir"
mkdir "$outdir"

for c in *.c; do
  echo cc -D__mocc_self__ -P -E "$c" -o "$outdir/${c%.c}.c"
  cc -D__mocc_self__ -P -E "$c" -o "$outdir/${c%.c}.c"
done

for c in $outdir/*.c; do
  echo $MOCC "$c" "> ${c%.c}.s"
  $MOCC "$c" | perl -nle 'print unless $.==1 && /^bbl loader\r$/' > "${c%.c}.s"
done
