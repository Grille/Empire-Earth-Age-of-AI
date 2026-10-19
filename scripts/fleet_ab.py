"""Fleet A/B (docs/navy.md stage 3), paired by game_seed: the same seed plays the same game, so a variant game and the
control game of that seed differ only by the variant. Runs are named <variant><map> (h2h.sh NAME / scen.sh variant
names, no underscores), e.g. ctlSe5, flSe5, fmSe5, bothSe5: the variant is the part before the map code ([SL]e<epoch>).
- h2h variants (h2h.sh: odd seeds P2, even seeds P3 play variant B): per seed, the B seat in the variant game minus the
  same seat in the control game (paired), and the difference in differences: (B seat - other) in the variant game
  minus (that seat - other) in the control game.
- both variants (scen.sh, every AI with it): per seed and AI, the variant game minus the control game.
usage: python scripts/fleet_ab.py [-strip n] -ctl ctl -h2h fl,fm -both both "work/autotest_*_h2h2026..._*.csv" ...
"""
import sys, re, glob, collections, math
args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
STRIP = int(opt('-strip', '0')); CTL = opt('-ctl', 'ctl'); H2H = opt('-h2h', 'fl').split(','); BOTH = [x for x in opt('-both', '').split(',') if x]
runs = {}   # (variant, map, seed) -> {p: metrics}
crashes = collections.Counter(); unfinished = collections.Counter()
def avg(v): return sum(v) / len(v) if v else 0.0
def se(v):
    if len(v) < 2: return 0.0
    a = avg(v); return math.sqrt(sum((x - a) ** 2 for x in v) / (len(v) - 1) / len(v))
def metrics(txt, p):
    m = {}
    bt = re.findall(r'^# battle: P%d at \d+ min: (.*)' % p, txt, re.M)
    bt = [int(x) for x in bt[-1].split()] if bt else [0] * 6
    m['kills'] = bt[3]; m['losses'] = bt[4]; m['razed'] = bt[2]; m['bldlost'] = bt[1]
    over = re.search(r'# autotest: game over at [0-9.]+ min:(.*)', txt)
    m['won'] = 1 if over and re.search(r'P%d=1' % p, over.group(1)) else 0
    ws, foes, home = [], [], []
    last = None
    for mm in re.finditer(r'^# navy: P%d at (\d+) min: (.*)' % p, txt, re.M):
        s = mm.group(2)
        gi = lambda k: int(re.search(k + r' (-?\d+)', s).group(1))
        ws.append(gi('war') + gi('subs')); foes.append(gi('foes-home')); home.append(gi('home'))
        ro = re.search(r'lost-roles F(\d+) L(\d+) A(\d+) S(\d+) AA(\d+) T(\d+) \| sunk-roles F(\d+) L(\d+) A(\d+) S(\d+) AA(\d+) T(\d+)', s)
        r = [int(x) for x in ro.groups()] if ro else [0] * 12
        last = dict(lost=sum(r[0:5]), sunk=sum(r[6:11]), tsunk=r[11], tlost=r[5], blost=gi('boats-lost'), bsunk=gi('boats-sunk'), food=gi('food'))
    if not last: return None
    m['war'] = avg(ws); m['home'] = avg(home); m['foes'] = avg(foes); m['wmax'] = max(ws) if ws else 0
    # the fleet's kills by its ships' targets (stage 3/4 fleet line): citizens, buildings, all
    fk = re.findall(r'^# navy: P%d fleet at [\d.]+ min: .*\| kills W(\d+) B(\d+) T(\d+) C(\d+) Bd(\d+) O(\d+)' % p, txt, re.M)
    fk = [int(x) for x in fk[-1]] if fk else [0] * 6
    m['fcit'] = fk[3]; m['fbld'] = fk[4]; m['fkill'] = sum(fk)
    m['wlost'] = last['lost']; m['wsunk'] = last['sunk']; m['sea'] = last['sunk'] - last['lost']
    m['blost'] = last['blost']; m['bsunk'] = last['bsunk']; m['fish'] = last['food']; m['trsunk'] = last['tsunk']; m['trlost'] = last['tlost']
    ar = re.findall(r'^# army: P%d at \d+ min: (\d+) soldiers' % p, txt, re.M)
    m['sold'] = avg([int(x) for x in ar])
    m['ashore'] = len(re.findall(r'^# landing: P%d at [\d.]+ min: all \d+ ashore' % p, txt, re.M))
    m['net'] = m['kills'] - m['losses']
    m['alive'] = 0 if re.search(r'^# .*P%d (?:is out|defeated)' % p, txt, re.M) else 1
    return m
for pat in args:
    for f in sorted(glob.glob(pat)):
        mm = re.search(r'_(?:h2h|suite)\d+_\d+_([A-Za-z]+?)([SL]e\d+)_[^_]+-r(\d+)\.csv$', f)
        if not mm: continue
        var, mp, seed = mm.group(1), mm.group(2), int(mm.group(3))
        if STRIP: var = var[STRIP:]   # -strip n: a run prefix of n letters dropped (rounds with other seeds pooled)
        txt = open(f, errors='replace').read()
        crashes[var + mp] += txt.count('\n# exception')
        unfinished[var + mp] += not (('# autotest: reached' in txt) or ('game over' in txt))
        runs[(var, mp, seed)] = {p: metrics(txt, p) for p in (2, 3)}
K = ['net', 'kills', 'losses', 'war', 'wmax', 'wsunk', 'wlost', 'sea', 'foes', 'blost', 'bsunk', 'fish', 'trsunk', 'trlost', 'fcit', 'fkill', 'sold', 'ashore', 'razed', 'bldlost', 'won']
maps = sorted({k[1] for k in runs})
print('runs:', len(runs), ' crashes:', dict(c for c in crashes.items() if c[1]), ' unfinished:', dict(c for c in unfinished.items() if c[1]))
def line(label, rows):
    print('%-14s %3d ' % (label, len(rows)) + ' '.join('%7.2f' % avg([r[k] for r in rows]) for k in K))
    print('%-14s %3s ' % ('', 'se') + ' '.join('%7.2f' % se([r[k] for r in rows]) for k in K))
print('%-14s %3s ' % ('', 'n') + ' '.join('%7s' % k for k in K))
for v in H2H:
    allp, alld = [], []
    for mp in maps + ['all']:
        pr, dd = [], []
        for (var, m2, seed), r in runs.items():
            if var != v or (mp != 'all' and m2 != mp): continue
            c = runs.get((CTL, m2, seed))
            b = 2 if seed % 2 else 3; o = 5 - b
            if not c or not r.get(b) or not c.get(b) or not r.get(o) or not c.get(o): continue
            pr.append({k: r[b][k] - c[b][k] for k in K})
            dd.append({k: (r[b][k] - r[o][k]) - (c[b][k] - c[o][k]) for k in K})
        if pr:
            line('%s %s seat' % (v, mp), pr)
            line('%s %s DiD' % (v, mp), dd)
for v in BOTH:
    for mp in maps + ['all']:
        pr = []
        for (var, m2, seed), r in runs.items():
            if var != v or (mp != 'all' and m2 != mp): continue
            c = runs.get((CTL, m2, seed))
            for p in (2, 3):
                if c and r.get(p) and c.get(p): pr.append({k: r[p][k] - c[p][k] for k in K})
        if pr: line('%s %s' % (v, mp), pr)
print('seat: the variant seat, variant game - control game (same seed); DiD: (seat - other) variant - (seat - other) control;')
print('both: per AI, variant game - control game. net = kills - losses; war: warships+subs afloat (2-min looks);')
print('wsunk/wlost: enemy warships sunk / own lost (subs incl.), sea = wsunk - wlost; foes: enemy warships in own waters;')
print('blost/bsunk: fishing boats lost / sunk; fish: food from fish; trsunk: transports sunk; ashore: landings ashore')
print('wmax: most warships afloat; fcit / fkill: citizens / all units and buildings killed by the targets of the led fleet')
