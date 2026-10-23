"""Citizen production against the base-game AI (scripts/eco_ablate.sh logs; dll/ai/citizens.c # cit lines, dll/test/ecolog.c
# eco lines): per variant and condition (cheat, map) the mean over the games for the mod (M) and the vanilla AI (V):
the producers' (Town Center, Capitol) seconds training citizens / something else / nothing, by game phase (E 0-10,
M 10-30, L 30-60 min), and the idle seconds' reasons (cap: no room; target: the citizens wanted there; pfood: the game's
own check refused - under 50 food; pbud: the mod's budget refused; batch: another producer trains the last batch; att:
attention off), citizens trained (tc), workers, mine gatherers / slots near drop sites (mg/ms) and known home mines
without a drop site (mnd), soldiers, the mod's stone+gold+iron income (# spend) and kills - losses, wins.
With -base NAME: paired differences (same seed and seat) against that variant.
usage: python scripts/cit.py [-base NAME] "work/autotest_*_ab-*.csv" """
import csv, glob, re, sys
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
paths = log_paths(args)
PH = (('E', 0, 10), ('M', 10, 30), ('L', 30, 61))
RS = ('cap', 'target', 'pfood', 'pbud', 'batch', 'att', 'rest')

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    work = defaultdict(dict)
    lines = [l for l in txt.splitlines() if not l.startswith('#')]
    hdr = None
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms': hdr = r; continue
        if not hdr or len(r) != len(hdr): continue
        d = dict(zip(hdr, r))
        if d['phase'] == 'tick': work[int(d['player'])][int(d['time_ms']) // 60000] = sum(int(d['list%d' % i]) for i in range(7))
    cit = defaultdict(dict)
    for m in re.finditer(r'^# cit: P(\d+) at (\d+) min: (.*)$', txt, re.M):
        seg = m.group(3).split('|')   # prod / reasons / requests / wanted .. cap (the pop cap) / food
        e = {}
        for i, sg in enumerate(seg):
            for k, v in re.findall(r'(\w+) (-?\d+)', sg): e[k if i != 3 else 'p_' + k] = int(v)
        cit[int(m.group(1))][int(m.group(2))] = e
    eco = defaultdict(dict)
    for m in re.finditer(r'^# eco: P(\d+) at (\d+) min: (.*)$', txt, re.M):
        eco[int(m.group(1))][int(m.group(2))] = {k: int(v) for k, v in re.findall(r'(\w+)=(-?\d+)', m.group(3))}
    inc = {}
    for m in re.finditer(r'^# spend: P(\d+) at \d+ min:.*income F(-?\d+) W(-?\d+) S(-?\d+) G(-?\d+) I(-?\d+)', txt, re.M):
        inc[int(m.group(1))] = [int(x) for x in m.group(2, 3, 4, 5, 6)]
    battle = {}
    for m in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(m.group(1))] = [int(x) for x in m.group(2).split()]
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    mod = [p for p in work if p not in van and p != 1]
    vp = [p for p in work if p in van]
    if not mod or not vp: return None
    end = max(max(w) for w in work.values())
    out = {}
    for side, p in (('M', mod[0]), ('V', vp[0])):
        g = {}
        for ph, a, b in PH:
            rows = [e for mi, e in cit[p].items() if a <= mi < b]
            prod = sum(e.get('prod', 0) for e in rows)
            g['idle' + ph] = sum(e.get('idle', 0) for e in rows) / prod if prod else float('nan')
            g['cit' + ph] = sum(e.get('cit', 0) for e in rows) / prod if prod else float('nan')
            for r in RS: g[r + ph] = sum(e.get(r, 0) for e in rows) / prod if prod else float('nan')
            g['np' + ph] = prod / 60.0 / max(len(rows), 1)
        ec = eco[p]
        def at(mi, k, src=ec):
            ks = [x for x in src if x <= mi]
            return src[max(ks)].get(k, 0) if ks else 0
        g['tc30'] = at(30, 'tc')   # cumulative
        g['tc'] = at(99, 'tc')
        for mi in (20, 30, 60):
            ks = [x for x in work[p] if x <= mi]
            g['w%d' % mi] = work[p][max(ks)] if ks else 0
        for mi in (30, 60):
            g['mg%d' % mi] = sum(at(mi, 'm%sg' % r) for r in 'SGI')
            g['ms%d' % mi] = sum(at(mi, 'm%sslots' % r) for r in 'SGI')
            g['mnd%d' % mi] = sum(at(mi, 'm%snd' % r) for r in 'SGI')
            g['sol%d' % mi] = at(mi, 'soldiers')
        g['metal'] = sum(inc.get(p, [0] * 5)[2:]) if side == 'M' else float('nan')
        g['food'] = inc.get(p, [0] * 5)[0] if side == 'M' else float('nan')
        b = battle.get(p, [0] * 6)
        g['kl'] = b[3] - b[4]
        w = work[p][max(work[p])]
        g['alive'] = float(res.get(p) == 1) if res else float(max(work[p]) >= end - 1 and w > 0)
        out[side] = g
    return out

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
nn = lambda v: [x for x in v if x == x]
mean = lambda v: sum(nn(v)) / len(nn(v)) if nn(v) else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in nn(v)) / max(len(nn(v)) - 1, 1) / len(nn(v))) ** 0.5 if nn(v) else 0
conds = sorted({c for _, c in data})
SHOW = ['np', 'cit', 'idle', 'pfood', 'pbud', 'target', 'cap', 'batch', 'att']
for c in conds:
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for side in 'MV':
            gs = [g[side] for g in games.values()]
            ph = '  '.join('%s: ' % p + ' '.join('%s %.2f' % (k, mean([g[k + p] for g in gs])) for k in SHOW) for p, _, _ in PH[1:])
            wins = sum(1 for g in games.values() if g[side]['alive'] and not g['VM'[side == 'V']]['alive'])
            print('  %-14s %s n=%d won %d  %s' % (v, side, len(gs), wins, ph))
            print('  %-14s %s tc30 %.1f tc %.1f w20/30/60 %.1f/%.1f/%.1f mg/ms30 %.1f/%.1f mg/ms60 %.1f/%.1f mnd60 %.1f sol30/60 %.1f/%.1f metal %.0f food %.0f kl %.1f' % (
                v, side, *[mean([g[k] for g in gs]) for k in ('tc30', 'tc', 'w20', 'w30', 'w60', 'mg30', 'ms30', 'mg60', 'ms60', 'mnd60', 'sol30', 'sol60', 'metal', 'food', 'kl')]))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            D = lambda f: [f(games[k]) - f(bg[k]) for k in keys]
            parts = []
            for name, f in (('idleL', lambda g: g['M']['idleL']), ('idleM', lambda g: g['M']['idleM']), ('tc', lambda g: g['M']['tc']),
                            ('w30', lambda g: g['M']['w30']), ('w60', lambda g: g['M']['w60']), ('mg30', lambda g: g['M']['mg30']),
                            ('mg60', lambda g: g['M']['mg60']), ('sol60', lambda g: g['M']['sol60']), ('metal', lambda g: g['M']['metal']),
                            ('kl', lambda g: g['M']['kl']), ('alive', lambda g: g['M']['alive'] - g['V']['alive'])):
                d = D(f)
                parts.append('%s %+.2f (%.2f)' % (name, mean(d), se(d)))
            print('  d %s - %s (%d paired): %s' % (v, base, len(keys), '  '.join(parts)))
