"""Gold and iron against the base-game AI (dll/ai/metal.c # metal lines; docs/economy.md "Gold and iron"). Logs of
scripts/test/vanilla.sh tagged ab-<name>-c<cheat>-<map>-r<seed>v<seat> (scripts/test/isl_ablate.sh), or head-to-head logs with -h2h.
Per variant and condition, for the mod (M) and the base AI (V): what the gold + iron bought by minute 60 (or the last
line) - its share by line (cit inf rng cav sie oth wsh boat air ep up7 tech tower wall mbl bld), the income (G+I, S), the
army alive by line, the enemy soldiers it knows by line (the mod), the time the budget saved for a refused unit
(share of 5-s samples, the gold + iron in stock then), the gold + iron in stock and in the navy account (means).
With -base NAME: paired differences (same seed and seat) against that variant - the measures the A/B tables use:
kills - losses, kills, losses, razed (buildings the mod destroyed), lost (its own), soldiers at 30/45/60/75 (land +
siege + ships + air, # mil), land soldiers trained (# mil), workers at 60 / 75 (# eco cit), the gold + iron income by 60
and the stone's, the orders refused short of gold / iron (# mil short, minutes 20+), the warships at 60 and at the end,
the enemy ships it sank and its own lost (the last # navy line), the base AI's
soldiers at 60, and the wins (mod won / base AI won).
usage: python scripts/results/metal.py [-base NAME] [-v] "local/work/autotest_*_ab-<name>-*.csv" ...
       python scripts/results/metal.py -h2h "local/work/autotest_*_h2h<stamp>_<name>_*.csv"   (variant players B - the others A per game)"""
import glob, re, sys
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
h2h = '-h2h' in args
verbose = '-v' in args
args = [a for a in args if a not in ('-v', '-h2h')]
paths = log_paths(args)
B = 'cit inf rng cav sie oth wsh boat air ep up7 tech tower wall mbl bld unk none'.split()
LINES = 'inf rng cav sie oth wsh boat air'.split()
METAL = re.compile(r'^# metal: P(\d+) at (\d+) min: inc (.*?) \| stock (.*?) \|(.*?)\| army (.*?) \| foe (.*?) \| save (\d+)/(\d+) gi ([\d.]+) refused (\d+) \| gi ([\d.]+) navy ([\d.]+) \| plan (.*)$', re.M)
MIL = re.compile(r'^# mil: P(\d+) at (\d+) min: stock .*? \| sold (\d+) siege (\d+) ships (\d+) air (\d+) value .*? calls \d+ asked \d+ trained (\d+) .*? \| short (\d+) (\d+) (\d+) (\d+) (\d+) \|', re.M)
ECO = re.compile(r'^# eco: P(\d+) at (\d+) min: cit=(\d+)', re.M)

def at(r, mi):
    ks = [k for k in r if k <= mi]
    return r[max(ks)] if ks else None

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    van = {int(x) for x in m.group(1).split()} if m else set()
    vp = re.search(r'^# variant: players (\d+) play B', txt, re.M)
    vps = {int(c) for c in vp.group(1)} if vp else set()
    met, mil, eco = defaultdict(dict), defaultdict(dict), defaultdict(dict)
    for x in METAL.finditer(txt):
        p, mi = int(x.group(1)), int(x.group(2))
        inc = [int(v) for v in x.group(3).split()]
        paid = {k: [int(v) for v in s.split('/')] for k, n, s in re.findall(r' (\w+)=(\d+):([\d/-]+)', x.group(5))}
        army = dict((k, int(v)) for k, v in re.findall(r'(\w+) (\d+)', x.group(6)))
        foe = dict((k, int(v)) for k, v in re.findall(r'(\w+) (\d+)', x.group(7)))
        plan = dict((k, int(v)) for k, v in re.findall(r'(\w+) (-?\d+)', x.group(14)))
        met[p][mi] = dict(inc=inc, paid=paid, army=army, foe=foe, save=int(x.group(8)), samples=int(x.group(9)), save_gi=float(x.group(10)),
                          refused=int(x.group(11)), gi=float(x.group(12)), navy=float(x.group(13)), plan=plan)
    for x in MIL.finditer(txt):
        g = [int(v) for v in x.groups()]
        mil[g[0]][g[1]] = dict(sold=g[2] + g[3] + g[4] + g[5], ships=g[4], trained=g[6], short=g[7:12])
    for x in ECO.finditer(txt):
        eco[int(x.group(1))][int(x.group(2))] = int(x.group(3))
    battle = {}
    for b in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(b.group(1))] = [int(v) for v in b.group(2).split()]
    navy = {}
    for x in re.finditer(r'^# navy: P(\d+) at \d+ min: docks.*?\| war (\d+) .*?\| lost (\d+) sunk (\d+) ', txt, re.M):
        navy[int(x.group(1))] = [int(v) for v in x.group(2, 3, 4)]
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    players = [p for p in mil if p != 1]
    if not players: return None
    end = max(max(r) for r in mil.values())
    def side(p):
        g = {}
        ml = mil[p]
        for mi in (30, 45, 60, 75): g['sold%d' % mi] = (at(ml, mi) or dict(sold=0))['sold']
        g['ships60'] = (at(ml, 60) or dict(ships=0))['ships']
        g['trained'] = sum(e['trained'] for e in ml.values())
        late = [e for k, e in ml.items() if k >= 20]
        g['shortGI'] = sum(e['short'][3] + e['short'][4] for e in late)
        for mi in (60, 75): g['cit%d' % mi] = at(eco[p], mi) or 0
        mt = met[p]
        e60 = at(mt, 60)
        g['m60'] = e60
        g['GI60'] = e60['inc'][3] + e60['inc'][4] if e60 else 0
        g['S60'] = e60['inc'][2] if e60 else 0
        last = mt[max(mt)] if mt else None
        g['last'] = last
        allm = list(mt.values())
        g['save'] = sum(e['save'] for e in allm) / max(sum(e['samples'] for e in allm), 1)
        g['save_gi'] = sum(e['save_gi'] * e['save'] for e in allm) / max(sum(e['save'] for e in allm), 1)
        g['gi'] = sum(e['gi'] * e['samples'] for e in allm) / max(sum(e['samples'] for e in allm), 1)
        g['navy'] = sum(e['navy'] * e['samples'] for e in allm) / max(sum(e['samples'] for e in allm), 1)
        g['plan'] = defaultdict(int)
        for e in allm:
            for k, v in e['plan'].items(): g['plan'][k] += v
        nv = navy.get(p, [0, 0, 0])
        g['war'] = nv[0]; g['shiplost'] = nv[1]; g['sunk'] = nv[2]
        bt = battle.get(p, [0] * 6)
        g['kl'] = bt[3] - bt[4]; g['kills'] = bt[3]; g['losses'] = bt[4]; g['razed'] = bt[2]; g['lost'] = bt[1]
        g['alive'] = float(res.get(p) == 1) if res else float(max(ml) >= end - 1)
        return g
    if h2h:
        a = [p for p in players if p not in vps]; b = [p for p in players if p in vps]
        if not a or not b: return None
        return {'A': [side(p) for p in a], 'B': [side(p) for p in b]}
    mod = [p for p in players if p not in van]; vv = [p for p in players if p in van]
    if not mod or not vv: return None
    return {'M': side(mod[0]), 'V': side(vv[0])}

mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
KEYS = ('kl', 'kills', 'losses', 'razed', 'lost', 'sold30', 'sold45', 'sold60', 'sold75', 'trained', 'cit60', 'cit75', 'GI60', 'S60',
        'shortGI', 'ships60', 'war', 'sunk', 'shiplost')

if h2h:
    games = [g for g in (load(p) for p in paths) if g]
    parts = []
    for k in KEYS + ('alive',):
        d = [mean([s[k] for s in g['B']]) - mean([s[k] for s in g['A']]) for g in games]
        parts.append('%s %+.1f (%.1f)' % (k, mean(d), se(d)))
    print('%d games, B - A per game: %s' % (len(games), '  '.join(parts)))
    sys.exit()

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g

def bought(gs):
    """the gold + iron paid by line at 60, mean per game, and the share"""
    tot = defaultdict(float)
    for g in gs:
        e = g['m60']
        if not e: continue
        for k, v in e['paid'].items(): tot[k] += (v[3] + v[4]) / len(gs)
    s = sum(tot.values()) or 1
    return ' '.join('%s %.0f (%.0f%%)' % (k, tot[k], 100 * tot[k] / s) for k in B if tot[k] >= 1), s

for c in sorted({c for _, c in data}):
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for sd in 'MV':
            gs = [g[sd] for g in games.values()]
            M = lambda k: mean([g[k] for g in gs])
            wins = sum(1 for g in games.values() if g[sd]['alive'] and not g['VM'[sd == 'V']]['alive'])
            b, s = bought(gs)
            print('  %-8s %s n=%d won %d | G+I income 60 %.0f S %.0f | paid G+I %.0f: %s' % (v, sd, len(gs), wins, M('GI60'), M('S60'), s, b))
            arm = ' '.join('%s %.1f' % (k, mean([(g['m60'] or {'army': {}})['army'].get(k, 0) for g in gs])) for k in LINES)
            foe = ' '.join('%s %.1f' % (k, mean([(g['m60'] or {'foe': {}})['foe'].get(k, 0) for g in gs])) for k in LINES[:5])
            pl = defaultdict(float)
            for g in gs:
                for k, x in g['plan'].items(): pl[k] += x / len(gs)
            print('  %-8s %s   army60 %s | foe60 %s | saving %.0f%% (G+I then %.0f) | G+I stock %.0f navy acc %.0f | sold60 %.1f trained %.0f short G+I %.0f | plan %s' % (
                v, sd, arm, foe if sd == 'M' else '-', 100 * M('save'), M('save_gi'), M('gi'), M('navy'), M('sold60'), M('trained'), M('shortGI'),
                ' '.join('%s %.0f' % (k, x) for k, x in pl.items())))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            parts = []
            for name in KEYS:
                d = [games[k]['M'][name] - bg[k]['M'][name] for k in keys]
                parts.append('%s %+.1f (%.1f)' % (name, mean(d), se(d)))
            d = [games[k]['V']['sold60'] - bg[k]['V']['sold60'] for k in keys]
            parts.append('Vsold60 %+.1f (%.1f)' % (mean(d), se(d)))
            same = sum(1 for k in keys if games[k]['M']['sold75'] == bg[k]['M']['sold75'] and games[k]['M']['kills'] == bg[k]['M']['kills'])
            wins = lambda gg: (sum(1 for g in gg if g['M']['alive'] and not g['V']['alive']), sum(1 for g in gg if g['V']['alive'] and not g['M']['alive']))
            print('  d %s - %s (%d paired, %d same): %s; wins M/V %d/%d -> %d/%d' % (v, base, len(keys), same, '  '.join(parts),
                  *wins([bg[k] for k in keys]), *wins([games[k] for k in keys])))
            if verbose:
                for k in sorted(keys): print('    r%dv%d kl %+d sold60 %+d' % (k[0], k[1], games[k]['M']['kl'] - bg[k]['M']['kl'], games[k]['M']['sold60'] - bg[k]['M']['sold60']))
