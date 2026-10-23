# Compare the float constants of functions with the same RTTI name in two Ghidra dumps (e.g. EE vs AoC):
# finds layout/tuning changes that don't change a function's size.
# usage: python diff_consts.py local/games/ee/decomp local/games/aoc/decomp [class-prefix]
import re, glob, sys
from collections import defaultdict

FLOAT = re.compile(r'(?<![\w.])-?\d+\.\d+(?:e[-+]?\d+)?')

def funcs(d):
    out = {}
    cur, buf = None, []
    for f in sorted(glob.glob(d + '/*.c')):
        for line in open(f, encoding='utf-8', errors='replace'):
            if line.startswith('// ==== '):
                if cur: out[cur] = buf
                name = line[8:].strip().split(' ', 1)[-1]
                cur, buf = (name if '::vf' in name else None), []
            elif cur:
                buf.extend(FLOAT.findall(line))
    if cur: out[cur] = buf
    return out

a, b = funcs(sys.argv[1]), funcs(sys.argv[2])
pref = sys.argv[3] if len(sys.argv) > 3 else ''
n = 0
for name in sorted(set(a) & set(b)):
    if not name.startswith(pref) or a[name] == b[name]:
        continue
    n += 1
    print('%-50s %s\n%50s %s' % (name, ' '.join(a[name][:16]), 'aoc', ' '.join(b[name][:16])))
print(n, 'functions with different float constants')
