# Economy summary of suite logs, per variant: mean income per AI by game minute N (F W S G I, from "# spend" lines),
# soldiers trained, and mean gather rate / drop distance of mines (from "# gather" lines).
# usage: python scripts/econsum.py [-m minute] log.csv...   (variant = the tag between _suiteSTAMP_ and _Map)
import sys, re, collections
args = sys.argv[1:]
M = 55
if args[:1] == ['-m']: M, args = int(args[1]), args[2:]
V = collections.defaultdict(lambda: collections.defaultdict(list))
for path in args:
    m = re.search(r'_suite\d+_\d+_([^_]+)_', path)
    v = m[1] if m else path
    trained = collections.Counter()
    for ln in open(path, errors='replace'):
        if ln.startswith('# spend: P') and f' at {M} min' in ln:
            inc = re.search(r'income F(-?\d+) W(-?\d+) S(-?\d+) G(-?\d+) I(-?\d+)', ln)
            if inc:
                for k, x in zip('FWSGI', inc.groups()): V[v][k].append(int(x))
        elif ln.startswith('# army: P') and 'trained' in ln:
            a = re.match(r'# army: P(\d+) at (\d+) min: .*trained (\d+)', ln)
            if a and int(a[2]) <= M: trained[a[1]] += int(a[3])
        elif ln.startswith('# gather: P'):
            for r, n, d, rate in re.findall(r' ([FWSGI]) \d+/(\d+) dist (\d+) rate ([\d.]+);', ln):
                if int(n): V[v]['dist' + r].append(int(d)); V[v]['rate' + r].append(float(rate))
    for p, t in trained.items(): V[v]['trained'].append(t)
mean = lambda l: sum(l) / len(l) if l else 0
for v, d in V.items():
    print(f'{v:12s} income@{M}: ' + ' '.join(f'{k} {mean(d[k]):6.0f}' for k in 'FWSGI') +
          f' | trained {mean(d["trained"]):5.1f} | mine dist S{mean(d["distS"]):4.1f} G{mean(d["distG"]):4.1f} I{mean(d["distI"]):4.1f}'
          f' rate S{mean(d["rateS"]):4.1f} G{mean(d["rateG"]):4.1f} I{mean(d["rateI"]):4.1f} | food dist {mean(d["distF"]):4.1f} rate {mean(d["rateF"]):4.1f}, wood {mean(d["distW"]):4.1f} {mean(d["rateW"]):4.1f}  (n={len(d["F"])})')
