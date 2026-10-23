#!/bin/sh
# usage: fn.sh <hexaddr>  - print decompiled function starting at addr
a=$(printf "%08x" 0x$1)
f=local/games/ee/decomp/$(echo $a | cut -c1-4)0000.c
awk -v A="// ==== $a" 'index($0,A)==1{p=1;print;next} p&&/^\/\/ ==== /{exit} p' "$f"
