# Compare two Ghidra dumps (functions.tsv: addr, name, size) by RTTI function names (Class::vfN):
# which classes' code changed between the base game and Art of Conquest.
# usage: python diff_exes.py decomp/functions.tsv aoc/decomp/functions.tsv [class-prefix-filter]
import sys, re
from collections import defaultdict

def load(p):
    out = {}
    for l in open(p, encoding='utf-8', errors='replace'):
        f = l.rstrip('\n').split('\t')
        if len(f) >= 3 and '::vf' in f[1]:
            out[f[1]] = int(f[2])
    return out

a, b = load(sys.argv[1]), load(sys.argv[2])
filt = sys.argv[3] if len(sys.argv) > 3 else ''
SPACE = re.compile(r'Space|Planet|Cloak|MindControl|Meteor|Conscription|LogicBomb|CivPower|Fanatic|JIT|Carrier')
per = defaultdict(lambda: [0, 0, 0, 0])   # same, changed, bytes base, bytes delta
changed = []
for name in sorted(set(a) & set(b)):
    cls = name.split('::')[0]
    if filt and not cls.startswith(filt):
        continue
    sa, sb = a[name], b[name]
    st = per[cls]
    st[2] += sa
    if abs(sa - sb) <= max(4, sa // 50):
        st[0] += 1
    else:
        st[1] += 1
        st[3] += abs(sb - sa)
        changed.append((abs(sb - sa), name, sa, sb))
tot_same = sum(v[0] for v in per.values()); tot_chg = sum(v[1] for v in per.values())
print('shared named functions: %d same size, %d changed' % (tot_same, tot_chg))
groups = defaultdict(lambda: [0, 0])
for cls, (s, c, _, d) in per.items():
    g = re.match(r'(EECP|EEAI|EEUI|EEU|EEM|EEA|EES|EEE|EEP|EE)?', cls).group(1) or 'other'
    groups[g][0] += s; groups[g][1] += c
print('by subsystem (same/changed): ' + ', '.join('%s %d/%d' % (g, s, c) for g, (s, c) in sorted(groups.items())))
print('\nclasses with the most changed code (space content excluded):')
for cls, (s, c, bb, d) in sorted(per.items(), key=lambda kv: -kv[1][3])[:45]:
    if c and not SPACE.search(cls):
        print('  %-50s %3d same %3d changed, delta %6d bytes' % (cls, s, c, d))
print('\nlargest changed functions:')
for d, name, sa, sb in sorted(changed, reverse=True)[:40]:
    if not SPACE.search(name):
        print('  %-55s %6d -> %6d' % (name, sa, sb))
