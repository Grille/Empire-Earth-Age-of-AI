# Checks of the spec's Art of Conquest values (spec/game/: fields, functions) against the code, beyond port_fields.py's
# class methods (which miss objects with few virtual functions - the AI's build, unit and civilization managers moved
# and were taken as kept): a class's fields voted in its source file's code (the files' bounds: the static initializers
# Ghidra names "<Class>_persist_", local/games/ee/decomp/functions.tsv), through the pointer that reaches it, and every
# function's stack arguments.
# usage: python port_check.py classes [CLASS ...]        each field of the classes in RANGES: the votes, DIFF where they disagree
#        python port_check.py via CLASS lo-hi[,..] kA kB  the class's fields read through [this + kA] (AoC: kB) in those ranges
#        python port_check.py rets                       the functions whose stack arguments (ret N) differ by build
import collections, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs as P
import port_fields as F
import spec_read

ROOT = P.ROOT
# a class's code: the 2017 update's address ranges (its file, or the files that use it)
UNIT = ['616178-628372', '5e820a-5f5938']
RANGES = {
    'ee_build_mgr': ['464fcf-46bfc3'], 'EECPCivilizationManager': ['46bfc3-47c495'], 'ee_plan_node': ['46bfc3-47c495'],
    'EECPCombatManager': ['46bfc3-47c495'], 'ee_combat_mgr_4c': ['440000-47c495'], 'ee_attack_cmd': ['440000-47c495'],
    'EECPCombatStrategy': ['46bfc3-47c495'], 'EECPAGTask': ['440000-464fcf'], 'EECPAttackGroup': ['440000-464fcf'],
    'EECPAttackSubGroup': ['440000-464fcf'], 'EECPAttackSubGroupUnit': ['440000-464fcf'], 'ee_asgu_info': ['440000-464fcf'],
    'ee_zone': ['47c495-48580b', '48730b-489d91'], 'ee_unit_vis': ['47c495-48580b', '48730b-489d91'],
    'EECPResearchManager': ['48580b-48730b'], 'EECPResourceManager': ['489d91-48e85b'], 'ee_unit_mgr': ['48e85b-49741d'],
    'ee_player_b98': ['42f434-464fcf'], 'EEPlayer': ['55370b-5bb064', '42f434-464fcf'],
    'EETechTreeNode': ['5bb064-5e820a'], 'ee_tech_data': ['5bb064-5e820a'], 'EETechTree': ['5bb064-5e820a'],
    'ee_tt_epoch': ['5bb064-5e820a'], 'ee_epoch_techs': ['5a0000-5e820a'], 'ee_effect': ['5a0000-5e820a'],
    'EEScenarioManager': ['590000-5bb064'], 'EESMTrigger': ['590000-5bb064'], 'ee_trig_cond': ['590000-5bb064'],
    'ee_trig_effect': ['590000-5bb064'], 'ee_scen_cond': ['590000-5bb064'], 'ee_scen_effect': ['590000-5bb064'],
    'ee_setup': ['521804-53d468'], 'ee_setup_slot': ['521804-53d468'], 'ee_game_data': ['521804-55370b'],
    'ee_damage_model': ['4d0000-4d84b8'], 'EECameraControl': ['518000-521804'], 'ee_unit_stats': ['5e820a-628372'],
    'RTSUnit': UNIT, 'EEUnit': UNIT, 'EEComplexUnit': UNIT,
    'ee_route_node': ['400000-420000'], 'EEAIGPGModifierLandAttackSubGroupRoutePlanner': ['400000-420000'],
    'EEAScroll': ['404e00-405400'], 'ee_grid': ['49c657-4d0000', '53f852-55370b'], 'ee_grid_cell': ['49c657-4d0000', '53f852-55370b'],
}
SPEC = spec_read.read_all()


def rows(cls):
    """the class's fields in the spec: (the 2017 offset, AoC's or None, how it was found)"""
    return [(v[0], None if v[1] == spec_read.NONE else v[1], how) for _, v, how in spec_read.fields(cls, SPEC)]


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
    if what == 'classes':
        for k in args or sorted(RANGES):
            if not rows(k): continue
            print('== %s' % k)
            th, an = range_votes(rng(RANGES[k]), {o for o, _, _ in rows(k)})
            show(k, th, an)
    elif what == 'via':
        v = via_votes(rng(args[1].split(',')), int(args[2], 16), int(args[3], 16))
        show(args[0], v, collections.defaultdict(collections.Counter))
    elif what == 'rets':
        for name, _, (a, b), m in spec_read.places(SPEC):
            if m is None or not m.func: continue
            if a not in P.A.size or b not in P.B.size: continue
            ra = sorted({t for t in P.texts(P.A, a) if t.startswith('ret')})
            rb = sorted({t for t in P.texts(P.B, b) if t.startswith('ret')})
            if ra != rb: print('%-26s %X %-14s %X %s' % (name, a, ra, b, rb))
