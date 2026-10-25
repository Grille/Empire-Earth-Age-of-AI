"""The fields the game's code reads and the spec doesn't name, by class: the decompilation made after ApplySpec
(local/games/<build>/decomp/, README "Setup") shows an access to such a field as var->field_0xNN of the spec's
structure - counted here by the variable's declared type (a parameter, a local, this), with the functions that do it.
usage: python scripts/re/spec_gaps.py [ee|aoc] [Class ...]   (no class: a summary of every class)"""
import collections, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
build = sys.argv[1] if len(sys.argv) > 1 else 'ee'
only = set(sys.argv[2:])
D = os.path.join(ROOT, 'local', 'games', build, 'decomp')
DECL = re.compile(r'\b([A-Za-z_]\w*)\s*\*\s*(\*?\s*)?([A-Za-z_]\w*)\s*[;,)]')
ACC = re.compile(r'\b([A-Za-z_]\w*)->field_0x([0-9a-f]+)')
hits = collections.defaultdict(lambda: collections.defaultdict(list))   # class -> offset -> [(function, line)]
for f in sorted(os.listdir(D)):
    if not f.endswith('.c'):
        continue
    for body in open(os.path.join(D, f), encoding='utf-8', errors='replace').read().split('// ==== ')[1:]:
        head, _, rest = body.partition('\n')
        addr, _, name = head.partition(' ')
        types = {}
        sig = re.search(r'\(([^)]*)\)\s*\n\s*\{', body)
        m = re.search(r'(\w+)::\w+\s*\(', body)
        if m and '__thiscall' in body[:400]:
            types['this'] = m.group(1)
        for t, stars, v in DECL.findall(body[:body.find('\n\n', body.find('{')) if '{' in body else 600]):
            if not stars.strip():
                types[v] = t
        for line in rest.split('\n'):
            for v, off in ACC.findall(line):
                t = types.get(v)
                if t and (not only or t in only):
                    hits[t][int(off, 16)].append((addr + ' ' + name, line.strip()))
for t in sorted(hits, key=lambda t: -sum(len(set(x[0] for x in v)) for v in hits[t].values())):
    offs = hits[t]
    if only:
        print('== %s' % t)
        for off in sorted(offs):
            fns = sorted(set(x[0] for x in offs[off]))
            print('  +0x%X  %d functions: %s' % (off, len(fns), ', '.join(fns[:4]) + (' ...' if len(fns) > 4 else '')))
            print('        %s' % offs[off][0][1][:150])
    else:
        print('%-40s %3d offsets  %s' % (t, len(offs), ' '.join('+0x%X(%d)' % (o, len(set(x[0] for x in offs[o])))
                                                              for o in sorted(offs))[:200]))
