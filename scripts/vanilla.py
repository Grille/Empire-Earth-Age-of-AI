"""Score the mod's AI against the game's own AI (scripts/vanilla.sh, docs/testing.md "Against the base-game AI").

Per game: the vanilla player(s) ('# vanilla: players ...') against the mod's AI(s). Outcome: a decided game
('# autotest: game over at M: P2=1 P3=2', 1 won 2 lost), else whose side still has workers at the end (both: a draw).
Per side: kills, losses (units), razed (enemy buildings destroyed), lost (own buildings), workers, epoch, soldiers,
warships afloat / ships lost / ships sunk / fishing food (last '# navy:' line, water maps), the mod's landings ashore.
usage: python scripts/vanilla.py [-v] logs...      (-v: one line per game)"""
import csv, re, sys
from collections import defaultdict

def load(path):
    txt = open(path, errors='replace').read()
    lines = [l for l in txt.splitlines() if not l.startswith('#')]
    rows, hdr = [], None
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms': hdr = r
        elif hdr and len(r) == len(hdr): rows.append(dict(zip(hdr, r)))
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m or not rows: return None
    van = {int(x) for x in m.group(1).split()}
    end = max(int(r['time_ms']) for r in rows)
    battle, army, navy = {}, {}, {}
    for m in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(m.group(1))] = [int(x) for x in m.group(2).split()]
    for m in re.finditer(r'^# army: P(\d+) at [0-9.]+ min: (\d+) soldiers', txt, re.M): army[int(m.group(1))] = int(m.group(2))
    for m in re.finditer(r'^# navy: P(\d+) at \d+ min: docks.*?food (\d+) .*?\| war (\d+) .*?transports (\d+) .*?\| lost (\d+) sunk (\d+) boats-lost (\d+)', txt, re.M):
        navy[int(m.group(1))] = [int(x) for x in m.group(2, 3, 4, 5, 6, 7)]
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    per = defaultdict(list)
    for r in rows:
        if r['phase'] == 'tick': per[int(r['player'])].append(r)
    out = {}
    for p, rs in per.items():
        last = rs[-1]
        w = sum(int(last['list%d' % i]) for i in range(7))
        alive = int(last['time_ms']) >= end - 30000 and w > 0
        if res: alive = res.get(p) == 1
        b = battle.get(p, [0] * 6); nv = navy.get(p, [0] * 6)
        ashore = len(re.findall(r'^# landing: P%d at [\d.]+ min: all \d+ ashore' % p, txt, re.M))
        out[p] = dict(alive=float(alive), kills=float(b[3]), losses=float(b[4]), razed=float(b[2]), lost=float(b[1]),
                      workers=float(w), epoch=float(max(int(r['epoch']) for r in rs)), soldiers=float(army.get(p, 0)),
                      warships=float(nv[1]), shiplost=float(nv[3]), sunk=float(nv[4]), fishfood=float(nv[0]), landings=float(ashore))
    return out, van, (float(over.group(1)) if over else None)

keys = ('alive', 'kills', 'losses', 'razed', 'lost', 'workers', 'epoch', 'soldiers', 'warships', 'shiplost', 'sunk', 'fishfood', 'landings')
verbose = '-v' in sys.argv
tot = {'M': defaultdict(float), 'V': defaultdict(float)}; cnt = {'M': 0, 'V': 0}
diffs = defaultdict(list); wins = {'M': 0, 'V': 0, 'draw': 0}; decided = []
for path in [a for a in sys.argv[1:] if a != '-v']:
    g = load(path)
    if not g: print('  skipped (no vanilla line or no rows):', path); continue
    players, van, over = g
    side = {'M': [s for p, s in players.items() if p not in van and p != 1], 'V': [s for p, s in players.items() if p in van]}
    if not side['M'] or not side['V']: continue
    a = {v: max(s['alive'] for s in side[v]) for v in 'MV'}
    wins['M' if a['M'] and not a['V'] else 'V' if a['V'] and not a['M'] else 'draw'] += 1
    if over is not None: decided.append(over)
    for k in keys:
        diffs[k].append(sum(s[k] for s in side['M']) / len(side['M']) - sum(s[k] for s in side['V']) / len(side['V']))
    for v in 'MV':
        for s in side[v]:
            cnt[v] += 1
            for k in keys: tot[v][k] += s[k]
    if verbose:
        tag = re.search(r'_van[0-9_]+_(.+)\.csv$', path)
        print('  %-24s V=%s %s  ' % (tag.group(1) if tag else path[-30:], ''.join(map(str, sorted(van))),
              'over %.0f' % over if over is not None else 'end    ') +
              '  '.join('%s %s' % (k, '/'.join('%.0f' % (sum(s[k] for s in side[v]) / len(side[v])) for v in 'MV'))
                        for k in ('alive', 'kills', 'losses', 'razed', 'lost', 'workers', 'epoch', 'warships', 'sunk', 'shiplost')), '(mod/vanilla)')
n = len(diffs['alive'])
if n:
    se = lambda v: (sum((x - sum(v) / len(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5
    print('  %d games: mod won %d, vanilla won %d, both standing %d; %d decided early (mean %.0f min)' % (
        n, wins['M'], wins['V'], wins['draw'], len(decided), sum(decided) / len(decided) if decided else 0))
    for v, name in (('M', 'mod    '), ('V', 'vanilla')):
        print('  %s ' % name + '  '.join('%s %.2f' % (k, tot[v][k] / cnt[v]) for k in keys))
    print('  mod - vanilla per game: ' + '  '.join('%s %+.2f (%.2f)' % (k, sum(diffs[k]) / n, se(diffs[k])) for k in keys))
