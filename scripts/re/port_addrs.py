# Find the addresses the spec names (spec/game/: [address], [call_site], [vtable], [import]) in another exe build (Art of
# Conquest): pairs the two exes' functions by their code with the
# addresses masked (Ghidra's function lists: local/games/ee/decomp/functions.tsv), then follows the pairs - the calls of a paired
# function pair their callees, its operands pair the data, an instruction inside a paired function is found by aligning
# the two functions' code. Vtables are paired by their RTTI class (and so their slots' functions), imports by name;
# functions whose struct offsets moved by their code with every number masked. Each line says how it was found and
# whether the spec's AoC value agrees (same, DIFFERS, NEW where the spec has none); a value goes into the spec by hand.
# usage: python port_addrs.py [other.exe] [other functions.tsv]   (default: the AoC exe beside the game, local/games/aoc/decomp)
# Imported, load() pairs two images (the exes by default; port_dll.py: an engine DLL's two builds, by name first).
import bisect, collections, difflib, os, re, struct, sys
import capstone, pefile
sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))   # scripts/: game_dir.py
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build'))   # spec_read.py
from game_dir import game_dir
import spec_read

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = game_dir()
EXE_A = os.path.join(GAME, 'Empire Earth.exe')
ARGS = sys.argv[1:] if __name__ == '__main__' else []
EXE_B = ARGS[0] if len(ARGS) > 0 else os.path.join(os.path.dirname(GAME), 'Empire Earth - The Art of Conquest', 'EE-AOC.exe')
FN_A = os.path.join(ROOT, 'local', 'games', 'ee', 'decomp', 'functions.tsv')
FN_B = ARGS[1] if len(ARGS) > 1 else os.path.join(ROOT, 'local', 'games', 'aoc', 'decomp', 'functions.tsv')
HEX = re.compile(r'0x[0-9a-f]+')


class Exe:
    def __init__(self, path, fns):
        pe = pefile.PE(path)
        self.ib = pe.OPTIONAL_HEADER.ImageBase
        self.img = pe.get_memory_mapped_image()
        self.hi = self.ib + len(self.img)
        text = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
        self.code = (self.ib + text.VirtualAddress, self.ib + text.VirtualAddress + text.Misc_VirtualSize)
        self.imports = {}
        for d in pe.DIRECTORY_ENTRY_IMPORT:
            for i in d.imports:
                self.imports[i.address] = (d.dll.lower(), i.name or i.ordinal)
        self.fns, self.name = [], {}
        for l in open(fns, encoding='utf-8', errors='replace'):
            f = l.split('\t')
            if len(f) >= 3:
                self.fns.append((int(f[0], 16), int(f[2])))
                self.name[int(f[0], 16)] = f[1]
        self.fns.sort()
        self.starts = [a for a, _ in self.fns]
        self.size = dict(self.fns)
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self._code = {}
        self.vt = self.vtables()

    def names(self):
        """name -> function, for the names one function has (not Ghidra's FUN_ ones; overloads share a name)"""
        d = collections.defaultdict(list)
        for f, n in self.name.items():
            if not n.startswith('FUN_'): d[n].append(f)
        return {n: v[0] for n, v in d.items() if len(v) == 1}

    def rd(self, va, n): return self.img[va - self.ib:va - self.ib + n]
    def u32(self, va): return struct.unpack('<I', self.rd(va, 4))[0]
    def in_image(self, v): return self.ib <= v < self.hi
    def in_code(self, v): return self.code[0] <= v < self.code[1]

    def func_of(self, va):
        i = bisect.bisect_right(self.starts, va) - 1
        if i >= 0 and va < self.starts[i] + max(self.fns[i][1], 1): return self.starts[i]
        return None

    def insns(self, f):
        """the function's instructions: (address, size, text with the addresses masked, the addresses in order)"""
        if f in self._code: return self._code[f]
        out, end = [], f + self.size[f]
        for i in self.cs.disasm(self.rd(f, self.size[f]), f):
            vals, ops = [], i.op_str
            def norm(m):
                v = int(m.group(0), 16)
                if i.mnemonic.startswith(('call', 'j', 'loop')) and m.start() == 0 and len(ops) == m.end():
                    if f <= v < end: return 'L%x' % (v - f)
                    vals.append(v); return 'X'
                if self.in_image(v): vals.append(v); return 'A'
                return m.group(0)
            out.append((i.address, i.size, i.mnemonic + ' ' + HEX.sub(norm, ops), vals))
        self._code[f] = out
        return out

    def vtables(self):
        """RTTI: the address of each vtable (vtable[-1] is its complete object locator) -> (class, offset in the class,
        its slots)"""
        out = {}
        for m in re.finditer(rb'\.\?AV([^\0@]+)@@\0', self.img):
            td = self.ib + m.start() - 8
            for c in re.finditer(re.escape(struct.pack('<I', td)), self.img):
                col = self.ib + c.start() - 12
                if c.start() < 12 or self.u32(col) != 0: continue
                for r in re.finditer(re.escape(struct.pack('<I', col)), self.img):
                    vt = self.ib + r.start() + 4
                    n = 0
                    while self.in_code(self.u32(vt + 4 * n)): n += 1
                    if n: out[vt] = (m.group(1).decode('latin-1'), self.u32(col + 4), n)
        return out


# found by hand (EE-AOC.exe): the base's address -> the other's, and why
EXE_HAND = {
    0x0048BBA8: (0x00490602, 'find_food_game: AssignCitizens (0x48EECF) calls it six times where the base calls 0x48BBA8'),
    0x004050C2: (0x00404734, 'the scroll timer period: push 0xA in the EEAScroll ctor (0x404733; the base: push 0x19)'),
    0x00617780: (0x0062A2DF, 'the kill statistic: the call of player_stats_add just before the attack alert, as in the base'),
    0x006177CF: (0x0062A366, 'the only call of data_hooks_fn_attack_alert'),
    0x00607D79: (0x00619604, 'the second of the calls of unit_fire, after the projectile code (proj_down 0x619143)'),
    0x0050FEC8: (0x0051A487, 'the call of mouse_update the others leave'),
    0x004FCE70: (0x00507940, "the goal markers' cmp [selection + count], 10 (AoC's count at +0x538): the only such cmp before a setge"),
    0x005007B2: (0x0050B2AD, 'mov esi, data_hooks_draw_old + 0x58 (the draw records\' init loop)'),
    0x005007CC: (0x0050B2C7, 'cmp esi, its end (data_hooks_draw_old + 0x58 + 0x200 x 0x148): the operand of the cmp at 0x50B2C5'),
    0x005D88C0: (0x005E8B68, 'EEUIESPEMainMenu slot 1: AoC\'s copies the name field first (0x5E8ACC) - a partial fix of its own'),
    0x0054880E: (0x00554F56, 'physics_model_turn_decide, slot 11 (AoC 12): its rewrite - the same code (0x554E98) is AoC\'s slot 11, '
                 'the base\'s slot 10, which the base fills with this function too'),
    0x005DC902: (0x005ED231, 'call_slot_goal: the group dispatch\'s call of goal_move_at (AoC 9 calls of it there, the base 8)'),
    0x005D05CC: (0x005E0E7F, 'game_functions.resign_send: a function of its own in the 2017 list (ApplySpec made it), inside '
                 'another in AoC\'s (local/games/aoc/decomp/: the list from before its project split 2787 functions)'),
    0x006167C7: (0x006292C9,'EEUnit.lets_pass: slot 18 of EEUnit\'s vtable (AoC 0x847A20), unpaired by its code'),
}

def texts(e, f): return [t for _, _, t, _ in e.insns(f)]
def unique(e, key):
    d = collections.defaultdict(list)
    for f, _ in e.fns: d[key(e, f)].append(f)
    return {h: v[0] for h, v in d.items() if len(v) == 1}


def add(x, y, why, shared=False):
    """x is y; shared: y may already be another's (the AoC linker folded functions with the same code into one)"""
    if x in pair or (y in paired_b and not shared): return 0
    pair[x] = y; how[x] = why; paired_b.add(y)
    return 1


def fits(x, y):
    """y lies where x's neighbours went: above the median of the 3 nearest same-code functions below x, below those
    above - the compiler kept most of the code's order, a weak pair (one caller) against it is likely a mistake"""
    i = bisect.bisect_left(anchors, x)
    lo, hi = sorted(pair[a] for a in anchors[max(0, i - 3):i]), sorted(pair[a] for a in anchors[i:i + 3])
    return (not lo or lo[len(lo) // 2] < y) and (not hi or y < hi[len(hi) // 2])


def align(fa, fb):
    """the two functions' instructions aligned: index in fa -> index in fb"""
    ta, tb = texts(A, fa), texts(B, fb)
    if ta == tb: return {k: k for k in range(len(ta))}
    m = difflib.SequenceMatcher(None, ta, tb, autojunk=False)
    return {blk.a + k: blk.b + k for blk in m.get_matching_blocks() for k in range(blk.size)}


def load(path_a=EXE_A, fns_a=FN_A, path_b=EXE_B, fns_b=FN_B, hand=None, by_name=False):
    """pair the functions of two builds of one image (the exes, or an engine DLL): A, B, pair, how, data_votes"""
    global A, B, HAND, vt_b, pair, how, paired_b, anchors, data_votes
    HAND = EXE_HAND if hand is None else hand
    A, B = Exe(path_a, fns_a), Exe(path_b, fns_b)
    vt_b = {v[:2]: (k, v[2]) for k, v in B.vt.items()}
    # the pairs of the same inputs (the images, the lists, this script, the hand pairs) from local/work/: a minute saved
    import hashlib, pickle
    key = hashlib.sha1(repr([(p, os.path.getsize(p), os.path.getmtime(p)) for p in (path_a, fns_a, path_b, fns_b, __file__)]
                            + [sorted(HAND.items()), by_name]).encode()).hexdigest()[:16]
    cache = os.path.join(ROOT, 'local', 'work', 'port_pairs_%s.pkl' % key)
    if os.path.exists(cache):
        pair, how, paired_b, anchors, data_votes = pickle.load(open(cache, 'rb'))
        return
    _pair(by_name)
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    pickle.dump((pair, how, paired_b, anchors, data_votes), open(cache, 'wb'))


def _pair(by_name):
    global pair, how, paired_b, anchors, data_votes
    pair, how, paired_b = {}, {}, set()
    if by_name:   # the same name once in each (the DLLs' exports: Ghidra names them after the mangled symbol)
        na, nb = A.names(), B.names()
        for n, f in na.items():
            if n in nb: add(f, nb[n], 'name')
        print('the same name in both: %d' % len(pair), file=sys.stderr)
    # 1. functions with the same code (addresses masked) once in each
    ua, ub = unique(A, lambda e, f: hash(tuple(texts(e, f)))), unique(B, lambda e, f: hash(tuple(texts(e, f))))
    for h, f in ua.items():
        if h in ub: add(f, ub[h], 'code')
    print('functions: %d and %d, the same code once in both: %d' % (len(A.fns), len(B.fns), len(pair)), file=sys.stderr)
    anchors = sorted(pair)

    # 2. the same class's vtables pair their slots' functions (only where both have as many slots: an added virtual
    # function shifts the rest)
    for vt, (cls, off, n) in A.vt.items():
        if (cls, off) in vt_b and vt_b[(cls, off)][1] == n:
            vb = vt_b[(cls, off)][0]
            for k in range(n):
                x, y = A.u32(vt + 4 * k), B.u32(vb + 4 * k)
                if x in A.size and y in B.size and pair.get(x, y) == y: add(x, y, 'vtable %s slot %d' % (cls, k))
    print('with the vtables\' slots: %d' % len(pair), file=sys.stderr)

    # 3. follow the pairs: what a paired function calls and uses, aligned instruction by instruction; functions still
    # unpaired by their code with every number masked (struct offsets moved), and by their place between paired neighbours
    loose = lambda e, f: hash(tuple(HEX.sub('N', t) for t in texts(e, f)))
    data_votes = collections.defaultdict(collections.Counter)
    done = set()
    for rnd in range(60):
        call_votes = collections.defaultdict(collections.Counter)
        for fa, fb in list(pair.items()):
            if fa in done: continue
            done.add(fa)
            ia, ib = A.insns(fa), B.insns(fb)
            for ka, kb in align(fa, fb).items():
                va, vb = ia[ka][3], ib[kb][3]
                if len(va) != len(vb): continue
                for x, y in zip(va, vb):
                    if x in A.size and y in B.size: call_votes[x][y] += 1
                    else: data_votes[x][y] += 1
        new = 0
        for x, c in call_votes.items():   # most of its callers agree (one caller: and it fits the order)
            (y, n), = c.most_common(1)
            if n >= 0.8 * sum(c.values()) and (n >= 2 or fits(x, y)):
                new += add(x, y, 'called (%d of %d)' % (n, sum(c.values())), shared=n >= 3)
        if not new:
            la = unique(A, loose); lb = unique(B, loose)
            for h, f in la.items():
                if h in lb and f not in pair: new += add(f, lb[h], 'code but numbers')
        if not new:   # a class's vtable with a slot more or less: its slots' functions by the most alike near the same slot
            for vt, (cls, off, n) in A.vt.items():
                if (cls, off) not in vt_b or vt_b[(cls, off)][1] == n: continue
                vb, nb = vt_b[(cls, off)]
                for k in range(n):
                    x = A.u32(vt + 4 * k)
                    if x in pair or x not in A.size: continue
                    cand = [B.u32(vb + 4 * j) for j in (k - 1, k, k + 1) if 0 <= j < nb]
                    cand = [y for y in cand if y in B.size and y not in paired_b]
                    r = sorted(((difflib.SequenceMatcher(None, texts(A, x), texts(B, y), autojunk=False).ratio(), y) for y in set(cand)), reverse=True)
                    if r and r[0][0] >= 0.6 and (len(r) == 1 or r[1][0] < r[0][0] - 0.15):
                        new += add(x, r[0][1], 'vtable %s near slot %d' % (cls, k))
        if not new:   # between paired neighbours: the unpaired function there most alike
            for k in range(1, len(A.fns) - 1):
                f = A.fns[k][0]
                p, q = pair.get(A.fns[k - 1][0]), pair.get(A.fns[k + 1][0])
                if f in pair or p is None or q is None or not p < q: continue
                j, j2 = B.starts.index(p), B.starts.index(q)
                cand = [y for y, _ in B.fns[j + 1:j2] if y not in paired_b]
                if not cand or len(cand) > 8: continue
                r = sorted(((difflib.SequenceMatcher(None, texts(A, f), texts(B, y), autojunk=False).ratio(), y) for y in cand), reverse=True)
                if r[0][0] >= 0.5 and (len(r) == 1 or r[1][0] < r[0][0] - 0.15):
                    new += add(f, r[0][1], 'between paired neighbours')
        print('round %d: %d more functions paired' % (rnd + 1, new), file=sys.stderr)
        if not new: break
    for x, y in pair.items(): data_votes[x][y] += 1   # a function's address used as data (vtables, callbacks)


def vt_at(e, va):
    """the vtable whose slots va lies in"""
    for vt, (_, _, n) in e.vt.items():
        if vt <= va < vt + 4 * n: return vt
    return None


def callee(t): return HAND[t][0] if t in HAND else pair.get(t)


def masked(e, lo, hi):
    """the bytes lo..hi as a regex, the image addresses and call/jmp targets in them any byte"""
    b, out, k = e.rd(lo, hi - lo), [], 0
    while k < len(b):
        if k + 5 <= len(b) and b[k] in (0xE8, 0xE9):
            out.append(re.escape(b[k:k + 1]) + b'....'); k += 5; continue
        if k + 4 <= len(b) and e.in_image(struct.unpack('<I', b[k:k + 4])[0]):
            out.append(b'....'); k += 4; continue
        out.append(re.escape(b[k:k + 1])); k += 1
    return b''.join(out)


def pattern(x):
    """the bytes around x (addresses masked) once in the other exe's code: x's place there"""
    for w in (16, 24, 32, 48, 64):
        rx = re.compile(masked(A, x - w, x) + b'(?=' + masked(A, x, x + w) + b')', re.S)
        lo, hi = B.code[0] - B.ib, B.code[1] - B.ib
        hits = [m.end() for m in rx.finditer(B.img, lo, hi)]
        if len(hits) == 1: return B.ib + hits[0]
        if not hits: return 0
    return 0


def port(x):
    """x in the other exe: (address, how) or (0, why not)"""
    if x in A.imports:   # an import slot: the same dll and name
        want = A.imports[x]
        ys = [a for a, v in B.imports.items() if v == want]
        name = want[1].decode() if isinstance(want[1], bytes) else want[1]
        return (ys[0], 'import %s' % name) if ys else (0, 'import %s gone' % name)
    if x in HAND: return HAND[x][0], 'by hand: ' + HAND[x][1]
    if x in pair: return pair[x], 'function (%s)' % how[x]
    f = A.func_of(x) if A.in_code(x) else None
    if f is not None:    # inside a function: the aligned instruction, the same byte in it
        why = 'its function %06x unpaired' % f
        if f in pair:
            ia, ib = A.insns(f), B.insns(pair[f])
            k = next((k for k, i in enumerate(ia) if i[0] <= x < i[0] + i[1]), None)
            al = align(f, pair[f])
            t = ia[k][3][0] if k is not None and ia[k][2] == 'call X' else None
            # a call in a changed function goes by its order below (the alignment can pick another call of the same
            # callee there)
            same = texts(A, f) == texts(B, pair[f])
            if k is not None and k in al and (t is None or (same and callee(t) == ib[al[k]][3][0])):
                return ib[al[k]][0] + (x - ia[k][0]), 'in %06x+%x (%s)' % (f, x - f, how[f])
            why = 'its instruction in %06x changed' % f
            # a call: the same callee's calls in both, by their order
            t = t if callee(t) else None
            if t is not None:
                ca = [i for i in ia if i[2] == 'call X' and i[3][0] == t]
                cb = [i for i in ib if i[2] == 'call X' and i[3][0] == callee(t)]
                if len(ca) == len(cb):
                    n = ca.index(ia[k])
                    return cb[n][0] + (x - ia[k][0]), 'call %d of %d of %06x in %06x' % (n + 1, len(ca), t, f)
                why += ' (%d calls of %06x, %d)' % (len(ca), t, len(cb))
        y = pattern(x)
        return (y, 'PATTERN') if y else (0, why)
    if A.in_code(x):
        y = pattern(x)
        if y: return y, 'PATTERN'
    vt = vt_at(A, x)
    if vt is not None:   # in a vtable: the same class's, the same slot
        cls, off, n = A.vt[vt]
        if (cls, off) not in vt_b: return 0, 'class %s gone' % cls
        y = vt_b[(cls, off)][0] + (x - vt)
        if x == vt: return y, 'vtable %s' % cls
        ok = 'ok' if pair.get(A.u32(x)) == B.u32(y) else 'UNCHECKED'
        return y, 'vtable %s slot %d %s' % (cls, (x - vt) // 4, ok)
    c = data_votes.get(x)   # data: what the paired code uses at that place
    if c:
        (y, n), = c.most_common(1)
        return y, 'data (%d of %d uses)' % (n, sum(c.values())) if len(c) > 1 else 'data (%d uses)' % n
    # data nobody uses directly: the nearest used address below, the same distance
    lo = max((v for v in data_votes if x - 0x40 <= v < x and len(data_votes[v]) == 1), default=None)
    if lo is not None:
        return next(iter(data_votes[lo])) + (x - lo), 'NEAR %06x+%x' % (lo, x - lo)
    return 0, 'not found'


if __name__ == '__main__':   # (imported: load() pairs - port_fields.py, port_dll.py)
    load()
    # (an RVA in an engine DLL is no address in the exe: port_dll.py's)
    rows = [(label, v[0], v[1]) for label, _, v, _ in spec_read.places() if A.in_image(v[0])]
    def insn_at(e, va):
        """the instruction at va (masked like insns), and its start"""
        f = e.func_of(va)
        for i in e.insns(f) if f is not None else ():
            if i[0] <= va < i[0] + i[1]: return i
        i = next(e.cs.disasm(e.rd(va, 16), va), None)   # outside the function lists: as it decodes there
        return (i.address, i.size, i.mnemonic + ' ' + HEX.sub(lambda m: 'A' if e.in_image(int(m.group(0), 16)) else m.group(0), i.op_str), []) if i else None


    def check(x, y):
        """an independent look at a pair: the code alike, the instruction the same, its callers agree"""
        if not y or not A.in_code(x): return ''
        if x in A.size and y in B.size:
            r = difflib.SequenceMatcher(None, texts(A, x), texts(B, y), autojunk=False).ratio()
            callers = [(f, k) for f in pair for k, i in enumerate(A.insns(f)) if x in i[3]]
            agree = sum(1 for f, k in callers if (al := align(f, pair[f])).get(k) is not None
                        and y in B.insns(pair[f])[al[k]][3])
            return 'code %3d%%, callers %d/%d' % (r * 100, agree, len(callers))
        ia, ib = insn_at(A, x), insn_at(B, y)
        if not ia or not ib: return 'NO INSTRUCTION'
        same = ia[2].replace('L', '') == ib[2].replace('L', '') if ia[2].split()[0] == ib[2].split()[0] else False
        return ('insn same' if same and x - ia[0] == y - ib[0] else 'INSN DIFFERS: %s / %s' % (ia[2], ib[2]))


    found = differ = 0
    for name, x, have in rows:
        y, why = port(x)
        found += y != 0
        cmp = 'NEW' if have == spec_read.NONE else 'same' if have == y else 'DIFFERS: spec %08X' % have
        differ += cmp.startswith('DIFFERS')
        print('%-44s %08X  %08X  %-24s %-44s %s' % (name, x, y, cmp, why, check(x, y)))
    print('%d of %d found, %d differ from the spec' % (found, len(rows), differ), file=sys.stderr)
