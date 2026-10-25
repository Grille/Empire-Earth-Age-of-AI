#!/bin/sh
# Stages dll/game/api/gen/ as generated from HEAD's spec plus the files named here (this session's spec/, gen_api.py,
# spec_read.py, types.h changes) - so a commit carries no other session's spec changes in gen/; the working tree's
# gen/ stays as it is.
# usage: sh scripts/build/stage_gen.sh spec/game/ui.cs spec/game/world.cs ...   (then git add your other files by name)
cd "$(dirname "$0")/../.."
T=local/work/stage_gen
rm -rf $T && mkdir -p $T
git archive HEAD spec scripts/build dll/game/api/types.h | tar -x -C $T
for f in "$@"; do cp "$f" "$T/$f"; done
local/venv/Scripts/python.exe $T/scripts/build/gen_api.py > /dev/null || { echo "gen failed"; exit 1; }
n=0
for f in $T/dll/game/api/gen/*; do
    p=dll/game/api/gen/$(basename $f)
    h=$(git hash-object -w --path=$p $f)
    git update-index --add --cacheinfo 100644,$h,$p
    if [ "$h" != "$(git rev-parse HEAD:$p 2>/dev/null)" ]; then
        n=$((n + 1))
        echo "staged $p"
    fi
done
echo "$n gen files staged"
