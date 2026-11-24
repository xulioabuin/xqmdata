#!/usr/bin/env bash
# Link the Windows engine binaries from the objects produced by compile-all.sh.
set -u

export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GPLSRC="$XQM_ROOT/gplsrc"

OUT="$SCRIPT_DIR/obj"
cd "$GPLSRC" || exit 1

# Enlace estatico del runtime GCC/pthread: binarios sin DLLs MinGW.
LSTATIC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic"

QMOBJS=""
for m in $(cat "$XQM_ROOT/gpl.src"); do
  QMOBJS="$QMOBJS $OUT/$m.o"
done
QMOBJS="$QMOBJS $OUT/winipc.o"

echo "Linking qm.exe..."
gcc -o "$OUT/qm.exe" $QMOBJS $LSTATIC -lws2_32 -lm 2> "$OUT/link-qm.err"
if [ $? -eq 0 ]; then
  echo "qm.exe OK"
else
  echo "qm.exe FAIL"
  head -n 60 "$OUT/link-qm.err"
fi

echo "Linking qmlnxd.exe..."
gcc -o "$OUT/qmlnxd.exe" "$OUT/qmlnxd.o" "$OUT/qmsem.o" "$OUT/winipc.o" \
  $LSTATIC -lws2_32 -lm 2> "$OUT/link-qmlnxd.err"
if [ $? -eq 0 ]; then
  echo "qmlnxd.exe OK"
else
  echo "qmlnxd.exe FAIL"
  head -n 60 "$OUT/link-qmlnxd.err"
fi

echo "Building qmteld.exe..."
if gcc -O2 -o "$OUT/qmteld.exe" "$SCRIPT_DIR/qmteld.c" $LSTATIC -lws2_32 2> "$OUT/qmteld.err"; then
  echo "qmteld.exe OK"
else
  echo "qmteld.exe FAIL"
  head -n 60 "$OUT/qmteld.err"
fi
