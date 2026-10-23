"""The army as one (dll/ai/mass.c, docs/economy.md "The army as one"): the '# mass:' lines once a game minute - land
soldiers alone (2 or fewer own within 6 tiles), away from the base, in a fight; attack subgroups out and their
stragglers; the land soldiers killed - alone, away, outnumbered; what army_mass did (held / released reinforcements,
subgroup orders refused, attack waits).
Logs of scripts/test/eco_ablate.sh (the mod M against the base AI V, tags ab-<variant>-c<cheat>-<map>-r<seed>v<seat>):
per variant and condition both sides, with -base NAME the paired differences of the mod's side (kills - losses,
kills, losses, razed, lost, workers, soldiers, alive, wins) against that variant.
Head-to-head logs (scripts/test/h2h.sh, '# variant: players'): the B players against the others, per game B - A.
-states: the '# mass2:' lines instead - per variant and condition the mod's attack subgroup members by state (forming,
marching, fighting, other), their stragglers at home / ahead of / behind the middle, the fronts (out subgroups 20+
tiles apart), the subgroups' orders (moves / attacks) a minute, TETHER's skipped targets, REGROUP's and JOIN's orders.
usage: python scripts/results/mass.py [-base NAME] "local/work/autotest_*_ab-*.csv"      python scripts/results/mass.py -h2h "local/work/*_h2h*_name_*.csv"
       python scripts/results/mass.py -states "local/work/autotest_*_ab-*.csv" """
import glob, os, re, sys
from logs import open_log, log_paths
from collections import defaultdict
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
h2h = '-h2h' in args
states = '-states' in args
args = [a for a in args if a not in ('-h2h', '-states')]
paths = log_paths(args)
MASS = re.compile(r'^# mass: P(\d+) at (\d+) min: sold ([\d.]+) iso ([\d.]+) away ([\d.]+) isoaway ([\d.]+) eng ([\d.]+) engiso ([\d.]+) \| '
                  r'sg ([\d.]+) out ([\d.]+) asg ([\d.]+) strag ([\d.]+) \| died (\d+) iso (\d+) away (\d+) isoaway (\d+) outn (\d+) \| '
                  r'held (\d+) released (\d+) refused (\d+) holding (-?\d+) \| waits (\d+) waited (\d+) goes (\d+)$', re.M)
F = ('sold', 'iso', 'away', 'isoaway', 'eng', 'engiso', 'sg', 'out', 'asg', 'strag', 'die', 'die_iso', 'die_away', 'die_isoaway', 'die_outn',
     'held', 'released', 'refused', 'holding', 'waits', 'waited', 'goes')

def mass_of(txt):
    per = defaultdict(lambda: defaultdict(float))
    for x in MASS.finditer(txt):
        p = int(x.group(1)); g = per[p]
        g['mins'] += 1
        for k, v in zip(F, x.groups()[2:]): g[k] += float(v)
    out = {}
    for p, g in per.items():
        n = max(g['mins'], 1)
        out[p] = dict(sold=g['sold'] / n, isoS=g['iso'] / max(g['sold'], 1e-9), isoawayS=g['isoaway'] / max(g['sold'], 1e-9),
                      awayS=g['away'] / max(g['sold'], 1e-9), engisoS=g['engiso'] / max(g['eng'], 1e-9), out=g['out'] / n,
                      stragS=g['strag'] / max(g['asg'], 1e-9), die=g['die'], die_isoS=g['die_iso'] / max(g['die'], 1e-9),
                      die_awayS=g['die_away'] / max(g['die'], 1e-9), die_isoawayS=g['die_isoaway'] / max(g['die'], 1e-9),
                      die_outnS=g['die_outn'] / max(g['die'], 1e-9), die_iso=g['die_iso'], die_outn=g['die_outn'],
                      held=g['held'], released=g['released'], refused=g['refused'], waits=g['waits'], waited=g['waited'] / 60.0, goes=g['goes'])
    return out

mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
SHOW = ('sold', 'isoS', 'awayS', 'isoawayS', 'engisoS', 'out', 'stragS', 'die', 'die_isoS', 'die_awayS', 'die_isoawayS', 'die_outnS',
        'held', 'released', 'refused', 'waits', 'waited', 'goes')
def line(gs):
    return '  '.join('%s %s' % (k, ('%.2f' if k.endswith('S') else '%.1f') % mean([g[k] for g in gs])) for k in SHOW)

M2 = re.compile(r'^# mass2: P(\d+) at (\d+) min: form (.*?) \| march (.*?) \| fight (.*?) \| other (.*?) \| fronts ([\d.]+) multi ([\d.]+) \| '
                r'orders ([\d/ ]+?)(?: \| tethered (\d+) regrouped (\d+)(?: joined (\d+))?)?'
                r'(?: \| died pack (\d+) ahead (\d+) behind (\d+) home (\d+) none (\d+))?$', re.M)
CLS = ('form', 'march', 'fight', 'other')
if states:
    tot = defaultdict(lambda: defaultdict(float))
    for path in paths:
        m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
        if not m: continue
        van = int(m.group(4))
        for x in M2.finditer(open_log(path).read()):
            t = tot[(m.group(1), m.group(2), 'V' if int(x.group(1)) == van else 'M')]
            t['mins'] += 1
            for c, g in zip(CLS, x.groups()[2:6]):
                for k, a in zip(('m', 'sh', 'sa', 'sb'), g.split()): t[c + k] += float(a)
            t['fronts'] += float(x.group(7)); t['multi'] += float(x.group(8))
            for k, gi in (('tethered', 10), ('regrouped', 11), ('joined', 12), ('dpack', 13), ('dahead', 14), ('dbehind', 15), ('dhome', 16), ('dnone', 17)):
                t[k] += int(x.group(gi) or 0)
            for c, o in zip(CLS, x.group(9).split()):
                mv, at = o.split('/'); t[c + 'mv'] += int(mv); t[c + 'at'] += int(at)
    for (v, c, sd), t in sorted(tot.items()):
        mi = max(t['mins'], 1)
        nd = max(sum(t[k] for k in ('dpack', 'dahead', 'dbehind', 'dhome', 'dnone')), 1e-9)
        print('%s %s %s: %d min, fronts %.2f, 2+ out %.0f%%, a minute: tethered %.0f regrouped %.2f joined %.2f' % (
              v, c, sd, t['mins'], t['fronts'] / mi, 100 * t['multi'] / mi, t['tethered'] / mi, t['regrouped'] / mi, t['joined'] / mi))
        if nd >= 1: print('    killed: in an out subgroup - in its pack %.0f%%, ahead %.0f%%, behind %.0f%%, on the way from home %.0f%%; in none out %.0f%%' % tuple(
            100 * t[k] / nd for k in ('dpack', 'dahead', 'dbehind', 'dhome', 'dnone')))
        for k in CLS:
            mm = max(t[k + 'm'], 1e-9)
            print('    %-6s members %.2f  stragglers home %.1f%% ahead %.1f%% behind %.1f%%  orders a minute: move %.1f attack %.1f' % (
                  k, t[k + 'm'] / mi, 100 * t[k + 'sh'] / mm, 100 * t[k + 'sa'] / mm, 100 * t[k + 'sb'] / mm, t[k + 'mv'] / mi, t[k + 'at'] / mi))
    sys.exit()

if h2h:
    side = {'A': [], 'B': []}; d = defaultdict(list)
    for path in paths:
        txt = open_log(path).read()
        m = re.search(r'^# variant: players (\d+)', txt, re.M)
        ms = mass_of(txt)
        if not m or not ms: continue
        bset = {int(c) for c in m.group(1)}
        grp = {'A': [ms[p] for p in ms if p not in bset and p != 1], 'B': [ms[p] for p in ms if p in bset]}
        if not grp['A'] or not grp['B']: continue
        for s in 'AB': side[s] += grp[s]
        for k in SHOW: d[k].append(mean([g[k] for g in grp['B']]) - mean([g[k] for g in grp['A']]))
    for s in 'AB': print('%s n=%d  %s' % (s, len(side[s]), line(side[s])))
    print('B - A per game: ' + '  '.join('%s %+.2f (%.2f)' % (k, mean(d[k]), se(d[k])) for k in SHOW))
    sys.exit()

_ns = {}   # vanilla.py's load() (the file scores its arguments when imported)
_src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vanilla.py')).read()
exec(_src[:_src.index('keys = (')], _ns)
vload = _ns['load']
data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    txt = open_log(path).read()
    v = vload(path); ms = mass_of(txt)
    if not v or not ms: continue
    players, van, over = v
    mod = [p for p in players if p not in van and p != 1]; vp = [p for p in players if p in van]
    if not mod or not vp or mod[0] not in ms or vp[0] not in ms: continue
    g = {}
    for s, p in (('M', mod[0]), ('V', vp[0])):
        g[s] = dict(ms[p]); g[s].update(players[p]); g[s]['kl'] = players[p]['kills'] - players[p]['losses']
    data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
wins = lambda gg: (sum(1 for g in gg if g['M']['alive'] and not g['V']['alive']), sum(1 for g in gg if g['V']['alive'] and not g['M']['alive']))
for c in sorted({c for _, c in data}):
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        w = wins(games.values())
        for s in 'MV':
            gs = [g[s] for g in games.values()]
            print('  %-8s %s n=%d won %d  kl %.1f razed %.1f workers %.1f soldiers %.1f | %s' % (v, s, len(gs), w[s == 'V'],
                  mean([g['kl'] for g in gs]), mean([g['razed'] for g in gs]), mean([g['workers'] for g in gs]), mean([g['soldiers'] for g in gs]), line(gs)))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            parts = []
            for name in ('kl', 'kills', 'losses', 'razed', 'lost', 'workers', 'soldiers', 'die', 'die_iso', 'die_outn', 'isoawayS'):
                d = [games[k]['M'][name] - bg[k]['M'][name] for k in keys]
                parts.append('%s %+.2f (%.2f)' % (name, mean(d), se(d)))
            d = [(games[k]['M']['alive'] - games[k]['V']['alive']) - (bg[k]['M']['alive'] - bg[k]['V']['alive']) for k in keys]
            parts.append('alive %+.2f (%.2f)' % (mean(d), se(d)))
            same = sum(1 for k in keys if games[k]['M']['kl'] == bg[k]['M']['kl'] and games[k]['V']['kl'] == bg[k]['V']['kl'])
            print('  d %s - %s (%d paired, %d same): %s; wins M/V %d/%d -> %d/%d' % (v, base, len(keys), same, '  '.join(parts),
                  *wins([bg[k] for k in keys]), *wins([games[k] for k in keys])))
