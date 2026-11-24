#!/usr/bin/env bash
# Compila las utilidades XQMdata (qmfix, qmconv, qmidx, qmtic) para Windows.
# Enlazado estatico: no requieren DLLs de MinGW para correr.
set -u
export PATH=/mingw64/bin:$PATH
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XQM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GPLSRC="$XQM_ROOT/gplsrc"
OUT="$SCRIPT_DIR/obj"
mkdir -p "$OUT"

CFLAGS="-std=gnu2x -Wall -Wno-format-nonliteral -I. -I$SCRIPT_DIR/include -DGPL -g"
# Enlace estatico del runtime GCC/C++ y pthread: binarios autocontenidos.
LSTATIC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic"

cd "$GPLSRC" || exit 1

# Lista de objetos extra necesarios por las utilidades
NEEDED="inipath ctype linuxlb dh_hash"
for m in $NEEDED; do
  if [ ! -f "$OUT/$m.o" ]; then
    echo "Compiling $m.c..."
    if ! gcc $CFLAGS -c "$m.c" -o "$OUT/$m.o" 2> "$OUT/$m.err"; then
      echo "FAIL $m"; cat "$OUT/$m.err"; exit 1
    fi
  fi
done

build_one() {
  local name="$1"; shift
  local objs="$OUT/$name.o"
  local extra=""
  for o in "$@"; do extra="$extra $OUT/$o.o"; done

  echo "Compiling $name.c..."
  if ! gcc $CFLAGS -c "$name.c" -o "$OUT/$name.o" 2> "$OUT/$name.err"; then
    echo "FAIL compile $name"; cat "$OUT/$name.err"; exit 1
  fi
  if [ -s "$OUT/$name.err" ]; then echo "(warnings) $name:"; cat "$OUT/$name.err"; fi

  echo "Linking $name.exe..."
  if gcc -o "$OUT/$name.exe" "$OUT/$name.o" $extra $LSTATIC -lws2_32 -lm 2> "$OUT/link-$name.err"; then
    echo "$name.exe OK"
  else
    echo "FAIL link $name"; cat "$OUT/link-$name.err"; exit 1
  fi
}

build_one qmtic inipath
# Estas tres incluyen qm.h: enlazan winipc.o (getuid/getpwuid/shm/sem Win32)
build_one qmfix ctype linuxlb dh_hash inipath winipc
build_one qmconv ctype linuxlb dh_hash winipc
build_one qmidx linuxlb winipc
