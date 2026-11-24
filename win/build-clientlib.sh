#!/usr/bin/env bash
# Compila xqmclilib.dll (libreria cliente QMClient para Windows, ANSI + W)
set -u
export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GPLSRC="$XQM_ROOT/gplsrc"
OUT="$SCRIPT_DIR/obj"
mkdir -p "$OUT"

CFLAGS="-std=gnu2x -Wall -Wno-format-nonliteral -I. -I$SCRIPT_DIR/include -DGPL -g -DWIN32_LEAN_AND_MEAN"
# Enlace estatico del runtime GCC/pthread: la DLL no exige DLLs MinGW al cliente.
LSTATIC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic"

cd "$GPLSRC" || exit 1

echo "Compiling qmclilib.c..."
if ! gcc $CFLAGS -c qmclilib.c -o "$OUT/xqmclilib.o" 2> "$OUT/xqmclilib.err"; then
  echo "FAIL compile qmclilib.c"; cat "$OUT/xqmclilib.err"; exit 1
fi
if [ -s "$OUT/xqmclilib.err" ]; then echo "(warnings qmclilib):"; cat "$OUT/xqmclilib.err"; fi

echo "Compiling xqmclilib_w.c..."
if ! gcc $CFLAGS -c "$SCRIPT_DIR/xqmclilib_w.c" -o "$OUT/xqmclilib_w.o" 2> "$OUT/xqmclilib_w.err"; then
  echo "FAIL compile xqmclilib_w.c"; cat "$OUT/xqmclilib_w.err"; exit 1
fi
if [ -s "$OUT/xqmclilib_w.err" ]; then echo "(warnings xqmclilib_w):"; cat "$OUT/xqmclilib_w.err"; fi

echo "Linking xqmclilib.dll..."
if gcc -shared -o "$OUT/xqmclilib.dll" "$OUT/xqmclilib.o" "$OUT/xqmclilib_w.o" \
  -Wl,--out-implib,"$OUT/xqmclilib.lib" $LSTATIC -lws2_32 2> "$OUT/link-xqmclilib.err"; then
  echo "xqmclilib.dll OK"
else
  echo "FAIL link"; cat "$OUT/link-xqmclilib.err"; exit 1
fi
