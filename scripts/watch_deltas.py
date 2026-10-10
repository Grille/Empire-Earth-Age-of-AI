# Group the "# watch" lines of an eeaimod log into value deltas per code path.
# usage: python watch_deltas.py <csv> [initial_value]
import re, sys
from collections import defaultdict

path = sys.argv[1]
prev = int(sys.argv[2]) if len(sys.argv) > 2 else None
rx = re.compile(r'^# watch \w+: after eip=(\w+) value=(\d+) thread=\d+ stack:(.*)$')
spend, gain = defaultdict(lambda: [0, 0]), defaultdict(lambda: [0, 0])
for line in open(path, errors='replace'):
    m = rx.match(line)
    if not m:
        continue
    eip, val, stack = m.group(1), int(m.group(2)), m.group(3).split()
    if val > 0x7FFFFFFF:
        val -= 1 << 32
    if prev is not None:
        d = val - prev
        key = ' '.join([eip] + stack[:4])
        bucket = spend if d < 0 else gain
        bucket[key][0] += d
        bucket[key][1] += 1
    prev = val
for title, b in (('SPENT', spend), ('GAINED', gain)):
    print('==', title, sum(v[0] for v in b.values()))
    for k, (tot, n) in sorted(b.items(), key=lambda kv: abs(kv[1][0]), reverse=True)[:15]:
        print('%8d in %4d writes  %s' % (tot, n, k))
