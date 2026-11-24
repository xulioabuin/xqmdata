#!/usr/bin/env bash
export PATH=/mingw64/bin:$PATH
echo 'int main(){return 42;}' > /tmp/h.c
gcc -o /tmp/h.exe /tmp/h.c
echo "gcc exit=$?"
ls -la /tmp/h.exe 2>/dev/null || echo "no exe"
/tmp/h.exe; echo "run exit=$?"
