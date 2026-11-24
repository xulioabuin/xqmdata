#!/usr/bin/env bash
# Compile every engine module for Windows (MinGW-w64) and report failures.
set -u

export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GPLSRC="$XQM_ROOT/gplsrc"

cd "$GPLSRC" || exit 1

OUT="$SCRIPT_DIR/obj"
rm -rf "$OUT"
mkdir -p "$OUT"
: > "$OUT/fails.txt"
ok=0
fail=0

CFLAGS="-std=gnu2x -Wall -Wno-format-nonliteral -I. -I$SCRIPT_DIR/include -DGPL -g"

for m in $(cat "$XQM_ROOT/gpl.src") qmlnxd; do
  if gcc $CFLAGS -c "$m.c" -o "$OUT/$m.o" 2> "$OUT/$m.err"; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
    echo "$m" >> "$OUT/fails.txt"
  fi
done

if gcc $CFLAGS -c "$SCRIPT_DIR/winipc.c" -o "$OUT/winipc.o" 2> "$OUT/winipc.err"; then
  ok=$((ok + 1))
else
  fail=$((fail + 1))
  echo "winipc" >> "$OUT/fails.txt"
fi

echo "OK=$ok FAIL=$fail"
echo "=== failing modules ==="
cat "$OUT/fails.txt"
