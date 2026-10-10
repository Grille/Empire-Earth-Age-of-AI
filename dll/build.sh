#!/bin/sh
# Builds eeaimod.dll (32-bit) with llvm-mingw (tools/llvm-mingw, or set CC to another i686 mingw compiler).
# usage: sh build.sh [install]   - "install" also copies the DLL into the game folder (two levels up).
cd "$(dirname "$0")"
CC=${CC:-../tools/llvm-mingw/bin/i686-w64-mingw32-clang}
"$CC" -O2 -Wall -Wextra -shared -static -o eeaimod.dll eeaimod.c -lgdi32 -Wl,--image-base,0x6E000000 || exit 1
if [ "$1" = install ]; then cp eeaimod.dll ../../eeaimod.dll && echo "installed to $(cd ../.. && pwd)/eeaimod.dll"; fi
