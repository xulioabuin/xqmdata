#!/usr/bin/env bash
set -u
export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT="$SCRIPT_DIR/obj"
cd "$XQM_ROOT/gplsrc" || exit 1

echo "Compiling xqmclilib_w.c..."
time gcc -std=gnu2x -Wall -Wno-format-nonliteral -I. -I"$SCRIPT_DIR/include" -DGPL -g -DWIN32_LEAN_AND_MEAN \
  -c "$SCRIPT_DIR/xqmclilib_w.c" -o "$OUT/xqmclilib_w.o" 2> "$OUT/xqmclilib_w.err"
rc=$?
echo "rc=$rc"
cat "$OUT/xqmclilib_w.err"

echo "Linking xqmclilib.dll..."
time gcc -shared -o "$OUT/xqmclilib.dll" "$OUT/xqmclilib.o" "$OUT/xqmclilib_w.o" \
  -Wl,--out-implib,"$OUT/xqmclilib.lib" -lws2_32 2> "$OUT/link-xqmclilib.err"
rc=$?
echo "rc=$rc"
cat "$OUT/link-xqmclilib.err"
