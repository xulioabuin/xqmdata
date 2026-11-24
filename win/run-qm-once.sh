#!/usr/bin/env bash
export PATH=/mingw64/bin:/usr/bin:/bin:$PATH
export QMSYS='D:\APPS\XQM\XQM\qmsys'
cd /d/APPS/XQM/XQM/qmsys/bin || exit 1

printf "%b" "$1" > /tmp/qmin
timeout -s KILL 15 ./qm.exe -aQMSYS < /tmp/qmin
echo "qm EXIT=$?"
