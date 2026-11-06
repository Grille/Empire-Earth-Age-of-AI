"""Riders passing through (tests/scenarios/riders.eeai against riders_none.eeai): how long the AI's gathering is down.

usage: riders.py [-p P] [-from M] [-to M] <logs...>   (scen.sh's logs: the variant from the tag _<name>_<scenario>-r<seed>)

Per variant and scenario: P's gatherers each minute (the # doing lines' "all citizens: gather"), the citizens it sent
back from sites and the sites it refused in the window (the # citizens lines' running counts), its citizens killed
there. Against riders_none (same variant, same seeds): the gatherer-minutes lost from -from to -to (default 25 - 35)
and the minutes until the gatherers are back within 2 of the control's."""
import os, re, sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(__file__))
from logs import log_paths, open_log

args = sys.argv[1:]
pl, lo, hi = 3, 25.0, 35.0
while args and args[0].startswith('-'):
    k, v = args[0], args[1]
    args = args[2:]
    if k == '-p': pl = int(v)
    elif k == '-from': lo = float(v)
    elif k == '-to': hi = float(v)
TAG = re.compile(r'_suite\d+_\d+_(?P<name>[^_]+)_(?P<scen>riders(?:_none)?)-r(?P<seed>\d+)')
DOING = re.compile(r'^# doing: P%d at ([\d.]+) min .*all citizens: gather (\d+)' % pl)
CIT = re.compile(r'^# citizens: P%d at ([\d.]+) min: \d+ sent back .*?; (\d+) in all, (\d+) dangerous sites refused' % pl)
DEATH = re.compile(r'^# death: P%d citizen at ([\d.]+) min' % pl)

runs = defaultdict(dict)   # (name, scen) -> seed -> {minute: gatherers, 'sent': .., 'refused': .., 'killed': ..}
for path in log_paths(args):
    m = TAG.search(path)
    if not m:
        continue
    r = {'g': {}, 'sent': [0, 0], 'refused': [0, 0], 'killed': 0}
    with open_log(path) as f:
        for line in f:
            if not line.startswith('# '):
                continue
            d = DOING.match(line)
            if d:
                r['g'][int(round(float(d.group(1))))] = int(d.group(2))
                continue
            c = CIT.match(line)
            if c:
                t = float(c.group(1))
                i = 0 if t < lo else 1 if t <= hi else None
                if i is not None:
                    r['sent'][i] = int(c.group(2)); r['refused'][i] = int(c.group(3))
                continue
            k = DEATH.match(line)
            if k and lo <= float(k.group(1)) <= hi:
                r['killed'] += 1
    runs[(m['name'], m['scen'])][int(m['seed'])] = r

for name in sorted({n for n, _ in runs}):
    a, b = runs.get((name, 'riders'), {}), runs.get((name, 'riders_none'), {})
    seeds = sorted(set(a) & set(b))
    if not seeds:
        print(f'{name}: no pair of riders / riders_none runs')
        continue
    mins = range(int(lo) - 2, int(hi) + 2)
    print(f'{name}: {len(seeds)} seeds, P{pl} gatherers a minute (riders / none / difference)')
    print('  ' + '  '.join(f'{t:>5}' for t in mins))
    ga = [sum(a[s]['g'].get(t, 0) for s in seeds) / len(seeds) for t in mins]
    gb = [sum(b[s]['g'].get(t, 0) for s in seeds) / len(seeds) for t in mins]
    print('  ' + '  '.join(f'{v:5.1f}' for v in ga))
    print('  ' + '  '.join(f'{v:5.1f}' for v in gb))
    print('  ' + '  '.join(f'{x - y:+5.1f}' for x, y in zip(ga, gb)))
    lost, back = [], []
    for s in seeds:
        d = [b[s]['g'].get(t, 0) - a[s]['g'].get(t, 0) for t in range(int(lo), int(hi) + 1)]
        lost.append(sum(d))
        late = [t for t, x in zip(range(int(lo), int(hi) + 1), d) if x > 2]
        back.append((late[-1] + 1 - lo) if late else 0)
    n = len(seeds)
    mean = lambda v: sum(v) / n
    se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / (n - 1) / n) ** 0.5 if n > 1 else 0.0
    sent = [(a[s]['sent'][1] - a[s]['sent'][0]) - (b[s]['sent'][1] - b[s]['sent'][0]) for s in seeds]
    ref = [(a[s]['refused'][1] - a[s]['refused'][0]) - (b[s]['refused'][1] - b[s]['refused'][0]) for s in seeds]
    kil = [a[s]['killed'] - b[s]['killed'] for s in seeds]
    print(f'  gatherer-minutes lost {lo:g}-{hi:g} min: {mean(lost):.1f} ({se(lost):.1f}); back within 2 after {mean(back):.1f} min '
          f'({se(back):.1f}); sent back +{mean(sent):.1f} ({se(sent):.1f}), sites refused +{mean(ref):.1f} ({se(ref):.1f}), '
          f'citizens killed +{mean(kil):.1f} ({se(kil):.1f})')
