"""The workforce against what the economy spends, and the army (docs/economy.md "Workforce into army").
Per variant and condition (cheat, map: the logs' ab-<name>-c<cheat>-<map> tags, or h2h logs with -h2h) and side (M the
mod's AI, V the base AI) the means over the games at minutes 15 / 30 / 45 / 60 / 75 (the last sample at or before):
citizens (cit), on food / wood / metals (gF gW gM), soldiers (land + siege + ships + air: # mil), the stock's food, wood,
gold + iron, the citizen goal (# cit goal, the game's wanted); over minutes 20+ the production buildings' busy share,
the military orders refused by the budget / the stock (poor) and the budget's short resources (F W S G I); from the
# spend lines (the mod's side) wood income and wood spent, minutes 0-30 / 30-60 / 60-75; # wf lines (citizens.c
workforce_plan): from minute 30 the means of the wood spent a minute, the woodcutters it needs and those there, CAP's
goal, the citizens wanted given, the idle; over the game the wanted calls held back, wood targets moved, THRIFT's
soldiers; the techs' gold + iron by minute 60 (rsch60).
With -base NAME: paired differences (same seed and seat) of the mod's side against that variant, plus kills - losses,
razed, wins.
usage: python scripts/workforce.py [-base NAME] [-v] "work/autotest_*_ab-<name>-*.csv" """
import glob, re, sys
from logs import open_log, log_paths
from collections import defaultdict

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
base = opt('-base', None)
verbose = '-v' in args
if verbose: args.remove('-v')
paths = log_paths(args)
MINS = (15, 30, 45, 60, 75)
ECO = re.compile(r'^# eco: P(\d+) at (\d+) min: cit=(\d+) gF=(\d+) gW=(\d+) gS=(\d+) gG=(\d+) gI=(\d+) .*?soldiers=(\d+) pop=(\d+) cap=(\d+)', re.M)
MIL = re.compile(r'^# mil: P(\d+) at (\d+) min: stock (.*?) \| acc (.*?) \| sold (\d+) siege (\d+) ships (\d+) air (\d+) value -?\d+ \| '
                 r'bld (.*?) busy (\d+)/(\d+) \| .*? \| calls (\d+) asked \d+ trained (\d+) budget (\d+) poor (\d+) room (\d+) game (\d+) subs \d+ \| short (.*?) \|', re.M)
CIT = re.compile(r'^# cit: P(\d+) at (\d+) min: .*? wanted (-?\d+) have (-?\d+) room (-?\d+) cap (-?\d+) .*? goal (-?\d+) feed -?\d+ slots (-?\d+) home (-?\d+)', re.M)
SPEND = re.compile(r'^# spend: P(\d+) at (\d+) min:(.*?); income F(-?\d+) W(-?\d+) S(-?\d+) G(-?\d+) I(-?\d+)', re.M)
WF = re.compile(r'^# wf: P(\d+) at (\d+) min: (.*)$', re.M)

def at(r, mi):
    ks = [k for k in r if k <= mi]
    return r[max(ks)] if ks else None

def load(path):
    txt = open_log(path).read()
    m = re.search(r'^# vanilla: players ([\d ]+) play', txt, re.M)
    van = {int(x) for x in m.group(1).split()} if m else set()
    vp = re.search(r'^# autotest: variant_players=(\d+)', txt, re.M)
    eco, mil, cit, spend, wf = (defaultdict(dict) for _ in range(5))
    for x in ECO.finditer(txt):
        g = [int(v) for v in x.groups()]
        eco[g[0]][g[1]] = dict(cit=g[2], gF=g[3], gW=g[4], gM=g[5] + g[6] + g[7], pop=g[9], cap=g[10])
    for x in MIL.finditer(txt):
        g = x.groups()
        st = [int(v) for v in g[2].split()]
        mil[int(g[0])][int(g[1])] = dict(stock=st, army=sum(int(v) for v in g[4:8]), bld=sum(int(v) for v in g[8].split()),
                                          busy=int(g[9]), have=int(g[10]), calls=int(g[11]), trained=int(g[12]), budget=int(g[13]),
                                          poor=int(g[14]), room=int(g[15]), short=[int(v) for v in g[17].split()])
    for x in CIT.finditer(txt):
        g = [int(v) for v in x.groups()]
        cit[g[0]][g[1]] = dict(wanted=g[2], have=g[3], goal=g[6], slots=g[7])
    for x in SPEND.finditer(txt):
        sp = defaultdict(int)
        for s in re.finditer(r' [0-9a-f]+/([FWSGI])=(\d+)\(', x.group(3)): sp[s.group(1)] += int(s.group(2))
        spend[int(x.group(1))][int(x.group(2))] = dict(spent=sp, inc=[int(v) for v in x.groups()[3:8]])
    for x in WF.finditer(txt):
        wf[int(x.group(1))][int(x.group(2))] = {k: int(v) for k, v in re.findall(r'(\w+) (-?\d+)', x.group(3))}
    battle = {}
    for b in re.finditer(r'^# battle: P(\d+) at \d+ min: (.*)$', txt, re.M): battle[int(b.group(1))] = [int(v) for v in b.group(2).split()]
    over = re.search(r'^# autotest: game over at ([\d.]+)[^:]*: (.*)$', txt, re.M)
    res = {int(p): int(v) for p, v in re.findall(r'P(\d+)=(\d+)', over.group(2))} if over else {}
    players = [p for p in mil if p != 1]
    if not players: return None
    end = max(max(r) for r in mil.values())
    def side(p):
        r = eco[p]; ml = mil[p]; g = {}
        for mi in MINS:
            e = at(r, mi) or dict(cit=0, gF=0, gW=0, gM=0)
            m_ = at(ml, mi) or dict(army=0, stock=[0] * 5)
            c = at(cit[p], mi) or dict(goal=0, wanted=0)
            for k in ('cit', 'gF', 'gW', 'gM'): g['%s%d' % (k, mi)] = e[k]
            g['sold%d' % mi] = m_['army']
            g['stF%d' % mi] = m_['stock'][0]; g['stW%d' % mi] = m_['stock'][1]; g['stGI%d' % mi] = m_['stock'][3] + m_['stock'][4]
            g['goal%d' % mi] = c['goal'] or c['wanted']
        late = [e for k, e in ml.items() if k >= 20]
        g['busy'] = sum(e['busy'] for e in late) / max(sum(e['have'] for e in late), 1)
        for k in ('calls', 'trained', 'budget', 'poor', 'room'): g[k] = sum(e[k] for e in late)
        g['short'] = [sum(e['short'][i] for e in late) for i in range(5)]
        g['bld'] = (at(ml, 75) or dict(bld=0))['bld']
        sp = spend[p]
        def wood(a, b):
            e0, e1 = at(sp, a), at(sp, b)
            if not e1: return (0, 0)
            i0 = e0['inc'][1] if e0 else 0; s0 = e0['spent']['W'] if e0 else 0
            return (e1['inc'][1] - i0, e1['spent']['W'] - s0)
        for a, b in ((0, 30), (30, 60), (60, 75)): g['wi%d' % b], g['ws%d' % b] = wood(a, b)
        wl = wf[p]
        wl30 = [e for k, e in wl.items() if k >= 30]
        g['wf'] = {k: sum(e.get(k, 0) for e in wl30) / max(len(wl30), 1) for k in ('spent', 'need', 'have', 'cap', 'given', 'idle')} if wl30 else None
        if g['wf']:
            for k in ('held', 'moved', 'thrift'): g['wf'][k] = sum(e.get(k, 0) for e in wl.values())
            g['wf']['rsch60'] = (at(wl, 60) or {}).get('rsch', 0)
        bt = battle.get(p, [0] * 6)
        g['kl'] = bt[3] - bt[4]; g['kills'] = bt[3]; g['losses'] = bt[4]; g['razed'] = bt[2]; g['lost'] = bt[1]
        g['alive'] = float(res.get(p) == 1) if res else float(max(ml) >= end - 1)
        return g
    if van:
        mod = [p for p in players if p not in van]; vv = [p for p in players if p in van]
        if not mod or not vv: return None
        return {'M': side(mod[0]), 'V': side(vv[0])}
    return {('P%d' % p): side(p) for p in players}

data = defaultdict(dict)
for path in paths:
    m = re.search(r'_ab-(.+?)-(c\d-\w+?)-r(\d+)v(\d+)\.csv(?:\.gz)?$', path)
    if not m: continue
    g = load(path)
    if g: data[(m.group(1), m.group(2))][(int(m.group(3)), int(m.group(4)))] = g
mean = lambda v: sum(v) / len(v) if v else float('nan')
se = lambda v: (sum((x - mean(v)) ** 2 for x in v) / max(len(v) - 1, 1) / len(v)) ** 0.5 if v else 0
SHOW = ['cit', 'gF', 'gW', 'gM', 'sold', 'stF', 'stW', 'stGI', 'goal']
for c in sorted({c for _, c in data}):
    print('== %s' % c)
    for (v, cc), games in sorted(data.items()):
        if cc != c: continue
        for sd in 'MV':
            gs = [g[sd] for g in games.values()]
            M = lambda k: mean([g[k] for g in gs])
            wins = sum(1 for g in games.values() if g[sd]['alive'] and not g['VM'[sd == 'V']]['alive'])
            print('  %-8s %s n=%d won %d | %s' % (v, sd, len(gs), wins, '  '.join('%s %s' % (k, '/'.join('%.0f' % M('%s%d' % (k, mi)) for mi in MINS)) for k in SHOW)))
            sh = ' '.join('%.0f' % mean([g['short'][i] for g in gs]) for i in range(5))
            print('  %-8s %s   20+: busy %.0f%% bld75 %.1f calls %.0f trained %.0f budget %.0f poor %.0f room %.0f short FWSGI %s | wood in/spent 0-30 %.0f/%.0f 30-60 %.0f/%.0f 60-75 %.0f/%.0f | kl %.1f razed %.1f' % (
                v, sd, 100 * M('busy'), M('bld'), M('calls'), M('trained'), M('budget'), M('poor'), M('room'), sh,
                M('wi30'), M('ws30'), M('wi60'), M('ws60'), M('wi75'), M('ws75'), M('kl'), M('razed')))
            wfs = [g['wf'] for g in gs if g['wf']]
            if wfs: print('  %-8s %s   wf means: %s' % (v, sd, ' '.join('%s %.1f' % (k, mean([w[k] for w in wfs])) for k in wfs[0])))
    if base and (base, c) in data:
        bg = data[(base, c)]
        for (v, cc), games in sorted(data.items()):
            if cc != c or v == base: continue
            keys = [k for k in games if k in bg]
            parts = []
            for name in ('cit45', 'cit75', 'gW45', 'gW75', 'gM45', 'gM75', 'sold30', 'sold45', 'sold60', 'sold75', 'stW60', 'busy', 'bld', 'trained', 'kl', 'kills', 'losses', 'razed', 'lost'):
                d = [games[k]['M'][name] - bg[k]['M'][name] for k in keys]
                parts.append('%s %+.1f (%.1f)' % (name, mean(d), se(d)))
            d = [games[k]['V']['sold60'] - bg[k]['V']['sold60'] for k in keys]
            parts.append('Vsold60 %+.1f (%.1f)' % (mean(d), se(d)))
            same = sum(1 for k in keys if games[k]['M']['sold75'] == bg[k]['M']['sold75'] and games[k]['M']['cit75'] == bg[k]['M']['cit75'])
            wins = lambda gg: (sum(1 for g in gg if g['M']['alive'] and not g['V']['alive']), sum(1 for g in gg if g['V']['alive'] and not g['M']['alive']))
            print('  d %s - %s (%d paired, %d same at 75): %s; wins M/V %d/%d -> %d/%d' % (v, base, len(keys), same, '  '.join(parts),
                  *wins([bg[k] for k in keys]), *wins([games[k] for k in keys])))
