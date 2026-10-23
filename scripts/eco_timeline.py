"""Economy timelines of the mod's AI against the game's own AI (scripts/vanilla.sh logs, docs/testing.md "Against the
base-game AI"): per side (mod / vanilla) the mean over the games at every STEP minutes - workers (the AI's resource
lists, tick rows), workers per list, stock, epoch, and from the mod's own lines: the population and cap (# popcheck),
the budget (# B: reserve, military account, idle, blocked), the soldiers (# army), the game's statistics (# battle,
every 10 min: gathered food/wood/stone/gold/iron, citizens, army, units lost) and, with the # eco line (dll/test/ecolog.c),
citizens by goal, trained and lost, town centres training.
usage: python scripts/eco_timeline.py [-step N] [-games] logs..."""
import csv, re, sys
from logs import open_log, log_paths
from collections import defaultdict

STEP = 5
args = sys.argv[1:]
if '-step' in args: i = args.index('-step'); STEP = int(args[i + 1]); del args[i:i + 2]
per_game = '-games' in args
args = [a for a in args if a != '-games']

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    lines = [l for l in txt.splitlines() if not l.startswith('#')]
    hdr, ser = None, defaultdict(dict)   # ser[player][minute] = {metric: value}
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms': hdr = r; continue
        if not hdr or len(r) != len(hdr): continue
        d = dict(zip(hdr, r))
        if d['phase'] != 'tick': continue
        p, mi = int(d['player']), int(d['time_ms']) // 60000
        s = ser[p].setdefault(mi, {})
        lst = [int(d['list%d' % i]) for i in range(7)]
        s['workers'] = sum(lst)
        for i, n in enumerate('xyFWSGI'):
            if i >= 2: s['L' + n] = lst[i]; s['st' + n] = int(d['stock%d' % i])
        s['L0'] = lst[0]
        s['epoch'] = int(d['epoch'])
    def put(p, mi, k, v): ser[p].setdefault(mi, {})[k] = v
    for m in re.finditer(r'^# popcheck: P(\d+) at ([\d.]+) min: cap (\d+), population (\d+)', txt, re.M):
        p, mi = int(m.group(1)), int(float(m.group(2))); put(p, mi, 'pop', int(m.group(4)))
    for m in re.finditer(r'^# B (\d+) P(\d+) food=(-?\d+) reserve=(-?\d+) mil_account=(-?[\d.]+) income=(-?\d+) mil_spent=(-?\d+) idle=(-?\d+) cit_blocked=(-?\d+)', txt, re.M):
        p, mi = int(m.group(2)), int(m.group(1)) // 60000
        put(p, mi, 'reserve', int(m.group(4))); put(p, mi, 'milacct', float(m.group(5))); put(p, mi, 'idleB', int(m.group(8)))
        put(p, mi, 'blocked', int(m.group(9)))
    for m in re.finditer(r'^# army: P(\d+) at (\d+) min: (\d+) soldiers', txt, re.M):
        put(int(m.group(1)), int(m.group(2)), 'soldiers', int(m.group(3)))
    for m in re.finditer(r'^# battle: P(\d+) at (\d+) min: (.*)$', txt, re.M):
        v = [int(x) for x in m.group(3).split()]
        p, mi = int(m.group(1)), int(m.group(2))
        for i, n in zip(range(7, 12), 'FWSGI'): put(p, mi, 'got' + n, v[i])
        put(p, mi, 'killed', v[3]); put(p, mi, 'unitslost', v[4]); put(p, mi, 'stat_cit', v[14]); put(p, mi, 'stat_army', v[13])
    for m in re.finditer(r'^# eco: P(\d+) at (\d+) min: (.*)$', txt, re.M):
        p, mi = int(m.group(1)), int(m.group(2))
        for k, v in re.findall(r'(\w+)=(-?\d+)', m.group(3)): put(p, mi, k, int(v))
    deaths = []
    for m in re.finditer(r'^# death: P(\d+) citizen at ([\d.]+) min at \d+,\d+, (-?\d+) tiles from a drop site, (\w+) \(site type (-?\d+)\), '
                         r'by P(\d+) \(nearest: type (-?\d+) class (-?\d+) at (-?\d+) tiles\); own soldiers within 10: (\d+), the killer.s within 8: (\d+)', txt, re.M):
        deaths.append(dict(p=int(m.group(1)), t=float(m.group(2)), dist=int(m.group(3)), goal=m.group(4), site=int(m.group(5)),
                           k=int(m.group(6)), ktype=int(m.group(7)), kcls=int(m.group(8)), kd=int(m.group(9)), own=int(m.group(10)), foe=int(m.group(11))))
    surr = re.search(r'^# surrender: P(\d+)', txt, re.M)
    return ser, van, deaths, (int(surr.group(1)) if surr else None)

def death_summary(rows, label, games):
    if not rows: print('  %s: no citizen deaths' % label); return
    n = len(rows)
    def dist(fn, buckets):
        c = defaultdict(int)
        for r in rows: c[fn(r)] += 1
        return '  '.join('%s %.1f' % (b, c[b] / games) for b in buckets if c[b])
    print('  %s: %.1f citizens killed a game' % (label, n / games))
    print('    by minute    ', dist(lambda r: '%02d-%02d' % (int(r['t']) // 10 * 10, int(r['t']) // 10 * 10 + 10), ['%02d-%02d' % (a, a + 10) for a in range(0, 70, 10)]))
    print('    from a drop  ', dist(lambda r: 'town<=6' if 0 <= r['dist'] <= 6 else 'near7-15' if 0 <= r['dist'] <= 15 else 'far>15', ['town<=6', 'near7-15', 'far>15']))
    print('    doing        ', dist(lambda r: r['goal'], ['gather', 'build', 'walk', 'attack', 'load', 'other', 'none']))
    print('    own soldiers ', dist(lambda r: 'none' if r['own'] == 0 else '1-3' if r['own'] <= 3 else '4+', ['none', '1-3', '4+']))
    print('    killers near ', dist(lambda r: '1' if r['foe'] <= 1 else '2-4' if r['foe'] <= 4 else '5+', ['1', '2-4', '5+']))
    print('    killer class ', dist(lambda r: 'cls%d' % r['kcls'], sorted({'cls%d' % r['kcls'] for r in rows})))
    tt = defaultdict(int)
    for r in rows: tt[r['ktype']] += 1
    print('    killer types ', '  '.join('%s %.1f' % (NAMES.get(t, 't%d' % t), c / games) for t, c in sorted(tt.items(), key=lambda x: -x[1])[:10]))

import os
NAMES = {}   # type id -> name, from data/db/converteroutput.log (extracted game data, see README Setup) when there
try:
    for l in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'db', 'converteroutput.log'), errors='replace'):
        m = re.match(r'Object\(value\):(\d+)\)InGame=\d Name:(.*?)\s+TechID', l)
        if m: NAMES.setdefault(int(m.group(1)), m.group(2).strip())
except OSError: pass

keys_order = ['workers', 'L0', 'LF', 'LW', 'LS', 'LG', 'LI', 'stF', 'stW', 'stS', 'stG', 'stI', 'epoch', 'pop', 'soldiers',
              'reserve', 'milacct', 'idleB', 'blocked', 'gotF', 'gotW', 'gotS', 'gotG', 'gotI', 'stat_cit', 'unitslost']
acc = {'M': defaultdict(lambda: defaultdict(list)), 'V': defaultdict(lambda: defaultdict(list))}
ngames = 0
dead = {'M': [], 'V': []}
dead_lost = []   # the mod's deaths in the games it resigned
nsurr = 0
for path in log_paths(args):
    g = load(path)
    if not g: continue
    ser, van, deaths, surr = g; ngames += 1
    for r in deaths:
        if r['p'] != 1: dead['V' if r['p'] in van else 'M'].append(r)
    if surr is not None and surr not in van:
        nsurr += 1; dead_lost += [r for r in deaths if r['p'] == surr]
    for p, mins in ser.items():
        if p == 1: continue
        side = 'V' if p in van else 'M'
        last = {}
        for mi in sorted(mins):
            last.update(mins[mi])
            if mi % STEP == 0:
                for k, v in last.items(): acc[side][mi][k].append(v)
        if per_game:
            fin = last
            print('%s P%d %s workers %s pop %s soldiers %s epoch %s' % (path[-40:], p, side, fin.get('workers'), fin.get('pop'), fin.get('soldiers'), fin.get('epoch')))
extra = sorted({k for s in acc.values() for mi in s.values() for k in mi} - set(keys_order))
keys = [k for k in keys_order if any(k in mi for s in acc.values() for mi in s.values())] + extra
print('%d games; mean per side at each minute (M mod, V vanilla); n = samples' % ngames)
mins = sorted(set(acc['M']) | set(acc['V']))
print('%-10s' % 'key' + ''.join('%11d' % m for m in mins))
for k in keys:
    for side in 'MV':
        row = []
        for m in mins:
            v = acc[side][m].get(k, [])
            row.append('%11.1f' % (sum(v) / len(v)) if v else '%11s' % '-')
        if any(c.strip() != '-' for c in row): print('%-10s' % (k + ' ' + side) + ''.join(row))
if dead['M'] or dead['V']:
    print('citizens killed (# death lines; nearest = the killer side\'s nearest soldier/tower/ship within 12 tiles):')
    death_summary(dead['M'], 'mod', ngames)
    death_summary(dead['V'], 'vanilla', ngames)
    if nsurr: death_summary(dead_lost, 'mod, in the %d games it resigned' % nsurr, nsurr)
