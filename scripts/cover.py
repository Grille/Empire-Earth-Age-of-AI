"""Fall back to cover (dll/ai/cover.c # cvd / # cover lines; docs/economy.md "Fall back to cover"). Logs of
scripts/vanilla.sh tagged ab-<name>-c<cheat>-<map>-r<seed>v<seat> (scripts/isl_ablate.sh), or head-to-head logs with -h2h.
Per variant and condition, for the mod (M) and the base AI (V), means per game: the military units killed and their
value (the base price F+W+S+G+I), land and sea; the value lost in fights it could not win (the estimate when the unit
took its target below 0.5 - or, without a fight in the last 2 minutes, at its death); units that died damaged with
time to heal (below half their hit points for 60 s or more, with a quiet moment then); expensive units (value 100+,
or siege, heroes, cavalry, warships) that died alone (no own fighter within 6 tiles) or under an enemy defence; the
hit points regained at Hospitals, Docks, elsewhere; Hospitals standing; what fall_back did (to the Hospital, healed,
broke off, back).
With -base NAME: paired differences (same seed and seat) against that variant: kills - losses, kills, losses, razed,
lost, soldiers alive at 60 / 75 (land + siege, # mil), warships at the end, sunk, ships lost, the value lost and killed
(military), value killed / lost, the value lost in lost fights, died damaged, and the wins (mod won / base AI won).
usage: python scripts/cover.py [-base NAME] "work/autotest_*_ab-<name>-*.csv" ...
       python scripts/cover.py -h2h "work/autotest_*_h2h<stamp>_<name>_*.csv"   (variant players B - the others A per game)"""
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
args = [a for a in args if a not in ('-h2h',)]
paths = log_paths(args)
CVD = re.compile(r'^# cvd: P(\d+) at ([\d.]+) min: (\w)(\d+) val (\d+) hp (\d+) \| eng (-?\d+) ([\d.]+) \| loc (\d+) \| tower (\d) near (\d+) \| '
                 r'low ([\d.]+) quiet (\d) \| hosp (-?\d+) dock (-?\d+) \| held (\d) \| by P(\d+)$', re.M)
COV = re.compile(r'^# cover: P(\d+) at (\d+) min: mil ([\d.]+) dmg ([\d.]+) quiet ([\d.]+) \| heal hosp (\d+) dock (\d+) other (\d+) \| '
                 r'hospitals (\d+) docks (\d+) defences (\d+) \| fb heal (\d+) healed (\d+) broke (\d+) back (\d+) held (-?\d+) refused (\d+) judged (\d+)(?: gave (\d+) home (\d+) ended (\d+))?$', re.M)
MIL = re.compile(r'^# mil: P(\d+) at (\d+) min: stock .*? \| sold (\d+) siege (\d+) ships (\d+) air (\d+) value ', re.M)

def at(r, mi):
    ks = [k for k in r if k <= mi]
    return r[max(ks)] if ks else None

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    van = {int(x) for x in m.group(1).split()} if m else set()
    vp = re.search(r'^# variant: players (\d+) play B', txt, re.M)
    vps = {int(c) for c in vp.group(1)} if vp else set()
    mil = defaultdict(dict)
    for x in MIL.finditer(txt):
        g = [int(v) for v in x.groups()]
        mil[g[0]][g[1]] = dict(land=g[2] + g[3], ships=g[4])
    dead = defaultdict(list)
    for x in CVD.finditer(txt):
        p = int(x.group(1))
        dead[p].append(dict(t=float(x.group(2)), line=x.group(3), val=int(x.group(5)), eng=int(x.group(7)), loc=int(x.group(9)),
                            tower=int(x.group(10)), near=int(x.group(11)), low=float(x.group(12)), quiet=int(x.group(13)), held=int(x.group(16))))
    cov = defaultdict(lambda: defaultdict(float))
    for x in COV.finditer(txt):
        p = int(x.group(1)); g = [float(v or 0) for v in x.groups()[2:]]
        c = cov[p]
        for k, v in zip(('hosp', 'dock', 'other'), g[3:6]): c[k] += v
        c['hospitals'] = g[6]; c['docks'] = g[7]
        for k, v in zip(('fbheal', 'healed', 'broke', 'back', 'refused', 'gave', 'home', 'ended'), (g[9], g[10], g[11], g[12], g[14], g[16], g[17], g[18])): c[k] += v
        c['dmg'] += g[1]; c['quietm'] += g[2]; c['n'] += 1
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
        for mi in (30, 45, 60, 75): g['sold%d' % mi] = (at(ml, mi) or dict(land=0))['land']
        d = dead[p]
        g['dead'] = len(d)
        g['vlost'] = sum(e['val'] for e in d)
        g['vlost_land'] = sum(e['val'] for e in d if e['line'] != 'w')
        g['vlost_sea'] = sum(e['val'] for e in d if e['line'] == 'w')
        lost = lambda e: (e['eng'] >= 0 and e['eng'] < 50) or (e['eng'] < 0 and e['loc'] < 50)
        g['vlost_bad'] = sum(e['val'] for e in d if lost(e))
        g['vlost_bad_land'] = sum(e['val'] for e in d if lost(e) and e['line'] != 'w')
        g['vlost_bad_sea'] = sum(e['val'] for e in d if lost(e) and e['line'] == 'w')
        g['n_bad'] = sum(1 for e in d if lost(e))
        g['dmgdied'] = sum(1 for e in d if e['low'] >= 60 and e['quiet'])
        g['vdmgdied'] = sum(e['val'] for e in d if e['low'] >= 60 and e['quiet'])
        exp = lambda e: e['val'] >= 100 or e['line'] in 'shcw'
        g['exp_dead'] = sum(1 for e in d if exp(e))
        g['exp_alone'] = sum(1 for e in d if exp(e) and e['near'] <= 0)
        g['exp_tower'] = sum(1 for e in d if exp(e) and e['tower'])
        g['vexp_alone_tower'] = sum(e['val'] for e in d if exp(e) and (e['near'] <= 0 or e['tower']))
        g['held_dead'] = sum(1 for e in d if e['held'])
        c = cov[p]
        for k in ('hosp', 'dock', 'other', 'hospitals', 'docks', 'fbheal', 'healed', 'broke', 'back', 'refused', 'gave', 'home', 'ended'): g[k] = c.get(k, 0.0)
        g['dmg'] = c['dmg'] / c['n'] if c.get('n') else 0; g['quietm'] = c['quietm'] / c['n'] if c.get('n') else 0
        nv = navy.get(p, [0, 0, 0])
        g['war'] = nv[0]; g['shiplost'] = nv[1]; g['sunk'] = nv[2]
        bt = battle.get(p, [0] * 6)
        g['kl'] = bt[3] - bt[4]; g['kills'] = bt[3]; g['losses'] = bt[4]; g['razed'] = bt[2]; g['lost'] = bt[1]
        g['alive'] = float(res.get(p) == 1) if res else float(max(ml) >= end - 1)
        return g
    if h2h:
        a = [p for p in players if p not in vps]; b = [p for p in players if p in vps]
        if not a or not b: return None
        A = [side(p) for p in a]; B = [side(p) for p in b]
        for s in A: s['vkilled'] = sum(x['vlost'] for x in B) / len(A)
        for s in B: s['vkilled'] = sum(x['vlost'] for x in A) / len(B)
        return {'A': A, 'B': B}
    mod = [p for p in players if p not in van]; vv = [p for p in players if p in van]
    if not mod or not vv: return None
    M, V = side(mod[0]), side(vv[0])
    M['vkilled'] = V['vlost']; V['vkilled'] = M['vlost']
    return {'M': M, 'V': V}

mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
KEYS = ('kl', 'kills', 'losses', 'razed', 'lost', 'sold60', 'sold75', 'war', 'sunk', 'shiplost', 'vlost', 'vkilled', 'vlost_land', 'vlost_sea',
        'vlost_bad', 'dmgdied', 'vexp_alone_tower')

def ratio_d(games, bg, keys, sd='M'):
    """value killed / lost: the ratio of the sums, and the paired difference of per-game log ratios' mean as a check"""
    import math
    d = [math.log((games[k][sd]['vkilled'] + 100) / (games[k][sd]['vlost'] + 100)) - math.log((bg[k][sd]['vkilled'] + 100) / (bg[k][sd]['vlost'] + 100)) for k in keys]
    return mean(d), se(d)

if h2h:
    games = [g for g in (load(p) for p in paths) if g]
    parts = []
    for k in KEYS + ('alive', 'broke', 'fbheal', 'gave', 'home', 'ended'):
        d = [mean([s[k] for s in g['B']]) - mean([s[k] for s in g['A']]) for g in games]
        parts.append('%s %+.1f (%.1f)' % (k, mean(d), se(d)))
    import math
    d = [math.log((mean([s['vkilled'] for s in g['B']]) + 100) / (mean([s['vlost'] for s in g['B']]) + 100)) -
         math.log((mean([s['vkilled'] for s in g['A']]) + 100) / (mean([s['vlost'] for s in g['A']]) + 100)) for g in games]
    print('%d games, B - A per game: %s  log(killed/lost) %+.3f (%.3f)' % (len(games), '  '.join(parts), mean(d), se(d)))
    sys.exit()

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g

for c in sorted({c for _, c in data}):
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for sd in 'MV':
            gs = [g[sd] for g in games.values()]
            M = lambda k: mean([g[k] for g in gs])
            wins = sum(1 for g in games.values() if g[sd]['alive'] and not g['VM'[sd == 'V']]['alive'])
            vl, vk = sum(g['vlost'] for g in gs), sum(g['vkilled'] for g in gs)
            print('  %-8s %s n=%d won %d | dead %.0f value %.0f (land %.0f sea %.0f) killed %.0f (k/l %.2f) | in lost fights %.0f units, value %.0f (%.0f%%; land %.0f sea %.0f) | '
                  'died damaged with time to heal %.1f (value %.0f) | expensive dead %.1f: alone %.1f under towers %.1f (value alone/towers %.0f) | '
                  'healed hosp %.0f dock %.0f other %.0f, hospitals %.1f docks %.1f | dmg %.1f quiet %.1f | fb hosp %.1f healed %.1f broke %.1f back %.1f refused %.0f held-died %.1f | gave up %.1f home %.1f ended %.2f' % (
                v, sd, len(gs), wins, M('dead'), M('vlost'), M('vlost_land'), M('vlost_sea'), M('vkilled'), vk / max(vl, 1), M('n_bad'), M('vlost_bad'),
                100 * sum(g['vlost_bad'] for g in gs) / max(vl, 1), M('vlost_bad_land'), M('vlost_bad_sea'), M('dmgdied'), M('vdmgdied'), M('exp_dead'), M('exp_alone'),
                M('exp_tower'), M('vexp_alone_tower'), M('hosp'), M('dock'), M('other'), M('hospitals'), M('docks'), M('dmg'), M('quietm'),
                M('fbheal'), M('healed'), M('broke'), M('back'), M('refused'), M('held_dead'), M('gave'), M('home'), M('ended')))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            parts = []
            for name in KEYS:
                d = [games[k]['M'][name] - bg[k]['M'][name] for k in keys]
                parts.append('%s %+.1f (%.1f)' % (name, mean(d), se(d)))
            lr, lse = ratio_d(games, bg, keys)
            same = sum(1 for k in keys if games[k]['M']['kills'] == bg[k]['M']['kills'] and games[k]['M']['losses'] == bg[k]['M']['losses'] and games[k]['M']['sold75'] == bg[k]['M']['sold75'])
            wins = lambda gg: (sum(1 for g in gg if g['M']['alive'] and not g['V']['alive']), sum(1 for g in gg if g['V']['alive'] and not g['M']['alive']))
            print('  d %s - %s (%d paired, %d same): %s  log(killed/lost) %+.3f (%.3f); wins M/V %d/%d -> %d/%d' % (v, base, len(keys), same, '  '.join(parts), lr, lse,
                  *wins([bg[k] for k in keys]), *wins([games[k] for k in keys])))
