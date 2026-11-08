"""Gold and iron income per minute of each side, from the logs' '# spend:' lines (every player's, the game's own additions of resources:
econ.c hook_add_resource) - what a game's economy war is judged by (the player 2026-11-08: "the gold the ai gets and the enemy loses per
minute is worth way more than a few unit loses"). A cheating game AI's own bonus income is counted as income too.
    python scripts/results/income.py vanilla <run name> ...      the mod against the game's AI: the mod's and the vanilla player's per minute
    python scripts/results/income.py h2h <run name> ...          head-to-head: B - A per game (paired), seats from <run name>'s .out in local/work
A run name is the NAME the runner was given (for vanilla its logs are autotest_*_<name>-r*v*, for h2h autotest_*<name>_*-r*)."""
import glob, math, os, re, sys
from logs import open_log

RES = ('F', 'W', 'S', 'G', 'I')

def income(path):
    """{player: (minute, {res: cumulative})} of each player's last '# spend:' line"""
    out = {}
    for l in open_log(path):
        m = re.match(r'# spend: P(\d+) at (\d+) min:.*; income F(-?\d+) W(-?\d+) S(-?\d+) G(-?\d+) I(-?\d+)', l)
        if m: out[int(m.group(1))] = (int(m.group(2)), dict(zip(RES, map(int, m.groups()[2:]))))
    return out

def rate(rec, keys):
    minute, val = rec
    return sum(val[k] for k in keys) / minute if minute else 0.0

def stat(xs):
    n = len(xs)
    if not n: return 'none'
    mean = sum(xs) / n
    se = math.sqrt(sum((x - mean) ** 2 for x in xs) / (n - 1) / n) if n > 1 else 0.0
    return '%+.1f (se %.1f)' % (mean, se)

def main():
    mode, names = sys.argv[1], sys.argv[2:]
    for name in names:
        if mode == 'vanilla':
            rows = []
            for f in sorted(glob.glob('local/work/autotest_*_%s-r*v*.csv.gz' % name)):
                van = int(re.search(r'v(\d)\.csv', f).group(1))
                mod = 5 - van   # the two AIs are P2 and P3
                r = income(f)
                if mod in r and van in r: rows.append((r[mod], r[van]))
            print('%s: %d games, per minute, the mod / its enemy' % (name, len(rows)))
            for label, keys in (('gold', 'G'), ('iron', 'I'), ('gold+iron', 'GI')):
                print('   %-10s mod %s   enemy %s   mod - enemy %s' % (label, stat([rate(a, keys) for a, b in rows]), stat([rate(b, keys) for a, b in rows]), stat([rate(a, keys) - rate(b, keys) for a, b in rows])))
        else:
            seats = {}
            for l in open(os.path.join('local/work', name + '.out'), errors='replace'):
                m = re.match(r'\s*\S+-(r\d+m?)\s+B=(\d+)', l)
                if m: seats[m.group(1)] = set(int(c) for c in m.group(2))
            rows = []
            for f in sorted(glob.glob('local/work/autotest_*%s_*-r*.csv.gz' % name)):
                tag = re.search(r'-(r\d+m?)\.csv', f).group(1)
                r = income(f)
                if tag not in seats or len(r) < 2: continue
                b = [v for p, v in r.items() if p in seats[tag]]
                a = [v for p, v in r.items() if p not in seats[tag]]
                if a and b: rows.append((a, b))
            print('%s: %d games, per minute, B (the variant) - A' % (name, len(rows)))
            for label, keys in (('gold', 'G'), ('iron', 'I'), ('gold+iron', 'GI')):
                d = [sum(rate(x, keys) for x in b) - sum(rate(x, keys) for x in a) for a, b in rows]
                print('   %-10s B - A %s   (B %.1f, A %.1f)' % (label, stat(d), sum(sum(rate(x, keys) for x in b) for a, b in rows) / max(len(rows), 1), sum(sum(rate(x, keys) for x in a) for a, b in rows) / max(len(rows), 1)))

main()
