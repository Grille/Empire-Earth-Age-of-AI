"""Where the money goes: the mod's AI (M) and the base game's AI (V) side by side (scripts/eco_ablate.sh logs; dll/ai/army.c
# mil lines once a game minute). Per variant and condition (cheat, map) the means over the games at minutes 20 / 30 / 45 / 60:
army (soldiers + siege + ships + aircraft) and its value (prices without the Cheat), the military production buildings
(all; B barracks, A archery range, S stable, F siege factory, D dock + navy yard, P airport, T tank + cyber factory),
the share of the time they trained, towers, wall pieces, techs researched (all / line upgrades), the stock (F+W+S+G+I)
and its mean from minute 20, the military account; over minutes 20-60 the military training calls and the refused ones
by why (budget, poor: the stock can't pay, room, game: the game refused), units trained, wishes (soldiers waiting in the
request lists); kills - losses, razed (enemy buildings), lost (own), wins.
With -base NAME: paired differences (same seed and seat) of the mod's side against that variant.
usage: python scripts/milspend.py [-base NAME] "work/autotest_*_ab-*.csv" """
import glob, re, sys
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
paths = log_paths(args)
MIL = re.compile(r'^# mil: P(\d+) at (\d+) min: stock (.*?) \| acc (.*?) \| sold (\d+) siege (\d+) ships (\d+) air (\d+) value (-?\d+) \| '
                 r'bld (.*?) busy (\d+)/(\d+) \| towers (\d+) walls (\d+) \| tech (\d+) up7 (\d+)(?: cost \d+)? \| wish (-?\d+) pop (-?\d+) cap (-?\d+) room (-?\d+) \| '
                 r'calls (\d+) asked (\d+) trained (\d+) budget (\d+) poor (\d+) room (\d+) game (\d+) subs (\d+)(?: \| short (.*?) \| res (.*?))?(?: \| plan float (.*?) fill (\d+) bld (\d+)(?: mix (\d+))?)?$', re.M)
MINS = (20, 30, 45, 60)

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    if not m: return None
    van = {int(x) for x in m.group(1).split()}
    per = defaultdict(dict)
    for x in MIL.finditer(txt):
        g = x.groups()
        st = [int(v) for v in g[2].split()]; acc = [float(v) for v in g[3].split()]; bld = [int(v) for v in g[9].split()]
        per[int(g[0])][int(g[1])] = dict(stock=st, acc=acc, army=sum(int(v) for v in g[4:8]), sold=int(g[4]), value=int(g[8]),
                                          bld=bld, busy=int(g[10]), have=int(g[11]), towers=int(g[12]), walls=int(g[13]),
                                          tech=int(g[14]), up7=int(g[15]), wish=int(g[16]), calls=int(g[20]), trained=int(g[22]),
                                          budget=int(g[23]), poor=int(g[24]), nroom=int(g[25]), game=int(g[26]), subs=int(g[27]),
                                          short=[int(v) for v in g[28].split()] if g[28] else [0] * 5,
                                          res=[int(v) for v in g[29].split()] if g[29] else [0] * 5,
                                          added=sum(int(v) for v in g[30].split()) if g[30] else 0,
                                          fill=int(g[31]) if g[31] else 0, preq=int(g[32]) if g[32] else 0, mix=int(g[33]) if g[33] else 0)
    upc = defaultdict(dict)
    for x in re.finditer(r'^# mil: P(\d+) at (\d+) min: .*? up7 \d+ cost (\d+)', txt, re.M): upc[int(x.group(1))][int(x.group(2))] = int(x.group(3))
    battle = {}
    for b in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(b.group(1))] = [int(v) for v in b.group(2).split()]
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    mod = [p for p in per if p not in van and p != 1]; vp = [p for p in per if p in van]
    if not mod or not vp: return None
    end = max(max(r) for r in per.values())
    out = {}
    for side, p in (('M', mod[0]), ('V', vp[0])):
        r = per[p]; g = {}
        def at(mi, f):
            ks = [k for k in r if k <= mi]
            return f(r[max(ks)]) if ks else 0
        for mi in MINS:
            g['army%d' % mi] = at(mi, lambda e: e['army'])
            g['value%d' % mi] = at(mi, lambda e: e['value'])
            g['bld%d' % mi] = at(mi, lambda e: sum(e['bld']))
            g['stock%d' % mi] = at(mi, lambda e: sum(e['stock']))
        e60 = r[max(r)]
        b = e60['bld']
        g.update(B=b[0], A=b[1], S=b[2], F=b[3], D=b[4] + b[5], P=b[6], T=b[7] + b[8], towers=e60['towers'], walls=e60['walls'],
                 tech=e60['tech'], up7=e60['up7'], upcost=upc[p][max(upc[p])] if upc[p] else 0)
        g['st60'] = e60['stock']
        late = [e for k, e in r.items() if k >= 20]
        g['stockm'] = sum(sum(e['stock']) for e in late) / max(len(late), 1)
        g['accm'] = sum(sum(e['acc']) for e in late) / max(len(late), 1)
        g['busy'] = sum(e['busy'] for e in late) / max(sum(e['have'] for e in late), 1)
        for k in ('calls', 'trained', 'budget', 'poor', 'nroom', 'game', 'subs'): g[k] = sum(e[k] for e in late)
        g['wish'] = sum(e['wish'] for e in late) / max(len(late), 1)
        for k in ('added', 'fill', 'preq', 'mix'): g[k] = sum(e[k] for e in r.values())
        g['short'] = [sum(e['short'][i] for e in late) for i in range(5)]
        g['resm'] = [sum(e['res'][i] for e in late) / max(len(late), 1) for i in range(5)]
        g['stm'] = [sum(e['stock'][i] for e in late) / max(len(late), 1) for i in range(5)]
        g['accr'] = [sum(e['acc'][i] for e in late) / max(len(late), 1) for i in range(5)]
        bt = battle.get(p, [0] * 6)
        g['kl'] = bt[3] - bt[4]; g['razed'] = bt[2]; g['lost'] = bt[1]; g['kills'] = bt[3]; g['losses'] = bt[4]
        g['alive'] = float(res.get(p) == 1) if res else float(max(r) >= end - 1)
        out[side] = g
    return out

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
SHOW = [('army', '%.1f'), ('value', '%.0f'), ('bld', '%.1f'), ('stock', '%.0f')]
for c in sorted({c for _, c in data}):
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for side in 'MV':
            gs = [g[side] for g in games.values()]
            M = lambda k: mean([g[k] for g in gs])
            wins = sum(1 for g in games.values() if g[side]['alive'] and not g['VM'[side == 'V']]['alive'])
            print('  %-10s %s n=%d won %d  ' % (v, side, len(gs), wins) +
                  '  '.join('%s %s' % (k, '/'.join(f % M('%s%d' % (k, mi)) for mi in MINS)) for k, f in SHOW))
            st = [mean([g['st60'][i] for g in gs]) for i in range(5)]
            print('  %-10s %s   60: B%.1f A%.1f S%.1f F%.1f D%.1f P%.1f T%.1f busy %.0f%% towers %.1f walls %.1f tech %.1f up7 %.1f (cost %.0f) | stock60 F%.0f W%.0f S%.0f G%.0f I%.0f '
                  'mean20+ %.0f acc %.0f | calls %.0f trained %.0f budget %.0f poor %.0f room %.0f game %.0f subs %.0f wish %.1f | kl %.1f razed %.1f lost %.1f' % (
                      v, side, M('B'), M('A'), M('S'), M('F'), M('D'), M('P'), M('T'), 100 * M('busy'), M('towers'), M('walls'), M('tech'), M('up7'), M('upcost'),
                      *st, M('stockm'), M('accm'), M('calls'), M('trained'), M('budget'), M('poor'), M('nroom'), M('game'), M('subs'), M('wish'),
                      M('kl'), M('razed'), M('lost')))
            V5 = lambda k: ' '.join('%.0f' % mean([g[k][i] for g in gs]) for i in range(5))
            print('  %-10s %s   20+ means F W S G I: stock %s | reserve %s | account %s | budget short of %s | plan added %.0f fill %.0f bld %.1f mix %.0f' % (v, side, V5('stm'), V5('resm'), V5('accr'), V5('short'), M('added'), M('fill'), M('preq'), M('mix')))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            parts = []
            for name in ('army30', 'army45', 'army60', 'value60', 'bld60', 'busy', 'tech', 'towers', 'stockm', 'stock60', 'trained', 'kl', 'kills', 'losses', 'razed', 'lost'):
                d = [games[k]['M'][name] - bg[k]['M'][name] for k in keys]
                parts.append('%s %+.1f (%.1f)' % (name, mean(d), se(d)))
            d = [(games[k]['M']['alive'] - games[k]['V']['alive']) - (bg[k]['M']['alive'] - bg[k]['V']['alive']) for k in keys]
            parts.append('alive %+.2f (%.2f)' % (mean(d), se(d)))
            wins = lambda gg: (sum(1 for g in gg if g['M']['alive'] and not g['V']['alive']), sum(1 for g in gg if g['V']['alive'] and not g['M']['alive']))
            print('  d %s - %s (%d paired): %s; wins M/V %d/%d -> %d/%d' % (v, base, len(keys), '  '.join(parts),
                  *wins([bg[k] for k in keys]), *wins([games[k] for k in keys])))
