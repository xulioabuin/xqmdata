#!/usr/bin/env bash
OUT=/tmp/wobj
for m in $(cat "$OUT/fails.txt"); do
  echo "================================================================"
  echo "### $m"
  echo "================================================================"
  # show only error lines and their context, first 40 matching lines
  grep -n -E "error:|Error|undefined reference|conflicting|redefinition|was not declared|expected|no such file" "$OUT/$m.err" | head -n 40
  echo
done
