# Count every player's units by type in the running game (reads the 3-tile unit grid through peek).
# usage: python army.py [player]
import struct, sys, os, re
from collections import Counter, defaultdict
sys.path.insert(0, os.path.dirname(__file__))
import mem

NAMES = {}
log = os.path.join(os.path.dirname(__file__), '..', 'data', 'db', 'converteroutput.log')
if os.path.exists(log):
    for l in open(log, errors='replace'):
        m = re.match(r'Object\(value\):(\d+)\)InGame=\d+ Name:(.*?)\s+TechID', l)
        if m:
            NAMES[int(m.group(1))] = m.group(2).strip()

grid = mem.u32(mem.u32(0x9183BC) + 0x181724)
_, cells, size, _, nx, ny = struct.unpack('<6I', mem.rd(grid, 24))
raw = mem.rd(cells, nx * ny * 16)
seen, per = set(), defaultdict(Counter)
for c in range(nx * ny):
    b, e = struct.unpack_from('<II', raw, c * 16 + 4)
    if e <= b:
        continue
    for u in struct.unpack('<%dI' % ((e - b) // 4), mem.rd(b, e - b)):
        if u in seen:
            continue
        seen.add(u)
        d = mem.rd(u, 0x30)
        owner, td = struct.unpack_from('<I', d, 0x18)[0], struct.unpack_from('<I', d, 0x2C)[0]
        if not owner or not td:
            continue
        p = mem.i32(owner + 0x45C)
        fam, typ = struct.unpack('<ii', mem.rd(td + 0x4B4, 8))
        per[p][(fam, typ)] += 1
args = [a for a in sys.argv[1:] if a != "-b"]
want = int(args[0]) if args else None
for p in sorted(per):
    if want is not None and p != want:
        continue
    fams = Counter()
    for (fam, typ), n in per[p].items():
        fams[fam] += n
    print('P%d: %s' % (p, ', '.join('family %d: %d' % kv for kv in sorted(fams.items()))))
    for (fam, typ), n in sorted(per[p].items(), key=lambda kv: (kv[0][0], -kv[1])):
        if fam != 2 or "-b" in sys.argv:
            print('    %3d x %-32s (type %d, family %d)' % (n, NAMES.get(typ, '?'), typ, fam))
