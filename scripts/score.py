# Score suite runs: per run and AI player - survival, epoch, workers, food, battle statistics; then averages
# per variant. Battle = last '# battle:' line (EEPlayerStatistics series; 3 kills, 4 losses, 5 buildings
# destroyed - meanings inferred from play, not yet confirmed in code).
# File names: autotest_<stamp>_suite<id>_<variant>_<map>.csv
import csv, re, sys
from collections import defaultdict

def load(path):
    lines = [l for l in open(path, errors='replace') if not l.startswith('#')]
    rows, hdr = [], None
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms':
            hdr = r
        elif hdr and len(r) == len(hdr):
            rows.append(dict(zip(hdr, r)))
    end = max((int(r['time_ms']) for r in rows), default=0)
    battle = {}
    for l in open(path, errors='replace'):
        m = re.match(r'# battle: P(\d+) at \d+ min: (.*)', l)
        if m:
            battle[int(m.group(1))] = [int(x) for x in m.group(2).split()]
    per = defaultdict(list)
    for r in rows:
        if r['phase'] == 'tick':
            per[int(r['player'])].append(r)
    out = {}
    for p, rs in per.items():
        last = rs[-1]
        workers = lambda r: sum(int(r['list%d' % i]) for i in range(7))
        out[p] = dict(
            alive=int(last['time_ms']) >= end - 30000 and workers(last) > 0,
            last_min=int(last['time_ms']) / 60000,
            epoch=max(int(r['epoch']) for r in rs),
            workers_end=workers(last),
            workers_max=max(workers(r) for r in rs),
            food_end=int(last['stock2']),
            kills=battle.get(p, [0] * 6)[3], losses=battle.get(p, [0] * 6)[4], razed=battle.get(p, [0] * 6)[5],
        )
    return end, out

runs = defaultdict(dict)
for path in sys.argv[1:]:
    m = re.search(r'_suite[0-9_]+_([^_]+)_(.+)\.csv$', path)
    if not m:
        continue
    variant, mapname = m.group(1), m.group(2)
    runs[variant][mapname] = load(path)

for variant, maps in runs.items():
    print('==== variant', variant)
    tot = defaultdict(float); n = 0
    for mapname, (end, players) in sorted(maps.items()):
        print('  %-14s end %4.1f min' % (mapname, end / 60000))
        for p, s in sorted(players.items()):
            print('    P%d %-5s epoch %d  workers end %3d max %3d  food end %6d  last seen %5.1f  kills %3d losses %3d razed %3d' % (
                p, 'alive' if s['alive'] else 'DEAD', s['epoch'], s['workers_end'], s['workers_max'], s['food_end'], s['last_min'],
                s['kills'], s['losses'], s['razed']))
            tot['alive'] += s['alive']; tot['epoch'] += s['epoch']; tot['workers_end'] += s['workers_end']
            tot['workers_max'] += s['workers_max']; n += 1
            for k in ('kills', 'losses', 'razed'): tot[k] += s[k]
    if n:
        print('  AVERAGE per AI: alive %.0f%%  epoch %.2f  workers end %.1f  max %.1f  kills %.1f losses %.1f razed %.1f' % (
            100 * tot['alive'] / n, tot['epoch'] / n, tot['workers_end'] / n, tot['workers_max'] / n,
            tot['kills'] / n, tot['losses'] / n, tot['razed'] / n))
