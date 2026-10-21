# Checks of fields.h and addr.h's Art of Conquest column against the code, beyond port_fields.py's class methods (which
# miss objects with few virtual functions - the AI's build, unit and civilization managers moved and were taken as
# kept): a kind's rows voted in its source file's code (the files' bounds: the static initializers Ghidra names
# "<Class>_persist_", decomp/functions.tsv), through the pointer that reaches it, and every function's stack arguments.
# usage: python port_check.py kinds [KIND ...]        each row of the kinds in RANGES: the votes, DIFF where they disagree
#        python port_check.py via KIND lo-hi[,..] kA kB  the kind's rows read through [this + kA] (AoC: kB) in those ranges
#        python port_check.py rets                      addr.h's functions whose stack arguments (ret N) differ by build
import collections, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs as P
import port_fields as F

ROOT = P.ROOT
# a kind's code: the 2017 update's address ranges (its file, or the files that use it)
RANGES = {
    'BM': ['464fcf-46bfc3'], 'CM': ['46bfc3-47c495'], 'CMLIST': ['46bfc3-47c495'], 'AM': ['46bfc3-47c495'],
    'AM4C': ['440000-47c495'], 'AMCMD': ['440000-47c495'], 'STRATEGY': ['46bfc3-47c495'], 'TASK': ['440000-464fcf'],
    'AG': ['440000-464fcf'], 'ASG': ['440000-464fcf'], 'ASGU': ['440000-464fcf'], 'ASGUINFO': ['440000-464fcf'],
    'ZONE': ['47c495-48580b', '48730b-489d91'], 'SITEOWN': ['47c495-48580b', '48730b-489d91'], 'RS': ['48580b-48730b'],
    'RM': ['489d91-48e85b'], 'UM': ['48e85b-49741d'], 'PLB98': ['42f434-464fcf'], 'PL': ['55370b-5bb064', '42f434-464fcf'],
    'TECH': ['5bb064-5e820a'], 'TECHDATA': ['5bb064-5e820a'], 'TECHNODE': ['5bb064-5e820a'], 'TECHCHILD': ['5bb064-5e820a'],
    'TT': ['5bb064-5e820a'], 'TTREC': ['5bb064-5e820a'], 'EPOCHTECHS': ['5a0000-5e820a'], 'EFFECT': ['5a0000-5e820a'],
    'SCEN': ['590000-5bb064'], 'SETUP': ['521804-53d468'], 'SETUPSLOT': ['521804-53d468'], 'GAMEDATA': ['521804-55370b'],
    'DMGMODEL': ['4d0000-4d84b8'], 'CAMERA': ['518000-521804'], 'USTAT': ['5e820a-628372'], 'UNIT': ['616178-628372', '5e820a-5f5938'],
    'ROUTENODE': ['400000-420000'], 'ROUTEPLANNER': ['400000-420000'], 'SCROLL': ['404e00-405400'],
    'GRID': ['49c657-4d0000', '53f852-55370b'], 'GRIDCELL': ['49c657-4d0000', '53f852-55370b'],
}


def rows(kind):
    src = open(os.path.join(ROOT, 'dll', 'game', 'fields.h'), encoding='latin-1').read()
    return [(int(o, 16), None if v == 'NA' else int(v, 16), how) for o, v, how in
            re.findall(r'O\(%s, 0x([0-9A-F]+), 0x[0-9A-F]+, (0x[0-9A-F]+|NA)\) /\* (.*?) \*/' % kind, src)]


def show(kind, this, other):
    for o, v, how in rows(kind):
        f = lambda c: ' '.join('%X:%d' % (y, m) for y, m in c.most_common(3)) or '-'
        top = (this[o] or other[o]).most_common(1)
        diff = top and top[0][0] != v and (top[0][1] >= 2 or not this[o])
        print('  %5X -> %-5s %-2s this %-20s any %-24s %s' % (o, '%X' % v if v is not None else 'NA', how[:1], f(this[o]),
                                                            f(other[o]), 'DIFF' if diff else ''))


def range_votes(ranges, offs):
    """[this + X] and [any register + X] in the paired functions in the ranges: X -> Counter of AoC's"""
    th, an = collections.defaultdict(collections.Counter), collections.defaultdict(collections.Counter)
    for f, g in P.pair.items():
        if not any(lo <= f < hi for lo, hi in ranges): continue
        try:
            for k, c in F.votes_of(f, g).items():
                if k[0] in offs: th[k[0]].update(c)
            for ka, kb in F.loose_align(f, g):
                for (sa, xa, da), (sb, xb, db) in zip(F.MEM.findall(F.raw(P.A, f, ka)), F.MEM.findall(F.raw(P.B, g, kb))):
                    if sa == sb and xa not in ('esp', 'ebp') and da and int(da, 0) in offs: an[int(da, 0)][int(db, 0)] += 1
        except StopIteration:
            pass
    return th, an


def held(e, f, k):
    """per instruction: the registers holding [this + k] (loaded, then copied)"""
    th, out, regs = F.this_regs(e, f), [], set()
    for i, (a, n, t, vals) in enumerate(e.insns(f)):
        out.append(frozenset(regs))
        ins = next(e.cs.disasm(e.rd(a, n), a))
        dst = ins.op_str.split(',')[0].strip()
        src = ins.op_str.split(',')[1].strip() if ',' in ins.op_str else ''
        m = re.match(r'dword ptr \[(e\w\w) \+ (0x[0-9a-f]+)\]$', src)
        if ins.mnemonic == 'mov' and dst in F.REGS and m and m.group(1) in th[i] and int(m.group(2), 16) == k: regs = regs | {dst}
        elif ins.mnemonic == 'mov' and dst in F.REGS and src in regs: regs = regs | {dst}
        elif ins.mnemonic.startswith('call'): regs = regs - {'eax', 'ecx', 'edx'}
        elif dst in F.REGS and not ins.mnemonic.startswith(F.NOWRITE): regs = regs - {dst}
    return out


def via_votes(ranges, ka, kb):
    v = collections.defaultdict(collections.Counter)
    for f, g in P.pair.items():
        if not any(lo <= f < hi for lo, hi in ranges): continue
        try:
            ha, hb = held(P.A, f, ka), held(P.B, g, kb)
            for ia, ib in F.loose_align(f, g):
                for (sa, xa, da), (sb, xb, db) in zip(F.MEM.findall(F.raw(P.A, f, ia)), F.MEM.findall(F.raw(P.B, g, ib))):
                    if xa in ha[ia] and xb in hb[ib] and sa == sb: v[int(da or '0', 0)][int(db or '0', 0)] += 1
        except StopIteration:
            pass
    return v


def rng(spec): return [tuple(int(x, 16) for x in r.split('-')) for r in spec]


if __name__ == '__main__':
    P.load()
    what, args = sys.argv[1], sys.argv[2:]
    if what == 'kinds':
        for k in args or sorted(RANGES):
            if not rows(k): continue
            print('== %s' % k)
            th, an = range_votes(rng(RANGES[k]), {o for o, _, _ in rows(k)})
            show(k, th, an)
    elif what == 'via':
        v = via_votes(rng(args[1].split(',')), int(args[2], 16), int(args[3], 16))
        show(args[0], v, collections.defaultdict(collections.Counter))
    elif what == 'rets':
        src = open(os.path.join(ROOT, 'dll', 'game', 'addr.h'), encoding='latin-1').read()
        for name, rest in re.findall(r'^\s*[FD]\((\w+),(.*)$', src, re.M):
            v = re.findall(r'0x[0-9A-Fa-f]{8}', rest.split('/*')[0])
            if len(v) < 2: continue
            a, b = int(v[0], 16), int(v[1], 16)
            if a not in P.A.size or b not in P.B.size: continue
            ra = sorted({t for t in P.texts(P.A, a) if t.startswith('ret')})
            rb = sorted({t for t in P.texts(P.B, b) if t.startswith('ret')})
            if ra != rb: print('%-26s %X %-14s %X %s' % (name, a, ra, b, rb))
