"""Ablation table for the economy against the base-game AI (scripts/test/eco_ablate.sh, docs/testing.md "Against the
base-game AI"): per variant and condition (cheat 0/1, map) the mod's results and, paired by game seed and seat with
the base variant, the differences. Columns: games, mod won / vanilla won, kills - losses (mod), the mod's workers at
20 / 30 / 60 min (tick rows: all resource lists), the mod's citizens killed (# death) and its soldiers at 20 min
(# eco), its epoch at the end; d* = paired difference to base (same seed and seat; standard error).
usage: python scripts/results/eco_ablate.py [-base NAME] "local/work/autotest_*_van*_ab-*.csv"   (tags ab-<variant>-c<cheat>-<map>-r<seed>v<seat>)"""
import csv, glob, re, sys
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
base = 'base'
if '-base' in args: i = args.index('-base'); base = args[i + 1]; del args[i:i + 2]
paths = log_paths(args)

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    lines = [l for l in txt.splitlines() if not l.startswith('#')]
    hdr, work, epoch, end = None, defaultdict(dict), defaultdict(int), 0
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms': hdr = r; continue
        if not hdr or len(r) != len(hdr) or r[2] != 'tick': continue
        p, t = int(r[1]), int(r[0])
        work[p][t // 60000] = sum(int(r[hdr.index('list%d' % i)]) for i in range(7))
        epoch[p] = max(epoch[p], int(r[-1])); end = max(end, t)
    mod = [p for p in work if p not in van and p != 1]
    if not mod: return None
    p = mod[0]; v = [q for q in work if q in van][0]
    def at(q, mi):
        ks = [k for k in work[q] if k <= mi]
        return work[q][max(ks)] if ks else 0
    battle = {int(m.group(1)): [int(x) for x in m.group(2).split()] for m in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M)}
    over = re.search(r'^# autotest: game over at [\d.]+[^:]*: (.*)$', txt, re.M)
    res = {int(a): int(b) for a, b in re.findall(r'P(\d+)=(\d+)', over.group(1))} if over else {}
    alive = lambda q: res.get(q) == 1 if res else at(q, 999) > 0
    won = 'M' if alive(p) and not alive(v) else 'V' if alive(v) and not alive(p) else 'D'
    b = battle.get(p, [0] * 6)
    sold20 = 0
    for m in re.finditer(r'^# eco: P%d at (\d+) min: .*?soldiers=(-?\d+)' % p, txt, re.M):
        if int(m.group(1)) <= 20: sold20 = int(m.group(2))
    dead = len(re.findall(r'^# death: P%d citizen' % p, txt, re.M))
    return dict(won=won, kl=b[3] - b[4], w20=at(p, 20), w30=at(p, 30), w60=at(p, 60), vw30=at(v, 30), dead=dead, sold20=sold20, epoch=epoch[p])

data = defaultdict(dict)   # (variant, cond) -> {(seed, seat): result}
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
keys = ['kl', 'w20', 'w30', 'w60', 'vw30', 'dead', 'sold20', 'epoch']
se = lambda v: (sum((x - sum(v) / len(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
conds = sorted({c for _, c in data})
variants = sorted({v for v, _ in data}, key=lambda v: (v != base, v))
for c in conds:
    print('== %s' % c)
    print('%-9s %4s %7s ' % ('variant', 'n', 'M/V/D') + ' '.join('%7s' % k for k in keys) + '   paired d vs %s: ' % base + ' '.join('%12s' % ('d' + k) for k in ('kl', 'w30', 'w60', 'dead', 'won')))
    for v in variants:
        g = data.get((v, c))
        if not g: continue
        n = len(g)
        w = ''.join(str(sum(1 for r in g.values() if r['won'] == x)) + '/' for x in 'MVD')[:-1]
        line = '%-9s %4d %7s ' % (v, n, w) + ' '.join('%7.1f' % (sum(r[k] for r in g.values()) / n) for k in keys)
        b = data.get((base, c), {})
        common = [s for s in g if s in b]
        if v != base and common:
            parts = []
            for k in ('kl', 'w30', 'w60', 'dead'):
                d = [g[s][k] - b[s][k] for s in common]
                parts.append('%+6.1f(%4.1f)' % (sum(d) / len(d), se(d)))
            sc = {'M': 1, 'D': 0, 'V': -1}
            d = [sc[g[s]['won']] - sc[b[s]['won']] for s in common]
            parts.append('%+6.2f(%4.2f)' % (sum(d) / len(d), se(d)))
            line += '   ' + ' '.join(parts) + ' n=%d' % len(common)
        print(line)
