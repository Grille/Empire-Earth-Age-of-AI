"""Navy A/B (docs/navy.md stage 1): per suite variant (file name _suite<id>_<variant>_), per AI averages from the
`# navy:`, `# army:`, `# landing:` and `# battle:` lines, with std. errors. -split: the AIs with the navy account
(a `# navy: P<n> budget` line) and those without apart (head-to-head runs, [autotest] v_navy); -mix: the AIs with the
fleet mix (a `# navy: P<n> mix` line, stage 2: the fleet builds, navy_fleet=2 / [autotest] v_navy_fleet) and those without apart. The second table: the
warships by role (F frigates, L battleships, A galleys/catapult ships, S attack subs, AA cruisers; average of the 2-min
looks) and lost / sunk by role at the end (T transports).
usage: python scripts/navy_ab.py [-split|-mix] "work/*_suite..._<variant>_*.csv" ..."""
import sys, re, glob, collections, math
groups = collections.OrderedDict()
split = '-split' in sys.argv
mix = '-mix' in sys.argv
for pat in [a for a in sys.argv[1:] if a not in ('-split', '-mix')]:
    for f in sorted(glob.glob(pat)):
        m = re.search(r'_suite\d+_\d+_([^_]+)_', f)
        groups.setdefault(m.group(1) if m else f, []).append(f)
def avg(v): return sum(v) / len(v) if v else 0.0
def se(v):
    if len(v) < 2: return 0.0
    a = avg(v); return math.sqrt(sum((x - a) ** 2 for x in v) / (len(v) - 1) / len(v))
R = ['rF', 'rL', 'rA', 'rS', 'rAA', 'lF', 'lL', 'lA', 'lS', 'lT', 'sF', 'sL', 'sA', 'sS', 'sT']
K = ['wmax', 'wend', 'wavg', 'home', 'foes', 'lost', 'sunk', 'fishmax', 'blost', 'fishF', 'F60', 'GI', 'sold', 'sailed', 'ashore', 'paid', 'refused', 'docks', 'war60', 'kills', 'losses', 'razed', 'bldlost', 'won']
print('%-7s %3s %2s %3s ' % ('grp', 'n', 'cr', 'end') + ' '.join('%7s' % k for k in K))
for g, files in groups.items():
  for side in (['+nv', '-nv'] if split else ['+mix', '-mix'] if mix else ['']):
    rows = collections.defaultdict(list)
    crash = ends = 0
    for f in files:
        txt = open(f, errors='replace').read()
        crash += txt.count('\n# exception')
        ends += ('# autotest: reached' in txt) or ('game over' in txt)
        battle = {int(a): [int(x) for x in b.split()] for a, b in re.findall(r'^# battle: P(\d+) at \d+ min: (.*)', txt, re.M)}
        over = re.search(r'# autotest: game over at [0-9.]+ min:(.*)', txt)
        for p in (2, 3):
            if split and (('+nv' if re.search(r'^# navy: P%d budget' % p, txt, re.M) else '-nv') != side): continue
            if mix and (('+mix' if re.search(r'^# navy: P%d mix' % p, txt, re.M) else '-mix') != side): continue
            bt = battle.get(p, [0] * 6)
            rows['kills'].append(bt[3]); rows['losses'].append(bt[4]); rows['razed'].append(bt[2]); rows['bldlost'].append(bt[1])
            rows['won'].append(1 if over and re.search(r'P%d=1' % p, over.group(1)) else 0)
            ws, homes, foes = [], [], []
            last = None
            for mm in re.finditer(r'^# navy: P%d at (\d+) min: (.*)' % p, txt, re.M):
                s = mm.group(2)
                gi = lambda k: int(re.search(k + r' (-?\d+)', s).group(1))
                w = gi('war') + gi('subs') + gi('carriers')
                ws.append(w); homes.append(gi('home')); foes.append(gi('foes-home'))
                ro = re.search(r'\] roles F(\d+) L(\d+) A(\d+) S(\d+) AA(\d+) \| lost-roles F(\d+) L(\d+) A(\d+) S(\d+) AA\d+ T(\d+) \| sunk-roles F(\d+) L(\d+) A(\d+) S(\d+) AA\d+ T(\d+)', s)
                if ro: rows.setdefault('ro_' + str(id(f)) + str(p), []).append([int(x) for x in ro.groups()])
                inc = list(map(int, re.search(r'income F(\d+) W(\d+) S(\d+) G(\d+) I(\d+)', s).groups()))
                last = dict(t=int(mm.group(1)), w=w, lost=gi('lost'), sunk=gi('sunk'), blost=gi('boats-lost'), food=gi('food'), inc=inc, docks=gi('docks'))
                rows.setdefault('fm_' + str(id(f)) + str(p), []).append(gi('fishing'))
            if not last: continue
            rl = rows.get('ro_' + str(id(f)) + str(p))
            if rl:
                for i, k in enumerate(R): rows[k].append(avg([x[i] for x in rl]) if i < 5 else rl[-1][i])
            rows['wmax'].append(max(ws)); rows['wend'].append(last['w']); rows['wavg'].append(avg(ws))
            rows['home'].append(avg(homes)); rows['foes'].append(avg(foes))
            rows['lost'].append(last['lost']); rows['sunk'].append(last['sunk']); rows['blost'].append(last['blost'])
            rows['fishmax'].append(max(rows['fm_' + str(id(f)) + str(p)])); rows['fishF'].append(last['food'])
            rows['F60'].append(last['inc'][0]); rows['GI'].append((last['inc'][3] + last['inc'][4]) / max(1, last['t']))
            rows['docks'].append(last['docks'])
            ar = re.findall(r'^# army: P%d at \d+ min: (\d+) soldiers' % p, txt, re.M)
            rows['sold'].append(avg([int(x) for x in ar]))
            rows['sailed'].append(len(re.findall(r'^# landing: P%d at [\d.]+ min: sails with' % p, txt, re.M)))
            rows['ashore'].append(len(re.findall(r'^# landing: P%d at [\d.]+ min: all \d+ ashore' % p, txt, re.M)))
            b = re.findall(r'^# navy: P%d budget at \d+ min: .*?ship orders paid (\d+) refused (\d+)' % p, txt, re.M)
            rows['paid'].append(int(b[-1][0]) if b else 0); rows['refused'].append(int(b[-1][1]) if b else 0)
            rows['war60'].append(last['w'] if last['t'] >= 58 else 0)
    print('%-7s %3d %2d %3d ' % (g + side, len(files), crash, ends) + ' '.join('%7.1f' % avg(rows[k]) for k in K))
    if '-se' in sys.argv[0:1] or True:
        print('%-7s %10s ' % ('', 'se') + ' '.join('%7.1f' % se(rows[k]) for k in K))
    if rows.get('rF'):
        print('%-7s %10s ' % ('', 'roles') + ' '.join('%5s %4.1f' % (k, avg(rows[k])) for k in R))
print('wmax/wend/wavg: warships+subs+carriers most / at the end / average of the 2-min looks; home/foes: own / enemy warships in own')
print('waters (average); lost/sunk: navy; fishmax: most fishing boats; blost: fishing boats lost; fishF: food from fish; F60: all food;')
print('GI: gold+iron income a minute; sold: soldiers (5-min army lines avg); sailed/ashore: landings; paid/refused: ship orders (budget line)')
