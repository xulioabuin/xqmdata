#!/usr/bin/env bash
export PATH=/mingw64/bin:/usr/bin:/bin:$PATH
export QMSYS='D:\APPS\XQM\XQM\qmsys'
cd /d/APPS/XQM/XQM/qmsys/bin || exit 1

./qmlnxd.exe &
p=$!
sleep 2
if kill -0 $p 2>/dev/null; then
  echo "qmlnxd RUNNING"
  kill $p 2>/dev/null
  wait $p 2>/dev/null
else
  wait $p
  echo "qmlnxd EXIT=$?"
fi
