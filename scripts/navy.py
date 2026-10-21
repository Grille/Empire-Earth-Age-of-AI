"""Navy: ship roles from the game's data, and the navy baseline from the `# navy:` log lines (dll/ai/navy.c, docs/navy.md).

usage:
  python scripts/navy.py types LOG        the ship types (a run with setup=scripts/navy_types.eeai): per naval
                                          type its epoch, cost, hit points, attack, range, speed and the game's hit
                                          factor (0x8D200C [target category][weapon kind]) against ship, building,
                                          land and air categories; then who hits each ship category hardest
  python scripts/navy.py [-v] LOG...      per run and AI: docks, fishing boats, warships, the food from fish, ships
                                          lost / sunk, who holds its waters, landings - and the averages per map group
The factor alone is not the damage: damage = attack x class table (1 almost everywhere) x factor - armour (per
attacker class), at least 1."""
import re, sys, os
from collections import defaultdict

here = os.path.dirname(os.path.abspath(__file__))
NAMES = {}
for l in open(os.path.join(here, '..', 'data', 'db', 'converteroutput.log'), errors='replace'):
    m = re.match(r'Object\(value\):(\d+)\)InGame=\d+ Name:(.*?)\s+TechID', l)
    if m:
        NAMES[int(m.group(1))] = m.group(2).strip()

def name(t):
    return NAMES.get(t, '?%d' % t)

def read_types(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    T = {}
    for m in re.finditer(r'T (\d+) fam (\d+) cat (\d+) kind (\d+) cls (\d+) hp (\d+) att (\d+) rng ([\d.]+) min ([\d.]+) area ([\d.]+) rel (\d+) tech (\d+)(?: cost (\d+) (\d+) (\d+) (\d+) (\d+))?(?: \| hp \d+ att [\d.]+ rng [\d.]+ spd ([\d.]+) arm ([\d. ]+))?', txt):
        g = m.groups()
        t = int(g[0])
        T[t] = dict(t=t, fam=int(g[1]), cat=int(g[2]), kind=int(g[3]), cls=int(g[4]), hp=int(g[5]), att=int(g[6]),
                    rng=float(g[7]), area=float(g[9]), rel=int(g[10]), tech=int(g[11]),
                    cost=[int(x) for x in g[12:17]] if g[12] else None, spd=float(g[17]) if g[17] else 0.0,
                    arm=[float(x) for x in g[18].split()] if g[18] else None)
    tab = {}
    for m in re.finditer(r'#   c(\d+):([ \d.e+-]+)', txt):
        tab[int(m.group(1))] = [float(x) for x in m.group(2).split()]
    return T, tab

def epoch(d):
    return d['tech'] // 1000 if d['tech'] < 100000 else 0

NAVAL = {4: 'ship', 10: 'sub', 18: 'transport', 21: 'carrier', 22: 'fishing'}

def cmd_types(path):
    T, tab = read_types(path)
    f = lambda a, c: tab.get(c, [0] * 64)[a['kind']] if a['att'] > 0 and a['kind'] < len(tab.get(c, [])) else 0.0
    real = lambda d: epoch(d) > 0 and name(d['t'])[:1] in 'sxa' or d['fam'] in NAVAL
    ships = [d for d in T.values() if d['fam'] in NAVAL and epoch(d) > 0 and d['cost'] and sum(d['cost']) > 0]
    # the target categories by what stands in them
    cats = defaultdict(list)
    for d in T.values():
        if epoch(d) > 0 or d['fam'] == 2:
            cats[d['cat']].append(d)
    def label(c):
        fams = defaultdict(int)
        for d in cats[c]:
            fams[d['fam']] += 1
        return ','.join('%d' % k for k in sorted(fams))
    print('categories: ' + '; '.join('c%d (fam %s): %s' % (c, label(c), ', '.join(name(d['t']) for d in cats[c][:4]))
                                      for c in sorted(cats)))
    cols = [('frig', 9), ('bship', 21), ('galley', 47), ('sub', 7), ('carr', 50), ('tower', 37), ('bldg', 3), ('AA bl', 46)]
    print('\n%-4s %-4s %-36s %-5s %-9s %5s %4s %4s %4s %5s  ' % ('ep', 'type', 'name', 'kind', 'cost W/G/I', 'hp', 'att', 'rng', 'spd', 'rel') +
          ' '.join('%6s' % c for c, _ in cols) + '  land(min/max)  air(min/max)')
    land_cats = sorted({d['cat'] for d in T.values() if d['fam'] in (5, 6) and epoch(d) > 0})
    air_cats = sorted({d['cat'] for d in T.values() if d['fam'] in (3, 7, 17) and epoch(d) > 0})
    for d in sorted(ships, key=lambda d: (epoch(d), d['fam'], d['kind'], d['t'])):
        lf = [f(d, c) for c in land_cats]
        af = [f(d, c) for c in air_cats]
        c = d['cost']
        print('%-4d %-4d %-36s %-5d %-9s %5d %4d %4.0f %4.2f %5d  ' % (epoch(d), d['t'], name(d['t'])[:36], d['kind'], '%d/%d/%d' % (c[1], c[3], c[4]),
              d['hp'], d['att'], d['rng'], d['spd'], d['rel']) + ' '.join('%6.2f' % f(d, c) for _, c in cols) +
              '  %4.2f/%4.2f     %4.2f/%4.2f' % (min(lf), max(lf), min(af), max(af)))
    print('\nwho hits each ship category hardest (land/air/building attackers with factor >= 1.5; x = factor, a = attack):')
    for lab, c in cols[:5]:
        hits = [(f(a, c), a) for a in T.values() if a['att'] > 0 and epoch(a) > 0 and a['fam'] not in NAVAL and f(a, c) >= 1.5]
        hits += [(f(a, c), a) for a in T.values() if a['att'] > 0 and a['fam'] == 2 and f(a, c) >= 1.0]
        hits.sort(key=lambda x: (epoch(x[1]), -x[0]))
        print('  %s (c%d): ' % (lab, c) + '; '.join('e%d %s x%.2f a%d' % (epoch(a), name(a['t'])[:24], x, a['att']) for x, a in hits[:60]))
    print('\nship categories no land unit / aircraft can hit (factor 0 everywhere): ' +
          ', '.join(lab for lab, c in cols[:5] if not any(f(a, c) > 0 for a in T.values() if a['fam'] in (3, 5, 7, 17) and epoch(a) > 0)))

def runs(paths, verbose):
    groups = defaultdict(list)
    for p in paths:
        txt = open(p, encoding='utf-8', errors='replace').read()
        m = re.search(r'# random map: "([^"]+)" size \d+, \d+ AIs \(\w+\), start epoch (\d+)', txt)
        grp = '%s e%s' % (m.group(1), m.group(2)) if m else os.path.basename(p)
        per = defaultdict(dict)   # P -> minute -> fields
        for m in re.finditer(r'# navy: P(\d+) at (\d+) min: (.*)', txt):
            p_, mi, s = int(m.group(1)), int(m.group(2)), m.group(3)
            g = lambda k: int(re.search(k + r' (-?\d+)', s).group(1))
            inc = dict(zip('FWSGI', map(int, re.search(r'income F(\d+) W(\d+) S(\d+) G(\d+) I(\d+)', s).groups())))
            wt = re.search(r'\[(.*?)\]', s).group(1)
            per[p_][mi] = dict(docks=g('docks'), fish=g('fishing'), food=g('food'), war=g('war'), subs=g('subs'),
                               tr=g('transports'), home=g('home'), away=g('away'), foes=g('foes-home'), lost=g('lost'),
                               sunk=g('sunk'), blost=g('boats-lost'), inc=inc, wt=wt)
        land = defaultdict(lambda: [0, 0, 0])
        for m in re.finditer(r'# landing: P(\d+) at [\d.]+ min: (all (\d+) ashore|sails with)', txt):
            if m.group(3): land[int(m.group(1))][1] += 1; land[int(m.group(1))][2] += int(m.group(3))
            else: land[int(m.group(1))][0] += 1
        shipfail = defaultdict(int)   # the 5-minute army lines' most failed requests (4 kinds a line): ships among them
        for m in re.finditer(r'# army: P(\d+) failed types:(.*)', txt):
            for t, n in re.findall(r'type (\d+) from \w+ x(\d+)', m.group(2)):
                if name(int(t))[:1] == 's' or name(int(t)).startswith('x07 Golden'):
                    shipfail[int(m.group(1))] += int(n)
        over = re.search(r'# autotest: (game over at [\d.]+: [^\n]*|reached \d+ game minutes)', txt)
        crash = '# exception' in txt
        for p_, mins in sorted(per.items()):
            ms = sorted(mins)
            last = mins[ms[-1]]
            peak_war = max(v['war'] + v['subs'] for v in mins.values())
            mid = mins.get(30) or mins[ms[len(ms) // 2]]
            held = sum(1 for v in mins.values() if v['foes'] > v['home']) / len(ms)
            r = dict(min=ms[-1], docks=last['docks'], fish30=mid['fish'], fishmax=max(v['fish'] for v in mins.values()),
                     food=last['food'], war30=mid['war'] + mid['subs'], war=last['war'] + last['subs'], peak=peak_war,
                     tr=last['tr'], lost=last['lost'], sunk=last['sunk'], blost=last['blost'],
                     gi=(last['inc']['G'] + last['inc']['I']) / max(1, ms[-1]), foodinc=last['inc']['F'],
                     home=sum(v['home'] for v in mins.values()) / len(ms), foes=sum(v['foes'] for v in mins.values()) / len(ms),
                     outheld=held, sailed=land[p_][0], ashore=land[p_][1], soldiers=land[p_][2], wt=last['wt'])
            peak_t = defaultdict(int)   # the most of each warship type at one look
            for v in mins.values():
                for tn in v['wt'].split():
                    t, n = map(int, tn.split('x'))
                    peak_t[t] = max(peak_t[t], n)
            r['types'] = peak_t
            r['shipfail'] = shipfail[p_]
            r['won'] = 1 if over and re.search(r'P%d=1' % p_, over.group(1)) else 0
            groups[grp].append(r)
            if verbose:
                print('%-50s P%d %s' % (os.path.basename(p)[-50:], p_, ' '.join('%s=%s' % (k, ('%.1f' % v) if isinstance(v, float) else v) for k, v in r.items() if k != 'types')) +
                      ' types ' + ' '.join('%s:%d' % (name(t)[:20].replace(' ', '_'), n) for t, n in sorted(r['types'].items())))
        if verbose:
            print('    end: %s%s' % (over.group(1) if over else 'unfinished', ' CRASH' if crash else ''))
    keys = ['docks', 'fish30', 'fishmax', 'food', 'foodinc', 'war30', 'war', 'peak', 'tr', 'lost', 'sunk', 'blost', 'gi', 'home', 'foes', 'outheld', 'sailed', 'ashore', 'soldiers', 'shipfail', 'won']
    print('%-22s %3s ' % ('group', 'n') + ' '.join('%7s' % k for k in keys))
    for g, rs in sorted(groups.items()):
        print('%-22s %3d ' % (g, len(rs)) + ' '.join('%7.1f' % (sum(r[k] for r in rs) / len(rs)) for k in keys))
        tt = defaultdict(list)
        for r in rs:
            for t, n in r['types'].items():
                tt[t].append(n)
        print('    warship types (AIs that had one / their most at once): ' +
              ', '.join('%s %d/%.1f' % (name(t), len(v), sum(v) / len(v)) for t, v in sorted(tt.items(), key=lambda kv: -len(kv[1]))))
    print('docks/war/tr: at the end; fish30/war30: at minute 30; fishmax/peak: the most; food: fish food total; foodinc: all food;\n'
          'lost/sunk: navy (ships, subs, transports, carriers); blost: fishing boats lost; gi: gold+iron income a minute;\n'
          'home/foes: own warships in own waters / enemy warships in own waters (average); outheld: share of looks with more\n'
          'enemy than own warships in own waters; sailed/ashore/soldiers: landings;\n'
          'shipfail: failed ship requests among the most failed (5-minute army lines); won: the game ended with this AI the winner')

if __name__ == '__main__':
    a = sys.argv[1:]
    if a and a[0] == 'types':
        cmd_types(a[1])
    else:
        v = '-v' in a
        runs([x for x in a if x != '-v'], v)
