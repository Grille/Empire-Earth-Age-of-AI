# Epoch advances per AI: game minute of each new epoch and the delay after the planned attempt (cm_t_epoch).
# usage: python scripts/epochs.py log.csv...
import sys, collections
from logs import open_log, log_paths
delays = []
for path in log_paths(sys.argv[1:]):
    hdr, prev = None, {}
    out = collections.defaultdict(list)
    for ln in open_log(path):
        if ln.startswith('#'): continue
        if ln.startswith('time_ms'): hdr = ln.strip().split(','); continue
        if not hdr: continue
        r = dict(zip(hdr, ln.strip().split(',')))
        if r.get('phase') != 'post_targets': continue
        p, t, e, a = r['player'], int(r['time_ms']), int(r['epoch']), int(r['cm_t_epoch'])
        if p in prev and e > prev[p][0]:
            d = (t - prev[p][1]) / 60000
            out[p].append(f'{e}@{t/60000:.1f}(+{d:.1f})'); delays.append(d)
        prev[p] = (e, a)
    print(path.split('_')[-1], ' '.join(f'P{p}: ' + ' '.join(v) for p, v in sorted(out.items())))
if delays: print(f'mean delay after the planned attempt: {sum(delays)/len(delays):.1f} min over {len(delays)} advances')
