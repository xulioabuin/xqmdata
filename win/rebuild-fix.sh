#!/usr/bin/env bash
# Recompila los modulos indicados (o los dados por argumento) y relinka qm.exe
# Uso: ./rebuild-fix.sh [modulo1 modulo2 ...]
set -u
export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GPLSRC="$XQM_ROOT/gplsrc"
OUT="$SCRIPT_DIR/obj"
CFLAGS="-std=gnu2x -Wall -Wno-format-nonliteral -I. -I$SCRIPT_DIR/include -DGPL -g"
# Enlace estatico del runtime GCC/pthread: binarios sin DLLs MinGW.
LSTATIC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic"

cd "$GPLSRC" || exit 1

if [ $# -gt 0 ]; then
  MODS="$@"
else
  MODS="dh_open op_dio1 linuxio"
fi

for m in $MODS; do
  echo "Compiling $m.c..."
  if ! gcc $CFLAGS -c "$m.c" -o "$OUT/$m.o" 2> "$OUT/$m.err"; then
    echo "FAIL $m"; cat "$OUT/$m.err"; exit 1
  fi
  if [ -s "$OUT/$m.err" ]; then echo "(warnings) $m:"; cat "$OUT/$m.err"; fi
done

echo "Compiling winipc.c..."
if ! gcc $CFLAGS -c "$SCRIPT_DIR/winipc.c" -o "$OUT/winipc.o" 2> "$OUT/winipc.err"; then
  echo "FAIL winipc"; cat "$OUT/winipc.err"; exit 1
fi
if [ -s "$OUT/winipc.err" ]; then echo "(warnings) winipc:"; cat "$OUT/winipc.err"; fi

QMOBJS=""
for m in $(cat "$XQM_ROOT/gpl.src"); do
  QMOBJS="$QMOBJS $OUT/$m.o"
done
QMOBJS="$QMOBJS $OUT/winipc.o"

echo "Linking qm.exe..."
if gcc -o "$OUT/qm.exe" $QMOBJS $LSTATIC -lws2_32 -lm 2> "$OUT/link-qm.err"; then
  echo "qm.exe OK"
else
  echo "qm.exe FAIL"; head -n 60 "$OUT/link-qm.err"; exit 1
fi

echo "Linking qmlnxd.exe..."
if gcc -o "$OUT/qmlnxd.exe" "$OUT/qmlnxd.o" "$OUT/qmsem.o" "$OUT/winipc.o" \
  $LSTATIC -lws2_32 -lm 2> "$OUT/link-qmlnxd.err"; then
  echo "qmlnxd.exe OK"
else
  echo "qmlnxd.exe FAIL"; head -n 60 "$OUT/link-qmlnxd.err"; exit 1
fi

echo "Building qmteld.exe..."
if gcc -O2 -o "$OUT/qmteld.exe" "$SCRIPT_DIR/qmteld.c" $LSTATIC -lws2_32 2> "$OUT/qmteld.err"; then
  echo "qmteld.exe OK"
else
  echo "qmteld.exe FAIL"; head -n 60 "$OUT/qmteld.err"; exit 1
fi
