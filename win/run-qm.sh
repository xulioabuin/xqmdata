#!/usr/bin/env bash
export PATH=/mingw64/bin:/usr/bin:/bin:$PATH
export QMSYS='D:\APPS\XQM\XQM\qmsys'
cd /d/APPS/XQM/XQM/qmsys/bin || exit 1

printf 'QUIT\n' | ./qm.exe -aQMSYS
echo "qm EXIT=$?"
