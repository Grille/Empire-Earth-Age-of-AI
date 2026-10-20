"""Other islands against the base-game AI (dll/expand.c # isl / # isl new / # expand lines; logs of scripts/vanilla.sh
tagged ab-<name>-c<cheat>-<map>-r<seed>v<seat>, as scripts/eco_ablate.sh names them): per variant and condition the
mean over the games for the mod (M) and the vanilla AI (V) at minutes 10/20/30/45/60 - the home island's mines and
the player's miners there (stone + gold + iron sites, gatherers whose site lies there), other islands it holds (own:
its buildings and no enemy's; contest: both), their mines, its miners, drop sites, towers, citizens and soldiers there,
the empty islands' mines, the first minute it had a drop site on another island (settled: share of games, mean
minute), buildings it put off its home island (# isl new), and the end: workers, soldiers, warships, the mod's stone +
gold + iron income (# spend), kills - losses, wins. With -base NAME: paired differences (same seed and seat) against
that variant. With -v: one line per game.
usage: python scripts/islands.py [-base NAME] [-v] "work/autotest_*_ab-<name>-*.csv" ..."""
import csv, glob, re, sys
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
verbose = '-v' in args
args = [a for a in args if a != '-v']
paths = [p for a in args for p in (glob.glob(a) if any(c in a for c in '*?') else [a])]
MINS = (10, 20, 30, 45, 60)
CL = ('home', 'own', 'contest', 'enemy', 'ally', 'empty')

def parse_isl(s):
    out = {}
    for part in s.split('|'):
        m = re.match(r'\s*(\w+) n=(\d+) S(\d+) G(\d+) I(\d+) W(\d+) F(\d+) my F(\d+) W(\d+) S(\d+) G(\d+) I(\d+) oth (\d+) drop (\d+) bld (\d+) tow (\d+) cit (\d+) sold (\d+)', part)
        if m:
            v = [int(x) for x in m.groups()[1:]]
            out[m.group(1)] = dict(n=v[0], mines=v[1] + v[2] + v[3], miners=v[8] + v[9] + v[10], food=v[6], oth=v[11], drop=v[12], bld=v[13],
                                   tow=v[14], cit=v[15], sold=v[16])
        m = re.match(r'\s*tr (\d+) aboard (\d+)', part)
        if m: out['tr'] = int(m.group(1)); out['aboard'] = int(m.group(2))
    return out

def load(path):
    txt = open(path, errors='replace').read()
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
    isl = defaultdict(dict)
    for m in re.finditer(r'^# isl: P(\d+) at (\d+) min:(.*)$', txt, re.M): isl[int(m.group(1))][int(m.group(2))] = parse_isl(m.group(3))
    new = defaultdict(int)
    for m in re.finditer(r'^# isl new: P(\d+) at', txt, re.M): new[int(m.group(1))] += 1
    inc = {}
    for m in re.finditer(r'^# spend: P(\d+) at \d+ min:.*income F(-?\d+) W(-?\d+) S(-?\d+) G(-?\d+) I(-?\d+)', txt, re.M):
        inc[int(m.group(1))] = [int(x) for x in m.group(2, 3, 4, 5, 6)]
    battle, army, navy = {}, {}, {}
    for m in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(m.group(1))] = [int(x) for x in m.group(2).split()]
    for m in re.finditer(r'^# army: P(\d+) at [0-9.]+ min: (\d+) soldiers', txt, re.M): army[int(m.group(1))] = int(m.group(2))
    for m in re.finditer(r'^# navy: P(\d+) at \d+ min: docks.*?\| war (\d+) ', txt, re.M): navy[int(m.group(1))] = int(m.group(2))
    ex = defaultdict(lambda: defaultdict(int))
    for m in re.finditer(r'^# expand: P(\d+) at [\d.]+ min: (.*)$', txt, re.M):
        s = m.group(2)
        for k, pat in (('plans', r'^plans '), ('sails', r'^sails '), ('ashore', r'^ashore '), ('drop', r'drop site .* finished'),
                       ('lost', r'^outpost lost'), ('given', r'^given up'), ('abort', r'called off|no transport - given up|failed')):
            if re.search(pat, s): ex[int(m.group(1))][k] += 1
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    mod = [p for p in work if p not in van and p != 1]
    vp = [p for p in work if p in van]
    if not mod or not vp: return None
    end = max(max(w) for w in work.values())
    out = {}
    for side, p in (('M', mod[0]), ('V', vp[0])):
        g = {}
        def at(mi):
            ks = [x for x in isl[p] if x <= mi]
            return isl[p][max(ks)] if ks else {}
        for mi in MINS:
            e = at(mi)
            h, o, c = e.get('home', {}), e.get('own', {}), e.get('contest', {})
            g['hm%d' % mi] = h.get('mines', 0); g['hg%d' % mi] = h.get('miners', 0)
            g['on%d' % mi] = o.get('n', 0) + c.get('n', 0)
            g['om%d' % mi] = o.get('mines', 0) + c.get('mines', 0)
            g['og%d' % mi] = o.get('miners', 0) + c.get('miners', 0)
            g['od%d' % mi] = o.get('drop', 0) + c.get('drop', 0)
            g['ot%d' % mi] = o.get('tow', 0) + c.get('tow', 0)
            g['oc%d' % mi] = o.get('cit', 0) + c.get('cit', 0)
            g['os%d' % mi] = o.get('sold', 0) + c.get('sold', 0)
            g['em%d' % mi] = e.get('empty', {}).get('mines', 0)
            g['eg%d' % mi] = e.get('enemy', {}).get('miners', 0)   # its miners on enemy islands
            g['mg%d' % mi] = sum(e.get(k, {}).get('miners', 0) for k in CL)
        first = [mi for mi, e in sorted(isl[p].items()) if e.get('own', {}).get('drop', 0) + e.get('contest', {}).get('drop', 0) > 0]
        g['settled'] = 1.0 if first else 0.0
        g['first'] = float(first[0]) if first else float('nan')
        g['new'] = float(new[p])
        ks = [x for x in work[p]]
        g['w60'] = work[p][max(ks)] if ks else 0
        g['sol'] = army.get(p, 0); g['war'] = navy.get(p, 0)
        g['metal'] = sum(inc.get(p, [0] * 5)[2:]) if side == 'M' else float('nan')
        b = battle.get(p, [0] * 6)
        g['kl'] = b[3] - b[4]; g['kills'] = b[3]; g['losses'] = b[4]
        w = work[p][max(work[p])] if work[p] else 0
        g['alive'] = float(res.get(p) == 1) if res else float(bool(work[p]) and max(work[p]) >= end - 1 and w > 0)
        for k in ('plans', 'sails', 'ashore', 'drop', 'lost', 'given', 'abort'): g['x_' + k] = float(ex[p][k])
        out[side] = g
    return out

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
nn = lambda v: [x for x in v if x == x]
mean = lambda v: sum(nn(v)) / len(nn(v)) if nn(v) else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in nn(v)) / max(len(nn(v)) - 1, 1) / len(nn(v))) ** 0.5 if nn(v) else 0
conds = sorted({c for _, c in data})
TL = ('hm', 'hg', 'on', 'om', 'og', 'od', 'ot', 'oc', 'os', 'em', 'mg')
for c in conds:
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for side in 'MV':
            gs = [g[side] for g in games.values()]
            wins = sum(1 for g in games.values() if g[side]['alive'] and not g['VM'[side == 'V']]['alive'])
            print('  %-10s %s n=%d won %d settled %.2f (first %.1f min) new-off-home %.1f | w60 %.1f sol %.1f war %.1f metal %.0f kills %.1f losses %.1f kl %.1f' % (
                v, side, len(gs), wins, mean([g['settled'] for g in gs]), mean([g['first'] for g in gs]), mean([g['new'] for g in gs]),
                *[mean([g[k] for g in gs]) for k in ('w60', 'sol', 'war', 'metal', 'kills', 'losses', 'kl')]))
            for mi in MINS:
                print('  %-10s %s  @%2d  ' % ('', side, mi) + '  '.join('%s %.1f' % (k, mean([g['%s%d' % (k, mi)] for g in gs])) for k in TL))
            if side == 'M' and any(g['x_plans'] for g in gs):
                print('  %-10s M  expand: ' % '' + '  '.join('%s %.2f' % (k, mean([g['x_' + k] for g in gs])) for k in ('plans', 'sails', 'ashore', 'drop', 'lost', 'given', 'abort')))
            if verbose:
                for key in sorted(games):
                    g = games[key][side]
                    print('     r%dv%d %s settled %d first %s og60 %d od60 %d hg60 %d mg60 %d w60 %d kl %d alive %d' % (
                        key[0], key[1], side, g['settled'], '%.0f' % g['first'] if g['first'] == g['first'] else '-', g['og60'], g['od60'],
                        g['hg60'], g['mg60'], g['w60'], g['kl'], g['alive']))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            same = sum(1 for k in keys if games[k]['M']['kl'] == bg[k]['M']['kl'] and games[k]['M']['w60'] == bg[k]['M']['w60'] and games[k]['V']['kl'] == bg[k]['V']['kl'])
            D = lambda f: [f(games[k]) - f(bg[k]) for k in keys]
            parts = []
            for name, f in (('og30', lambda g: g['M']['og30']), ('og60', lambda g: g['M']['og60']), ('mg30', lambda g: g['M']['mg30']),
                            ('mg60', lambda g: g['M']['mg60']), ('Vmg60', lambda g: g['V']['mg60']), ('w60', lambda g: g['M']['w60']),
                            ('sol', lambda g: g['M']['sol']), ('war', lambda g: g['M']['war']), ('metal', lambda g: g['M']['metal']),
                            ('kills', lambda g: g['M']['kills']), ('losses', lambda g: g['M']['losses']), ('kl', lambda g: g['M']['kl']),
                            ('alive', lambda g: g['M']['alive'] - g['V']['alive'])):
                d = D(f)
                parts.append('%s %+.2f (%.2f)' % (name, mean(d), se(d)))
            wb = sum(1 for k in keys if bg[k]['M']['alive'] and not bg[k]['V']['alive']), sum(1 for k in keys if bg[k]['V']['alive'] and not bg[k]['M']['alive'])
            wv = sum(1 for k in keys if games[k]['M']['alive'] and not games[k]['V']['alive']), sum(1 for k in keys if games[k]['V']['alive'] and not games[k]['M']['alive'])
            print('  d %s - %s (%d paired, %d the same): wins M/V %d/%d -> %d/%d  %s' % (v, base, len(keys), same, wb[0], wb[1], wv[0], wv[1], '  '.join(parts)))
