# Summarize an aimod_econ.csv: per player, one line per interval with stock, workers per resource, targets, epoch.
# usage: python summarize.py <csv> [interval_minutes=2]
import csv, sys

path = sys.argv[1]
step = float(sys.argv[2]) * 60000 if len(sys.argv) > 2 else 120000
RES = ['food', 'wood', 'stone', 'gold', 'iron']   # types 2..6 (mine order unverified)

rows = []
with open(path, newline='') as f:
    lines = [l for l in f if not l.startswith('#')]
hdr = None
for r in csv.reader(lines):
    if r and r[0] == 'time_ms':
        hdr = r
        continue
    if hdr and len(r) == len(hdr):
        rows.append(dict(zip(hdr, r)))

players = sorted({int(r['player']) for r in rows})
for p in players:
    pr = [r for r in rows if int(r['player']) == p and r['phase'] in ('post_assign', 'tick', 'post_targets')]
    print(f'== player {p} (difficulty {pr[0]["difficulty"]}, cheat {pr[0]["cheat"]})')
    print(f'{"min":>5} ep | ' + ' '.join(f'{n:>6}' for n in RES) + ' | workers ' +
          ' '.join(f'{n[:2]:>3}' for n in RES) + ' | targets ' + ' '.join(f'{n[:2]:>3}' for n in RES))
    nxt = 0
    for r in pr:
        t = int(r['time_ms'])
        if t < nxt:
            continue
        nxt = t + step
        st = [int(r[f'stock{i}']) for i in range(2, 7)]
        wk = [int(r[f'list{i}']) for i in range(2, 7)]
        tg = [int(r[f'target{i}']) for i in range(2, 7)]
        print(f'{t/60000:5.1f} {r["epoch"]:>2} | ' + ' '.join(f'{v:6d}' for v in st) + ' |         ' +
              ' '.join(f'{v:3d}' for v in wk) + f' ={sum(wk):3d} |         ' + ' '.join(f'{v:3d}' for v in tg))
