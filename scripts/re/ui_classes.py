# The user interface's classes in both exes (the 2017 update and Art of Conquest), by their RTTI: each class's vtable,
# its bases, the functions that construct it (an out-of-line constructor: its address; one inlined into a factory: the
# factory's) and the object's size (the operator new's argument before the construction). For the spec's UI files
# (spec/game/ui.cs, ui_forms.cs, ui_events.cs, editor.cs): a class's [vtable], [size] and init.
# A constructor is a function that writes the class's vtable to [reg] and is not its destructor (slot 0, the scalar
# deleting destructor, and what it calls); a function writing several classes' vtables is each one's that isn't a base
# of another written there (a constructor writes its bases' too). Code Ghidra left without a function (the UI manager's
# event creators) is one from each `&LAB_x` of the decompilation. Needs unicorn (requirements.txt) for --events.
# usage: python ui_classes.py [regex of class names] [--json out.json]   (default: the UI's prefixes, a table on stdout)
#        python ui_classes.py --events   each event id -> the class EEUIManager's event creators make, both builds
import collections, json, os, re, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs

GAME_NEW = {'ee': 0x00683718, 'aoc': 0x0069D178}   # spec/game/game.cs game_functions.game_new
UI = r'^(EEUI|UI[A-Z]|EESE|EESM|EEButtonObject|EECBUIListBox|EECampaignFormManager|EEPGSFormManager|' \
     r'EEWonNetAvailableGameFormManager|EEDSUI)'
MOVS = {0x00, 0x01, 0x02, 0x03, 0x06, 0x07}   # C7 modrm: mov dword ptr [eax|ecx|edx|ebx|esi|edi], imm32


def bases(e, vt):
    """the class's bases from its complete object locator's hierarchy: itself first, then each base depth first"""
    col = e.u32(vt - 4)
    chd = e.u32(col + 16)
    n, bca = e.u32(chd + 8), e.u32(chd + 12)
    out = []
    for i in range(n):
        td = e.u32(e.u32(bca + 4 * i))
        s = e.rd(td + 8, 200)
        out.append(s[4:s.index(b'@@')].decode('latin-1'))
    return out


def labels(e, decomp):
    """code Ghidra left without a function whose address the code takes (`&LAB_x` in the decompilation: the UI
    manager's event creators, 0x659CDD) made functions up to the next start - a vtable written there is found too"""
    found = set()
    for name in os.listdir(decomp):
        if name.endswith('.c'):
            with open(os.path.join(decomp, name), encoding='utf-8', errors='replace') as f:
                found.update(int(m, 16) for m in re.findall(r'&LAB_([0-9a-f]{8})', f.read()))
    import bisect
    for a in sorted(found):
        if not e.in_code(a) or e.func_of(a): continue
        i = bisect.bisect_right(e.starts, a)
        end = e.starts[i] if i < len(e.starts) else e.code[1]
        e.fns.insert(i, (a, end - a)); e.starts.insert(i, a); e.size[a] = end - a; e.name[a] = 'LAB_%08x' % a
    # a label inside another label's range ends that one
    for i in range(len(e.starts) - 1):
        a, n = e.fns[i]
        if a + n > e.starts[i + 1]:
            e.fns[i] = (a, e.starts[i + 1] - a); e.size[a] = e.starts[i + 1] - a


def scan(e, build, pattern):
    prim = {}
    for vt, (cls, off, n) in e.vt.items():
        if off == 0 and re.search(pattern, cls) and '@' not in cls:
            prim.setdefault(cls, (vt, n))
    by_vt = {vt: cls for cls, (vt, _) in prim.items()}
    lo, hi = e.code
    code = e.rd(lo, hi - lo)
    calls = collections.defaultdict(list)   # target -> call sites (E8 rel32; checked against the caller's code later)
    for m in re.finditer(rb'\xE8', code):
        o = m.start()
        if o + 5 <= len(code):
            calls[(lo + o + 5 + struct.unpack('<i', code[o + 1:o + 5])[0]) & 0xFFFFFFFF].append(lo + o)
    writes = collections.defaultdict(list)   # function -> [(address, class)]
    for m in re.finditer(rb'\xC7[\x00-\x03\x06\x07]', code):
        o = m.start()
        v = struct.unpack('<I', code[o + 2:o + 6])[0]
        if v in by_vt:
            f = e.func_of(lo + o)
            if f: writes[f].append((lo + o, by_vt[v]))
    depth = {cls: bases(e, vt) for cls, (vt, _) in prim.items()}

    def target(va):
        f = e.u32(va)
        b = e.rd(f, 5)
        return f + 5 + struct.unpack('<i', b[1:5])[0] if b[0] == 0xE9 else f

    dtors = collections.defaultdict(set)
    for cls, (vt, _) in prim.items():
        f0 = target(vt)
        dtors[cls].add(f0)
        if f0 in e.size:
            for a, sz, t, vals in e.insns(f0):
                if t.startswith('call X') and vals: dtors[cls].add(vals[0])

    def before(at, n=0xC0):
        """the instructions before `at`, disassembled from the furthest start whose instructions end exactly there
        (x86 resynchronizes within a few; Ghidra's list cuts some switches short, so no function bounds)"""
        for back in range(n, 0, -1):
            ins = list(e.cs.disasm(e.rd(at - back, back), at - back))
            if ins and ins[-1].address + ins[-1].size == at:
                return ins
        return []

    def sized(_, at):
        """the operator new's argument before the call or write at `at` (None: none within 40 instructions)"""
        ins = before(at)[-40:]
        for i in range(len(ins) - 1, -1, -1):
            if ins[i].mnemonic == 'call' and ins[i].op_str == hex(GAME_NEW[build]):
                for j in range(i - 1, max(i - 6, -1), -1):
                    m = re.match(r'(0x[0-9a-f]+|\d+)$', ins[j].op_str)
                    if ins[j].mnemonic == 'push' and m: return int(m.group(1), 0)
                return None
        return None

    out = {}
    for cls, (vt, n) in prim.items():
        out[cls] = {'vtable': vt, 'slots': n, 'bases': depth[cls][1:], 'slot0': target(vt),
                    'slot1': target(vt + 4) if n > 1 else None, 'ctors': [], 'factories': [], 'sizes': []}
    for f, ws in writes.items():
        written = {c for _, c in ws}
        for cls in written:
            # a base's vtable written by its derived class's construction (or destruction) isn't the base's
            if any(cls in depth[d][1:] for d in written) or f in dtors[cls]: continue
            r = out[cls]
            sizes = [s for s in (sized(f, a) for a, c in ws if c == cls) if s is not None]
            if sizes:   # constructed inline after the operator new: a factory
                r['factories'].append(f); r['sizes'] += sizes; continue
            r['ctors'].append(f)
    owners = collections.Counter(f for r in out.values() for f in r['ctors'])
    for r in out.values():
        for f in r['ctors']:
            if owners[f] > 1: continue   # an object's constructor that builds members of other classes: its callers'
            for site in calls.get(f, []):   # operator new is the object's, not the members'
                ins = before(site + 5, 0x40)   # a call there, not the bytes of another instruction
                if not ins or ins[-1].address != site or ins[-1].mnemonic != "call": continue
                s = sized(None, site)
                if s is not None: r['sizes'].append(s)
        r['sizes'] = sorted(set(r['sizes']))
        r['ctors'].sort(); r['factories'].sort()
    return out


def creators(e, mgr_ctor):
    """the event creators EEUIManager's constructor pushes onto its list (+0x1C), in order: mov [ebp+x], creator (or
    a register holding it) before each call of the push"""
    regs, out = {}, []
    for i in e.cs.disasm(e.rd(mgr_ctor, 0x200), mgr_ctor):
        m = re.match(r'(e[a-d]x|e[sd]i), (0x[0-9a-f]+)$', i.op_str)
        if i.mnemonic == 'mov' and m: regs[m.group(1)] = int(m.group(2), 16)
        m = re.match(r'dword ptr \[ebp \+ 0x10\], (\w+)$', i.op_str)
        if i.mnemonic == 'mov' and m:
            v = regs.get(m.group(1)) or (int(m.group(1), 16) if m.group(1).startswith('0x') else None)
            if v and e.in_code(v): out.append(v)
        if i.mnemonic == 'ret': break
    return out


def events(e, build, classes, mgr_ctor, top=0x400):
    """event id -> (creator, class): each creator run for each id under an emulator (cdecl(form, id); the form a
    zeroed object; the operator new a fresh zeroed block; any other call skipped, eax a zeroed block) until it writes
    a UI class's vtable or calls a class's constructor - the class of that id; a ret without one: none"""
    import unicorn as uc
    from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
    vts = {r['vtable']: c for c, r in classes.items()}
    ctors = {f: c for c, r in classes.items() for f in r['ctors']}
    mu = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
    size = (len(e.img) + 0xFFF) & ~0xFFF
    mu.mem_map(e.ib, size)
    mu.mem_write(e.ib, bytes(e.img))
    ZERO, STACK, HEAP = 0x1000, 0x10000000, 0x20000000
    mu.mem_map(0, 0x100000)   # the zeroed objects (and reads through their null fields) land here
    mu.mem_map(STACK - 0x10000, 0x20000)
    mu.mem_map(HEAP, 0x1000000)
    state = {}

    def on_code(mu, addr, n, _):
        state['steps'] += 1
        if state['steps'] > 5000: mu.emu_stop(); return
        b = e.rd(addr, 6)
        target = None
        if b[0] == 0xE8: target = (addr + 5 + struct.unpack('<i', b[1:5])[0]) & 0xFFFFFFFF; ln = 5
        elif b[:2] == b'\xFF\x15': target = e.u32(addr + 2); ln = 6   # call [import slot]: never a class's
        elif b[0] == 0xC3 or b[0] == 0xC2: mu.emu_stop(); return
        if target is None: return
        if target in ctors:
            state['hit'] = ctors[target]; mu.emu_stop(); return
        if target == GAME_NEW[build]:
            state['heap'] += 0x10000
            mu.mem_write(state['heap'], b'\0' * 0x10000)
            mu.reg_write(UC_X86_REG_EAX, state['heap'])
        else:
            mu.reg_write(UC_X86_REG_EAX, ZERO)
        mu.reg_write(UC_X86_REG_EIP, addr + ln)

    def on_write(mu, access, addr, n, value, _):
        if n == 4 and (value & 0xFFFFFFFF) in vts:
            state['hit'] = vts[value & 0xFFFFFFFF]; mu.emu_stop()

    mu.hook_add(uc.UC_HOOK_CODE, on_code)
    mu.hook_add(uc.UC_HOOK_MEM_WRITE, on_write)
    out = {}
    for cr in dict.fromkeys(creators(e, mgr_ctor)):
        for i in range(top):
            mu.mem_write(0, b'\0' * 0x100000)
            sp = STACK - 0x100
            mu.mem_write(sp, struct.pack('<III', 0xDEAD0000, ZERO, i))   # return address, form, id
            mu.reg_write(UC_X86_REG_ESP, sp)
            state.update(steps=0, hit=None, heap=HEAP - 0x10000)
            try:
                mu.emu_start(cr, 0xDEAD0000)
            except uc.UcError:
                pass
            if state['hit']: out.setdefault(i, (cr, state['hit']))
    return out


def main():
    argv = sys.argv[1:]
    out = argv.pop(argv.index('--json') + 1) if '--json' in argv else None
    args = [a for a in argv if not a.startswith('--')]
    pattern = args[0] if args else UI
    ee = port_addrs.Exe(port_addrs.EXE_A, port_addrs.FN_A)
    aoc = port_addrs.Exe(port_addrs.EXE_B, port_addrs.FN_B)
    labels(ee, os.path.dirname(port_addrs.FN_A))
    labels(aoc, os.path.dirname(port_addrs.FN_B))
    res = {'ee': scan(ee, 'ee', pattern), 'aoc': scan(aoc, 'aoc', pattern)}
    if '--events' in argv:   # event id -> the class its creator makes, both builds
        ev = {b: events(x, b, res[b], res[b]['EEUIManager']['ctors'][0]) for b, x in (('ee', ee), ('aoc', aoc))}
        for i in sorted(set(ev['ee']) | set(ev['aoc'])):
            a, b = ev['ee'].get(i), ev['aoc'].get(i)
            print('%4d 0x%03X  %-44s %s  %s%s' % (i, i, a[1] if a else '-', '%06X' % a[0] if a else '-',
                                                  '%06X' % b[0] if b else '-',
                                                  '' if (a and b and a[1] == b[1]) else '  AoC: %s' % (b and b[1])))
        return
    if out:
        with open(out, 'w') as f:
            json.dump(res, f, indent=1)
        return
    h = lambda v: '%06X' % v if v else '-'
    for cls in sorted(res['ee']):
        a, b = res['ee'][cls], res['aoc'].get(cls)
        print('%-44s %s/%s %3d  ctor %s/%s  fact %s  size %s/%s  base %s' % (
            cls, h(a['vtable']), h(b and b['vtable']), a['slots'], ','.join(map(h, a['ctors'])) or '-',
            ','.join(map(h, b['ctors'])) if b else '-', ','.join(map(h, a['factories'])) or '-',
            ','.join('%X' % s for s in a['sizes']) or '-', ','.join('%X' % s for s in b['sizes']) if b else '-',
            a['bases'][0] if a['bases'] else '-'))
    print(len(res['ee']), 'classes;', len(res['aoc']), 'in AoC; only in EE:',
          sorted(set(res['ee']) - set(res['aoc'])), 'only in AoC:', sorted(set(res['aoc']) - set(res['ee'])))


if __name__ == '__main__':
    main()
