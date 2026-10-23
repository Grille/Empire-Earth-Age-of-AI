# Worker churn per suite variant: citizens leaving a resource (sum of drops in the per-resource worker counts between
# consecutive post_assign rows, 10 s apart; includes deaths) per AI and 10 minutes, and target volatility (sum of
# |target change| between consecutive post_targets rows, 5 s apart).
# usage: python scripts/churn.py log.csv...
import csv, sys, re, collections
from logs import open_log, log_paths
V = collections.defaultdict(lambda: [0, 0, 0.0])   # leaves, target moves, AI-minutes
for path in log_paths(sys.argv[1:]):
    m = re.search(r'_suite\d+_\d+_([^_]+)_', path); v = m[1] if m else path
    hdr, last, lastt, span = None, {}, {}, {}
    for ln in open_log(path):
        if ln.startswith('#'): continue
        if ln.startswith('time_ms'): hdr = ln.strip().split(','); continue
        if not hdr: continue
        r = dict(zip(hdr, ln.strip().split(',')))
        p, t = r.get('player'), int(r.get('time_ms', 0))
        if r.get('phase') == 'post_assign':
            w = [int(r[f'list{i}']) for i in range(2, 7)]
            if p in last: V[v][0] += sum(max(0, a - b) for a, b in zip(last[p], w))
            last[p] = w; span.setdefault(p, [t, t])[1] = t
        elif r.get('phase') == 'post_targets':
            g = [int(r[f'target{i}']) for i in range(2, 7)]
            if p in lastt: V[v][1] += sum(abs(a - b) for a, b in zip(lastt[p], g))
            lastt[p] = g
    V[v][2] += sum((b - a) / 60000 for a, b in span.values())
for v, (lv, tm, mins) in V.items():
    print(f'{v:10s} citizens leaving a resource {lv / mins * 10:6.1f} per AI and 10 min, target changes {tm / mins * 10:6.1f}')
