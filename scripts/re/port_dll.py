# The engine DLLs' offsets in another build: Art of Conquest ships its own Low-Level Engine, DX7 display and Miles
# mixer DLLs (other builds of the same code). Their functions are paired like the exes' (port_addrs.py load), by their
# exported names first; the fields of their objects are read off the paired code like port_fields.py does - a class's
# methods are the functions Ghidra names after it (Class::Method, from the exports) and its vtables'.
# Needs Ghidra's function lists of both builds: local/games/ee/decomp/dll/<name>/functions.tsv and local/games/aoc/decomp/dll/<name>/ (README
# "Setup": ExportDecomp.java over each DLL).
# usage: python port_dll.py <dll> fields Class[,Class...] [offset ...]   a class's fields (offsets hex)
#        python port_dll.py <dll> global [offset ...]                   every [reg + X] -> [reg + Y], whatever the object
#        python port_dll.py <dll> rva <rva> ...                         a code or data address, relative to the image
#        python port_dll.py <dll> export <name> ...                     an export's address in both builds
# <dll>: the file name without .dll ("Low-Level Engine", DX7HRDisplay, DX7HRTnLDisplay, "Miles Sound System Mixer")
import collections, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs as P
import port_fields as F

dll, what, args = sys.argv[1], sys.argv[2], sys.argv[3:]
short = dll.replace(' ', '').replace('-', '')
P.load(os.path.join(P.GAME, dll + '.dll'), os.path.join(P.ROOT, 'local', 'games', 'ee', 'decomp', 'dll', short, 'functions.tsv'),
       os.path.join(os.path.dirname(P.GAME), 'Empire Earth - The Art of Conquest', dll + '.dll'),
       os.path.join(P.ROOT, 'local', 'games', 'aoc', 'decomp', 'dll', short, 'functions.tsv'), hand={}, by_name=True)
print('%d of %d functions paired' % (len(P.pair), len(P.A.fns)), file=sys.stderr)


def show(v, want):
    for (off, size), c in sorted(v.items()):
        if want and off not in want: continue
        tot = sum(c.values())
        print('%5X %-5s -> %s' % (off, size, '  '.join('%X x%d' % (y, n) for y, n in c.most_common(4))) +
              ('' if len(c) == 1 else '   (%d%%)' % (100 * c.most_common(1)[0][1] // tot)))


if what == 'fields':
    classes, want = args[0].split(','), {int(x, 16) for x in args[1:]}
    v = collections.defaultdict(collections.Counter)
    for cls in classes:
        fs = F.class_methods(cls) | {f for f, n in P.A.name.items() if n.startswith(cls + '::') and f in P.pair}
        print('%s: %d methods' % (cls, len(fs)))
        for f in fs:
            for k, c in F.votes_of(f, P.pair[f]).items(): v[k].update(c)
    show(v, want)
elif what == 'global':
    want = {int(x, 16) for x in args}
    v = collections.defaultdict(collections.Counter)
    for f, g in P.pair.items():
        for ka, kb in F.loose_align(f, g):
            for (sa, xa, da), (sb, xb, db) in zip(F.MEM.findall(F.raw(P.A, f, ka)), F.MEM.findall(F.raw(P.B, g, kb))):
                if sa == sb and xa not in ('esp', 'ebp') and da: v[(int(da, 0), sa)][int(db, 0)] += 1
    show(v, want)
elif what == 'rva':
    for a in args:
        x = P.A.ib + int(a, 16)
        y, why = P.port(x)
        f = P.A.func_of(x)
        print('%06X -> %06X  %s  (in %s)' % (x - P.A.ib, y - P.B.ib if y else 0, why, P.A.name.get(f, '?')))
elif what == 'export':
    na, nb = P.A.names(), P.B.names()
    for n in args:
        print('%-50s %06X %06X' % (n, na[n] - P.A.ib if n in na else 0, nb[n] - P.B.ib if n in nb else 0))
