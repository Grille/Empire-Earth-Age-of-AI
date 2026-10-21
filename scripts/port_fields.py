# Find the struct offsets of another exe build (Art of Conquest): in the functions port_addrs.py paired, aligned with
# every number masked, an instruction reading [this + X] in the base reads [this + Y] in the other - for each class
# (its vtable's functions: `this` is ecx at the start, and the registers it is copied to) the pairs X -> Y and how
# often each was seen. A class's fields are its base classes' too: a type's votes are those of its classes together.
# usage: python port_fields.py Class[,Class...] [offset ...]    (offsets hex; without: every offset seen)
#        python port_fields.py --all <file.json>      every class's votes (a few minutes)
#        python port_fields.py --global <file.json>   every offset's, whatever the object (for structures without methods)
# The table it feeds: dll/game/fields.h (its header says how each value was found).
import collections, difflib, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs as P

HEX = re.compile(r'0x[0-9a-f]+')
REGS = ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp')
NOWRITE = ('cmp', 'test', 'push', 'call', 'j', 'fld', 'fild', 'fcom', 'fucom', 'fadd', 'fsub', 'fmul', 'fdiv', 'ret')
MEM = re.compile(r'(byte ptr |word ptr |dword ptr |qword ptr |tbyte ptr |)\[(e[a-z]{2})(?: \+ (0x[0-9a-f]+|\d+))?\]')   # lea: no size


def this_regs(e, f):
    """per instruction of f: the registers that hold `this` before it (ecx at the start, its copies)"""
    regs, out = {'ecx'}, []
    for _, _, t, _ in e.insns(f):
        out.append(frozenset(regs))
        m, _, ops = t.partition(' ')
        dst = ops.split(',')[0].strip()
        if m == 'mov' and dst in REGS and ops.split(',')[1].strip() in regs: regs = regs | {dst}
        elif m.startswith('call'): regs = regs - {'eax', 'ecx', 'edx'}
        elif dst in REGS and not m.startswith(NOWRITE): regs = regs - {dst}
    return out


def loose_align(fa, fb):
    ta = [HEX.sub('N', t) for t in P.texts(P.A, fa)]
    tb = [HEX.sub('N', t) for t in P.texts(P.B, fb)]
    m = difflib.SequenceMatcher(None, ta, tb, autojunk=False)
    return [(blk.a + k, blk.b + k) for blk in m.get_matching_blocks() for k in range(blk.size)]


def raw(e, f, k):
    """instruction k of f as capstone prints it (the numbers kept)"""
    a, n, _, _ = e.insns(f)[k]
    i = next(e.cs.disasm(e.rd(a, n), a))
    return i.mnemonic + ' ' + i.op_str


def votes_of(fa, fb):
    """(offset in the base, size) -> Counter of the other's offsets, for the `this` reads of one function pair"""
    out = collections.defaultdict(collections.Counter)
    ra, rb = this_regs(P.A, fa), this_regs(P.B, fb)
    for ka, kb in loose_align(fa, fb):
        ma, mb = MEM.findall(raw(P.A, fa, ka)), MEM.findall(raw(P.B, fb, kb))
        for (sa, xa, da), (sb, xb, db) in zip(ma, mb):
            if xa in ra[ka] and xb in rb[kb] and sa == sb:
                out[(int(da or '0', 0), sa)][int(db or '0', 0)] += 1
    return out


_methods = {}
def class_methods(cls):
    """the paired functions of the class: its vtable's (at offset 0), and what they call with `this` in ecx - their
    non-virtual methods (followed until nothing new; a base class's methods come along, their fields are the same)"""
    if cls in _methods: return _methods[cls]
    fs = set()
    for vt, (c, off, n) in P.A.vt.items():
        if c == cls and off == 0:
            fs |= {P.A.u32(vt + 4 * k) for k in range(n)}
    fs = {f for f in fs if f in P.A.size}
    todo = list(fs)
    while todo:
        f = todo.pop()
        regs = this_regs(P.A, f)
        for k, (_, _, t, vals) in enumerate(P.A.insns(f)):
            if t == 'call X' and 'ecx' in regs[k] and vals[0] in P.A.size and vals[0] not in fs:
                fs.add(vals[0]); todo.append(vals[0])
    _methods[cls] = {f for f in fs if f in P.pair}
    return _methods[cls]


def field_votes(classes):
    v = collections.defaultdict(collections.Counter)
    for cls in classes:
        for f in class_methods(cls):
            for k, c in votes_of(f, P.pair[f]).items(): v[k].update(c)
    return v


def all_votes(path):
    """every class's votes, saved as JSON {class: {"off": {"other off": n}}} (port_fields.py --all <file>)"""
    import json
    per_fn = {}
    for f in P.pair:
        try: per_fn[f] = votes_of(f, P.pair[f])
        except StopIteration: pass
    out = {}
    for cls in sorted({c for c, off, n in P.A.vt.values()}):
        v = collections.defaultdict(collections.Counter)
        for f in class_methods(cls):
            for (off, _), c in per_fn.get(f, {}).items(): v[off].update(c)
        if v: out[cls] = {'%X' % k: {'%X' % y: n for y, n in c.items()} for k, c in v.items()}
    json.dump(out, open(path, 'w'))


if __name__ == "__main__" and sys.argv[1] == "--all":
    all_votes(sys.argv[2])
elif __name__ == "__main__" and sys.argv[1] != "--global":
    classes = sys.argv[1].split(',')
    want = {int(x, 16) for x in sys.argv[2:]}
    v = field_votes(classes)
    print('%d methods' % sum(len(class_methods(c)) for c in classes))
    for (off, size), c in sorted(v.items()):
        if want and off not in want: continue
        tot = sum(c.values())
        print('%5X %-5s -> %s' % (off, size, '  '.join('%X x%d' % (y, n) for y, n in c.most_common(4))) + ('' if len(c) == 1 else '   (%d%%)' % (100 * c.most_common(1)[0][1] // tot)))


def global_votes(path):
    """every [reg + X] -> [reg + Y] of the aligned code, whatever the object (port_fields.py --global <file>): an offset
    that kept its value everywhere is the same in every object; one that moved needs its object's votes"""
    import json
    v = collections.defaultdict(collections.Counter)
    for f in P.pair:
        fb = P.pair[f]
        try:
            for ka, kb in loose_align(f, fb):
                ma, mb = MEM.findall(raw(P.A, f, ka)), MEM.findall(raw(P.B, fb, kb))
                for (sa, xa, da), (sb, xb, db) in zip(ma, mb):
                    if sa == sb and xa not in ('esp', 'ebp') and da: v[int(da, 0)][int(db, 0)] += 1
        except StopIteration: pass
    json.dump({'%X' % k: {'%X' % y: n for y, n in c.items()} for k, c in v.items()}, open(path, 'w'))


if __name__ == "__main__" and sys.argv[1] == "--global":
    global_votes(sys.argv[2])
