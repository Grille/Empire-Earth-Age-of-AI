"""Food sources and mine slots against the base-game AI (scripts/eco_ablate.sh logs, dll/ecolog.c # eco lines): per
variant and condition (cheat, map) the mean over the games for the mod (M) and the vanilla AI (V) at the minutes of
-mins: workers, food gatherers by source (fo forage, fh hunt, ff fields, fb fishing boats, fx other), idle (list 0),
food / wood stock, epoch; hunt list and animals within 30 tiles (hl30, an30, anNew30), forage within 30 tiles (fsF),
fields (fld/fldOK); mines with a drop site within 12 tiles - their slots (6 each), gatherers there (mg) and farther out
(mfar), known mines on the base's continent without a drop site (mnd). The epoch line: the first minute of each epoch.
With -base NAME: paired differences (same seed and seat) of the mod's epoch-up minute, workers and mine gatherers.
usage: python scripts/eco_food.py [-base NAME] [-mins 5,10,20,30,60] [-keys k1,k2] "work/autotest_*_ab-*.csv" """
import csv, glob, re, sys
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
MINS = [int(x) for x in opt('-mins', '5,10,15,20,30,40,60').split(',')]
KEYS = opt('-keys', 'workers,gF,fo,fh,ff,fb,fx,idle,stF,stW,stG,stI,epoch,hl30,an30,anNew30,fsF,fld,fldOK,mslots,mg,mfar,mnd').split(',')
paths = [p for a in args for p in (glob.glob(a) if any(c in a for c in '*?') else [a])]

def load(path):
    txt = open(path, errors='replace').read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    ser = defaultdict(dict)
    lines = [l for l in txt.splitlines() if not l.startswith('#')]
    hdr = None
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms': hdr = r; continue
        if not hdr or len(r) != len(hdr): continue
        d = dict(zip(hdr, r))
        if d['phase'] != 'tick': continue
        p, mi = int(d['player']), int(d['time_ms']) // 60000
        s = ser[p].setdefault(mi, {})
        s['workers'] = sum(int(d['list%d' % i]) for i in range(7))
        s['idle'] = int(d['list0'])
        for i, n in enumerate('xyFWSGI'):
            if i >= 2: s['st' + n] = int(d['stock%d' % i])
        s['epoch'] = int(d['epoch'])
    for m in re.finditer(r'^# eco: P(\d+) at (\d+) min: (.*)$', txt, re.M):
        p, mi = int(m.group(1)), int(m.group(2))
        e = {k: int(v) for k, v in re.findall(r'(\w+)=(-?\d+)', m.group(3))}
        s = ser[p].setdefault(mi, {})
        for k in ('gF', 'fo', 'fh', 'ff', 'fb', 'fx', 'hl30', 'an30', 'anNew30', 'fsF', 'fld', 'fldOK'):
            if k in e: s[k] = e[k]
        if 'mSn' in e:
            s['mslots'] = e['mSslots'] + e['mGslots'] + e['mIslots']
            s['mg'] = e['mSg'] + e['mGg'] + e['mIg']
            s['mfar'] = e['mSfar'] + e['mGfar'] + e['mIfar']
        if 'mSnd' in e: s['mnd'] = e['mSnd'] + e['mGnd'] + e['mInd']
    mod = [p for p in ser if p not in van and p != 1]
    vp = [p for p in ser if p in van]
    if not mod or not vp: return None
    out = {}
    for side, p in (('M', mod[0]), ('V', vp[0])):
        mins, last, row = ser[p], {}, {}
        ep_at = {}
        for mi in range(0, max(mins) + 1 if mins else 0):
            last.update(mins.get(mi, {}))
            if 'epoch' in last and last['epoch'] not in ep_at: ep_at[last['epoch']] = mi
            row[mi] = dict(last)
        out[side] = (row, ep_at)
    return out

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g

def at(g, side, mi, k):
    row = g[side][0]
    ks = [x for x in row if x <= mi]
    return row[max(ks)].get(k) if ks else None
mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
conds = sorted({c for _, c in data})
variants = sorted({v for v, _ in data}, key=lambda v: (v != base, v))
for c in conds:
    for v in variants:
        g = data.get((v, c))
        if not g: continue
        print('== %s %s: %d games' % (v, c, len(g)))
        print('%-10s' % 'key' + ''.join('%8d' % m for m in MINS))
        for k in KEYS:
            for side in 'MV':
                vals = []
                for mi in MINS:
                    xs = [x for x in (at(gg, side, mi, k) for gg in g.values()) if x is not None]
                    vals.append('%8.1f' % mean(xs) if xs else '%8s' % '-')
                if any(x.strip() != '-' for x in vals): print('%-10s' % (k + ' ' + side) + ''.join(vals))
        for side in 'MV':   # the first minute of each epoch reached (games that reached it)
            eps = defaultdict(list)
            for gg in g.values():
                for e, mi in gg[side][1].items(): eps[e].append(mi)
            e0 = min(eps) if eps else 0
            print('epoch-up %s: ' % side + '  '.join('e%d %.1f min (%d/%d)' % (e, mean(eps[e]), len(eps[e]), len(g)) for e in sorted(eps) if e > e0))
        if base and v != base and (base, c) in data:
            b = data[(base, c)]
            common = [s for s in g if s in b]
            parts = []
            for k, mi in (('workers', 20), ('workers', 30), ('workers', 60), ('gF', 10), ('gF', 20), ('mg', 20), ('mg', 40), ('mfar', 40), ('epoch', 30), ('epoch', 60)):
                d = [at(g[s], 'M', mi, k) - at(b[s], 'M', mi, k) for s in common if at(g[s], 'M', mi, k) is not None and at(b[s], 'M', mi, k) is not None]
                if d: parts.append('%s@%d %+.1f (%.1f)' % (k, mi, mean(d), se(d)))
            # the first epoch-up: minute (60 if never)
            def first_up(gg):
                ep = gg['M'][1]; e0 = min(ep) if ep else 0
                return ep.get(e0 + 1, 61)
            d = [first_up(g[s]) - first_up(b[s]) for s in common]
            parts.append('epoch-up min %+.1f (%.1f)' % (mean(d), se(d)))
            print('paired vs %s (n=%d): ' % (base, len(common)) + ', '.join(parts))
