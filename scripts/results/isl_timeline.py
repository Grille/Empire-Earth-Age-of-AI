"""Why the mod loses island games (docs/economy.md "Large island maps"): timelines and the first failure per game from
the logs of scripts/test/isl_ablate.sh (vanilla.sh, tags ab-<name>-c<cheat>-<map>-r<seed>v<seats>; 1v1 or team games - a
side's players summed). Per side at 10/20/30/45/60/75 minutes: land soldiers (# army), warships (# navy war), the
enemy's warships in its waters (# navy foes-home), enemy land soldiers on its home island (# isl home_foe), miners home /
on other islands (# isl), other islands held, metal / wood / food gatherers (# gather), fishing boats, mines explored
(# isl known mines). Events per game (first minute): the other side's first building off its home island (# isl new) and
when this side first knew one (# isl seen), sea lost (3+ enemy warships in its waters and fewer of its own), an enemy
landing (5+ enemy soldiers on its home island), the army gap (the other side 3x its soldiers, 20+), the enemy settled
first (the other side's drop site on another island, none of its own). The failure of a game = the first of those for the
mod; counted over the games the mod did not win.
usage: python scripts/results/isl_timeline.py [-v] "local/work/autotest_*_ab-<name>-*.csv" ..."""
import re, sys, glob
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
verbose = '-v' in args
paths = log_paths([a for a in args if a != '-v'])
MINS = (10, 20, 30, 45, 60, 75)

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    ais = {int(p) for p in re.findall(r'^# cheat: P(\d+) ', txt, re.M)}
    side = {p: ('V' if p in van else 'M') for p in ais}
    ser = defaultdict(lambda: defaultdict(dict))   # player -> key -> minute -> value
    for m in re.finditer(r'^# army: P(\d+) at ([\d.]+) min: (\d+) soldiers', txt, re.M):
        ser[int(m.group(1))]['sol'][int(float(m.group(2)))] = int(m.group(3))
    for m in re.finditer(r'^# navy: P(\d+) at (\d+) min: docks.*?fishing (\d+) .*?\| war (\d+) .*?foes-home (\d+)', txt, re.M):
        p, mi = int(m.group(1)), int(m.group(2))
        ser[p]['boats'][mi] = int(m.group(3)); ser[p]['war'][mi] = int(m.group(4)); ser[p]['foeship'][mi] = int(m.group(5))
    for m in re.finditer(r'^# isl: P(\d+) at (\d+) min:(.*)$', txt, re.M):
        p, mi, s = int(m.group(1)), int(m.group(2)), m.group(3)
        d = {}
        for part in s.split('|'):
            mm = re.match(r'\s*(\w+) n=(\d+) S\d+ G\d+ I\d+ W\d+ F\d+ my F\d+ W\d+ S(\d+) G(\d+) I(\d+) oth \d+ drop (\d+)', part)
            if mm: d[mm.group(1)] = (int(mm.group(2)), int(mm.group(3)) + int(mm.group(4)) + int(mm.group(5)), int(mm.group(6)))
        h = d.get('home', (0, 0, 0)); o = d.get('own', (0, 0, 0)); c = d.get('contest', (0, 0, 0))
        ser[p]['hg'][mi] = h[1]; ser[p]['og'][mi] = o[1] + c[1]
        ser[p]['on'][mi] = (o[0] if o[2] else 0) + (c[0] if c[2] else 0)
        ser[p]['odrop'][mi] = o[2] + c[2]
        mm = re.search(r'home_foe sold (\d+)', s)
        if mm: ser[p]['foe'][mi] = int(mm.group(1))
        mm = re.search(r'known mines (\d+) of (\d+)', s)
        if mm: ser[p]['known'][mi] = 100.0 * int(mm.group(1)) / max(int(mm.group(2)), 1)
    for m in re.finditer(r'^# gather: P(\d+) at (\d+) min.*?F (\d+)/\d+.*?W (\d+)/\d+.*?S (\d+)/\d+.*?G (\d+)/\d+.*?I (\d+)/\d+', txt, re.M):
        p, mi = int(m.group(1)), int(m.group(2))
        ser[p]['food'][mi] = int(m.group(3)); ser[p]['wood'][mi] = int(m.group(4))
        ser[p]['metal'][mi] = int(m.group(5)) + int(m.group(6)) + int(m.group(7))
    newb = defaultdict(list)   # owner -> minutes of buildings off home
    for m in re.finditer(r'^# isl new: P(\d+) at ([\d.]+) min', txt, re.M): newb[int(m.group(1))].append(float(m.group(2)))
    seenb = defaultdict(list)  # viewer -> (minute, since)
    for m in re.finditer(r'^# isl seen: P(\d+) at ([\d.]+) min: P(\d+).*there since ([\d.]+) min', txt, re.M):
        seenb[int(m.group(1))].append((float(m.group(2)), float(m.group(4)), int(m.group(3))))
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    def at(p, k, mi):
        ks = [x for x in ser[p][k] if x <= mi]
        return ser[p][k][max(ks)] if ks else 0
    out = {}
    for sd in 'MV':
        ps = [p for p in ais if side[p] == sd]
        if not ps: return None
        tl = {k: {mi: sum(at(p, k, mi) for p in ps) for mi in MINS} for k in ('sol', 'war', 'foeship', 'foe', 'hg', 'og', 'on', 'odrop', 'metal', 'wood', 'food', 'boats')}
        tl['known'] = {mi: sum(at(p, 'known', mi) for p in ps) / len(ps) for mi in MINS}
        out[sd] = dict(ps=ps, tl=tl, ser=ser)
    for sd, ot in (('M', 'V'), ('V', 'M')):
        e = out[sd]; ps = e['ps']; ops = out[ot]['ps']
        ev = {}
        first_other = min([t for p in ops for t in newb[p]] or [999])
        ev['enemy_out'] = first_other
        sn = [t for p in ps for (t, since, own) in seenb[p] if own in ops]
        ev['knew_out'] = min(sn) if sn else 999
        lags = [t - since for p in ps for (t, since, own) in seenb[p] if own in ops]
        ev['lag'] = sum(lags) / len(lags) if lags else float('nan')
        ev['nseen'] = len(lags); ev['nout'] = sum(len(newb[p]) for p in ops)
        def first(cond):
            mins = sorted({mi for p in ps for mi in ser[p]['sol']} | {mi for p in ps for mi in ser[p]['foe']})
            for mi in mins:
                if cond(mi): return mi
            return 999
        sumat = lambda pp, k, mi: sum(at(p, k, mi) for p in pp)
        ev['sea_lost'] = first(lambda mi: sumat(ps, 'foeship', mi) >= 3 and sumat(ps, 'war', mi) < sumat(ps, 'foeship', mi))
        ev['landing'] = first(lambda mi: sumat(ps, 'foe', mi) >= 5)
        ev['army_gap'] = first(lambda mi: sumat(ops, 'sol', mi) >= 20 and sumat(ops, 'sol', mi) >= 3 * sumat(ps, 'sol', mi))
        ev['settled_first'] = first(lambda mi: sumat(ops, 'odrop', mi) > 0 and sumat(ps, 'odrop', mi) == 0)
        ev['won'] = float(any(res.get(p) == 1 for p in ps)) if res else None
        ev['alive'] = (any(res.get(p) == 1 for p in ps)) if res else True
        e['ev'] = ev
    out['over'] = float(over.group(1)) if over else None
    return out

data = defaultdict(list)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))].append((path, g))
mean = lambda v: sum(v) / len(v) if v else float('nan')
for (v, c), games in sorted(data.items()):
    print('== %s %s: %d games' % (v, c, len(games)))
    for sd in 'MV':
        for k in ('sol', 'war', 'foeship', 'foe', 'hg', 'og', 'on', 'metal', 'wood', 'food', 'boats', 'known'):
            print('  %s %-8s ' % (sd, k) + '  '.join('%2d:%6.1f' % (mi, mean([g[sd]['tl'][k][mi] for _, g in games])) for mi in MINS))
    for sd, ot in (('M', 'V'), ('V', 'M')):
        evs = [g[sd]['ev'] for _, g in games]
        km = [e['knew_out'] for e in evs if e['knew_out'] < 999]
        eo = [e['enemy_out'] for e in evs if e['enemy_out'] < 999]
        print('  %s: the other side out first at %.1f min (%d games); %s knew one at %.1f min (%d games), lag %.1f min, %d of %d seen' % (
            sd, mean(eo), len(eo), sd, mean(km), len(km), mean([e['lag'] for e in evs if e['lag'] == e['lag']]),
            sum(e['nseen'] for e in evs), sum(e['nout'] for e in evs)))
    # the mod's failures: the first event in the games it did not win
    lost = [g for _, g in games if not (g['M']['ev']['won'])]
    cnt = defaultdict(int); when = defaultdict(list); any_ = defaultdict(int)
    for g in lost:
        ev = g['M']['ev']
        ks = [k for k in ('sea_lost', 'landing', 'army_gap', 'settled_first') if ev[k] < 999]
        for k in ks: any_[k] += 1; when[k].append(ev[k])
        if ks:
            f = min(ks, key=lambda k: ev[k]); cnt[f] += 1
        else: cnt['none'] += 1
    print('  mod not winning: %d games; first failure: %s' % (len(lost), ', '.join('%s %d' % kv for kv in sorted(cnt.items(), key=lambda kv: -kv[1]))))
    print('  ever (games, mean minute): ' + ', '.join('%s %d (%.0f)' % (k, any_[k], mean(when[k])) for k in ('sea_lost', 'landing', 'army_gap', 'settled_first')))
    if verbose:
        for path, g in games:
            ev = g['M']['ev']
            print('    %s over %s won %s sea %s land %s gap %s settled %s' % (re.search(r'r\d+v\d+', path).group(0), g['over'], ev['won'], ev['sea_lost'], ev['landing'], ev['army_gap'], ev['settled_first']))
