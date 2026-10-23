# Economy timeline from autotest logs: per AI every N minutes - epoch, stock, outstanding demand, worker targets and
# workers per resource (F food, W wood, S stone, G gold, I iron; idle = not assigned), plus the 5-minute army lines.
# usage: python scripts/results/econ.py [-s minutes] log.csv...
import csv, sys, re
from logs import open_log, log_paths

def main():
    args = sys.argv[1:]
    step = 5
    if args[:1] == ['-s']: step, args = int(args[1]), args[2:]
    for path in log_paths(args):
        print('==', path)
        rows, army = {}, {}
        with open_log(path, newline='') as f:
            lines = f.read().splitlines()
        hdr = None
        for ln in lines:
            if ln.startswith('# army: P') and 'soldiers' in ln:
                m = re.match(r'# army: P(\d+) at (\d+) min: (\d+) soldiers; military requests (\d+), trained (\d+)(?: \(substituted \d+\))?, failed short of food (\d+) wood (\d+) stone (\d+) gold (\d+) iron (\d+)', ln)
                if m: army.setdefault(int(m[1]), {})[int(m[2])] = m.groups()[2:]
                continue
            if ln.startswith('#'): continue
            if ln.startswith('time_ms'): hdr = ln.split(','); continue
            if not hdr: continue
            r = dict(zip(hdr, ln.split(',')))
            if r.get('phase') != 'post_targets': continue
            p, t = int(r['player']), int(r['time_ms']) // 60000
            if t % step == 0 and (p, t) not in rows: rows[(p, t)] = r
        R = 'FWSGI'
        for p in sorted({k[0] for k in rows}):
            print(f'P{p}  min ep | stock F W S G I        | demand F W S G I       | target F W S G I | workers idle F W S G I')
            for (pp, t), r in sorted(rows.items()):
                if pp != p: continue
                g = lambda k: [int(r[f'{k}{i}']) for i in range(2, 7)]
                w = [int(r[f'list{i}']) for i in range(7)]
                print(f'   {t:4d} {r["epoch"]:>2} | {" ".join(f"{v:5d}" for v in g("stock"))} | '
                      f'{" ".join(f"{v:5d}" for v in g("demand"))} | {" ".join(f"{v:2d}" for v in g("target"))} | '
                      f'{w[0]+w[1]:3d}  {" ".join(f"{v:2d}" for v in w[2:])}')
            for t, a in sorted(army.get(p, {}).items()):
                print(f'   army {t:3d} min: {a[0]} soldiers, req {a[1]} trained {a[2]}, short F{a[3]} W{a[4]} S{a[5]} G{a[6]} I{a[7]}')

main()
