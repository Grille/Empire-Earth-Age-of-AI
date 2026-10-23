# Score head-to-head runs (scripts/h2h.sh): per game the AIs of variant B ('# variant: players ...') against the
# others (A) - survival, soldiers (last '# army:' line), kills, losses, razed buildings, epoch, workers; then the mean
# of the per-game differences B - A with its standard error (paired: map, seed and length are the same for both).
import csv, re, sys
from logs import open_log, log_paths
from collections import defaultdict

def load(path):
    lines = [l for l in open_log(path) if not l.startswith('#')]
    rows, hdr = [], None
    for r in csv.reader(lines):
        if r and r[0] == 'time_ms':
            hdr = r
        elif hdr and len(r) == len(hdr):
            rows.append(dict(zip(hdr, r)))
    end = max((int(r['time_ms']) for r in rows), default=0)
    battle, army, bset = {}, {}, None
    for l in open_log(path):
        m = re.match(r'# variant: players (\d+)', l)
        if m: bset = {int(c) for c in m.group(1)}
        m = re.match(r'# battle: P(\d+) at \d+ min: (.*)', l)
        if m: battle[int(m.group(1))] = [int(x) for x in m.group(2).split()]
        m = re.match(r'# army: P(\d+) at [0-9.]+ min: (\d+) soldiers', l)
        if m: army[int(m.group(1))] = int(m.group(2))
    per = defaultdict(list)
    for r in rows:
        if r['phase'] == 'tick': per[int(r['player'])].append(r)
    out = {}
    for p, rs in per.items():
        last = rs[-1]
        w = sum(int(last['list%d' % i]) for i in range(7))
        b = battle.get(p, [0] * 6)
        out[p] = dict(alive=float(int(last['time_ms']) >= end - 30000 and w > 0), soldiers=float(army.get(p, 0)),
                      kills=float(b[3]), losses=float(b[4]), razed=float(b[2]), epoch=float(max(int(r['epoch']) for r in rs)),
                      workers=float(w))
    return out, bset

keys = ('alive', 'soldiers', 'kills', 'losses', 'razed', 'epoch', 'workers')
diffs = defaultdict(list); tot = {'A': defaultdict(float), 'B': defaultdict(float)}; cnt = {'A': 0, 'B': 0}
for path in log_paths(sys.argv[1:]):
    players, bset = load(path)
    if not bset or not players: print('  skipped (no variant line):', path); continue
    g = {'A': [s for p, s in players.items() if p not in bset], 'B': [s for p, s in players.items() if p in bset]}
    if not g['A'] or not g['B']: continue
    tag = re.search(r'_h2h[0-9_]+_(.+)\.csv(?:\.gz)?$', path)
    print('  %-28s B=%s  ' % (tag.group(1) if tag else path, ''.join(map(str, sorted(bset)))) + '  '.join(
        '%s %s' % (k, '/'.join('%.0f' % (sum(s[k] for s in g[v]) / len(g[v])) for v in 'AB'))
        for k in ('alive', 'soldiers', 'kills', 'losses')), '(A/B, alive x1)')
    for k in keys:
        a = sum(s[k] for s in g['A']) / len(g['A']); b = sum(s[k] for s in g['B']) / len(g['B'])
        diffs[k].append(b - a)
    for v in 'AB':
        for s in g[v]:
            cnt[v] += 1
            for k in keys: tot[v][k] += s[k]
n = len(diffs['alive'])
if n:
    se = lambda v: (sum((x - sum(v) / len(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5
    print('  %d games, %d A and %d B AIs' % (n, cnt['A'], cnt['B']))
    print('  mean per AI    A: ' + '  '.join('%s %.2f' % (k, tot['A'][k] / cnt['A']) for k in keys))
    print('                 B: ' + '  '.join('%s %.2f' % (k, tot['B'][k] / cnt['B']) for k in keys))
    print('  B - A per game:   ' + '  '.join('%s %+.2f (se %.2f)' % (k, sum(diffs[k]) / n, se(diffs[k])) for k in keys))
