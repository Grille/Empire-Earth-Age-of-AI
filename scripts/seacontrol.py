"""Sea control against the sea opponent (dll/test/testopp.c, scenarios/seaop.eeai; docs/testing.md "Sea opponent"): per run
the last "# sea:" line (cumulative counts) and the averages of the minute lines, the AI's own landings ("# landing:")
and its battle line; per group of runs the mean and the standard error.
Runs are named <variant><map> as in fleet_ab.py (scen.sh variant names, no underscores), e.g. f0Se5, f1Le9: the
variant is the part before the map code ([SL]e<epoch>); other names are grouped whole. With -ctl <variant>, each run is
also compared with the control variant's run of the same map and seed (game_seed: the same seed plays the same game
up to where the variant changes something) - paired differences, mean and standard error.
usage: python scripts/seacontrol.py [-v] [-ctl f0] "work/autotest_*_suite<stamp>_*_seaop-r*.csv" ...
"""
import sys, re, glob, math, collections
args = sys.argv[1:]
VERBOSE = '-v' in args
if VERBOSE: args.remove('-v')
CTL = None
if '-ctl' in args:
    i = args.index('-ctl'); CTL = args[i + 1]; del args[i:i + 2]

def avg(v): return sum(v) / len(v) if v else 0.0
def se(v):
    if len(v) < 2: return 0.0
    a = avg(v); return math.sqrt(sum((x - a) ** 2 for x in v) / (len(v) - 1) / len(v))

# metric: (title, function of the run's dict)
M = [
    ('ai warships afloat (avg)', 'ai_war_avg'), ('ai warships most', 'ai_war_most'),
    ('ai warships sunk by opp', 'ai_war_lost'), ('opp warships sunk by ai', 'opp_lost_by_ai'),
    ('sea exchange (opp sunk - ai lost)', 'exchange'), ('opp warships afloat (avg)', 'opp_war_avg'),
    ('ai fishing boats (avg)', 'ai_boats_avg'), ('ai fishing boats lost', 'ai_boats_lost'), ('ai food from fish', 'ai_fish'),
    ('ai citizens killed by opp', 'ai_cit_killed'), ('  of them by ships', 'ai_cit_by_ships'),
    ('ai transports sunk', 'ai_tr_lost'), ('ai landings sailed', 'ai_sailed'), ('ai landings ashore', 'ai_ashore'),
    ('opp landings sailed', 'sailed'), ('opp landings ashore', 'ashore'), ('opp landings stopped', 'stopped'),
    ('opp soldiers landed', 'landed'), ('opp transports sunk', 'opp_tr_lost'),
    ('ai home waters held %', 'ai_held'), ('opp home waters held %', 'opp_held'),
    ('ai buildings on other islands (end)', 'ai_isl_bld'), ('ai towns on other islands (most)', 'ai_isl_towns'),
    ('ai land units on opp island (avg)', 'ai_on_opp_avg'),
    ('ai kills / losses', 'kl'), ('ai kills - losses', 'net'), ('opp retreats / flees', 'rf'),
    ('ai defeated (share of runs)', 'ai_out'), ('minutes', 'min'),
]

def run_metrics(txt):
    lines = re.findall(r'^# sea: P1 vs P(\d+) at (\d+) min \((\w+)\): (.*)$', txt, re.M)
    if not lines: return None
    ai = int(lines[-1][0])
    vals = []
    for _, mn, _, rest in lines:
        d = {k: float(v) for k, v in re.findall(r'(\w+)=(-?[\d.]+)', rest.split(' | ')[0])}
        d['min'] = int(mn); vals.append(d)
    last = dict(vals[-1])
    r = {k: last.get(k, 0) for k in ('ai_war_most', 'ai_war_lost', 'opp_lost_by_ai', 'ai_boats_lost', 'ai_fish', 'ai_cit_killed',
                                      'ai_cit_by_ships', 'ai_tr_lost', 'sailed', 'ashore', 'stopped', 'landed', 'opp_tr_lost',
                                      'ai_isl_bld', 'min')}
    r['exchange'] = r['opp_lost_by_ai'] - r['ai_war_lost']
    r['ai_war_avg'] = avg([v['ai_war'] for v in vals]); r['opp_war_avg'] = avg([v['opp_war'] for v in vals])
    r['ai_boats_avg'] = avg([v['ai_boats'] for v in vals]); r['ai_on_opp_avg'] = avg([v['ai_on_opp'] for v in vals])
    r['ai_isl_towns'] = max(v['ai_isl_towns'] for v in vals)
    tot = last.get('total_s', 0) or 1
    r['ai_held'] = 100.0 * last.get('ai_held_s', 0) / tot; r['opp_held'] = 100.0 * last.get('opp_held_s', 0) / tot
    r['rf'] = (last.get('retreats', 0), last.get('flees', 0))
    r['ai_sailed'] = len(re.findall(r'^# landing: P%d at [\d.]+ min: sails with' % ai, txt, re.M))
    r['ai_ashore'] = len(re.findall(r'^# landing: P%d at [\d.]+ min: all \d+ ashore' % ai, txt, re.M))
    bt = re.findall(r'^# battle: P%d at \d+ min: (.*)' % ai, txt, re.M)
    bt = [int(x) for x in bt[-1].split()] if bt else [0] * 6
    r['kl'] = (bt[3], bt[4]); r['net'] = bt[3] - bt[4]
    over = re.search(r'# autotest: game over at ([0-9.]+) min:(.*)', txt)
    r['over'] = over.group(0) if over else ''
    r['ai_out'] = 1 if over and re.search(r'P%d=2' % ai, over.group(2)) else 0
    r['crash'] = txt.count('\n# exception')
    return r

runs = {}   # (variant, map, seed) -> metrics
groups = collections.OrderedDict()
for pat in args:
    for f in sorted(glob.glob(pat)):
        mm = re.search(r'_(?:h2h|suite)(?:\d{8}_\d{4}|[A-Za-z0-9]+)_([A-Za-z0-9]+?)([SL]e\d+)?_[^_]+-r(\d+)\.csv$', f)
        if mm: var, mp, seed = mm.group(1), mm.group(2) or '', int(mm.group(3))
        else: var, mp, seed = re.sub(r'^.*autotest_\d+_\d+(_s\d+)?_?', '', f).replace('.csv', ''), '', 0
        r = run_metrics(open(f, errors='replace').read())
        if not r: print('no # sea: lines:', f); continue
        runs[(var, mp, seed)] = r
        groups.setdefault(var + mp, []).append((seed, r))
        if VERBOSE:
            print('%-12s r%-3d %s' % (var + mp, seed, ' '.join('%s=%s' % (k, (('%.1f' % v) if isinstance(v, float) else v)) for k, v in r.items() if k not in ('over',))), r['over'])

def cell(vals):
    if not vals: return '-'
    if isinstance(vals[0], tuple):
        return '/'.join('%.1f' % avg([v[i] for v in vals]) for i in range(len(vals[0])))
    return '%.1f (%.1f)' % (avg(vals), se(vals))

names = list(groups)
w = max([len(t) for t, _ in M]) + 1
print('%-*s' % (w, 'per run, mean (std. error)') + ''.join('%16s' % n for n in names))
print('%-*s' % (w, 'runs') + ''.join('%16d' % len(groups[n]) for n in names))
for title, key in M:
    print('%-*s' % (w, title) + ''.join('%16s' % cell([r[key] for _, r in groups[n]]) for n in names))
print('%-*s' % (w, 'crashes / decided games') + ''.join('%16s' % ('%d / %d' % (sum(r['crash'] for _, r in groups[n]), sum(1 for _, r in groups[n] if r['over']))) for n in names))
if CTL:
    print('\npaired with %s (same map and seed): variant - control, mean (std. error)' % CTL)
    keys = [(t, k) for t, k in M if k not in ('kl', 'rf', 'min')]
    pairs = collections.OrderedDict()
    for (var, mp, seed), r in runs.items():
        c = runs.get((CTL, mp, seed))
        if var == CTL or not c: continue
        pairs.setdefault(var + mp, []).append((r, c))
    pn = list(pairs)
    print('%-*s' % (w, '') + ''.join('%16s' % n for n in pn))
    for title, key in keys:
        print('%-*s' % (w, title) + ''.join('%16s' % cell([a[key] - b[key] for a, b in pairs[n]]) for n in pn))
