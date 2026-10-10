# Find value changes between two builds of the same code (EE vs AoC): functions whose instruction sequences
# are identical except for operands are paired; then every differing immediate (push/mov imm32) and every
# differing float/int constant read from memory is listed. Used for the 1024x768 editor layout backport.
# usage: python layout_diff.py <base lo hex> <base hi hex> [out.tsv]
import sys, os, struct, hashlib
import pefile, capstone
from capstone import x86

HERE = os.path.dirname(os.path.abspath(__file__))
EXES = {'base': os.path.join(HERE, '..', '..', 'Empire Earth.exe'),
        'aoc': os.path.join(HERE, '..', 'aoc', 'work', 'EE-AOC.exe')}
TSV = {'base': os.path.join(HERE, '..', 'decomp', 'functions.tsv'),
       'aoc': os.path.join(HERE, '..', 'aoc', 'decomp', 'functions.tsv')}

class Exe:
    def __init__(self, path, tsv):
        self.pe = pefile.PE(path, fast_load=True)
        self.ib = self.pe.OPTIONAL_HEADER.ImageBase
        self.img = self.pe.get_memory_mapped_image()
        self.funcs = []
        for l in open(tsv, encoding='utf-8', errors='replace'):
            f = l.rstrip('\n').split('\t')
            if len(f) >= 3:
                self.funcs.append((int(f[0], 16), f[1], int(f[2])))
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
    def rd(self, va, n):
        o = va - self.ib
        return self.img[o:o + n] if 0 <= o < len(self.img) - n else None
    def dis(self, va, size):
        code = self.rd(va, size)
        return list(self.md.disasm(code, va)) if code else []

def is_addr(v):
    return 0x401000 <= v < 0xA00000

def norm(ins):
    """mnemonic + operand shapes, immediates/addresses blanked"""
    parts = [ins.mnemonic]
    for op in ins.operands:
        if op.type == x86.X86_OP_REG:
            parts.append(ins.reg_name(op.reg))
        elif op.type == x86.X86_OP_IMM:
            parts.append('I')
        elif op.type == x86.X86_OP_MEM:
            m = op.mem
            parts.append('[%s+%s*%d+%s]' % (ins.reg_name(m.base) if m.base else '', ins.reg_name(m.index) if m.index else '',
                                           m.scale, 'D' if (m.disp and is_addr(m.disp & 0xFFFFFFFF)) or m.disp else '0'))
    return ' '.join(parts)

def sig(insns):
    return hashlib.md5('\n'.join(norm(i) for i in insns).encode()).hexdigest()

def plausible(v):
    """float or int that could be a screen coordinate / size"""
    f = struct.unpack('<f', struct.pack('<I', v))[0]
    if 0.5 <= abs(f) <= 4096 and abs(f * 100 - round(f * 100)) < 0.02 and v > 0x3F000000:
        return 'f', f
    if 1 <= v <= 4096:
        return 'i', v
    return None, None

def compare(base, aoc, va, name, pairs, rows):
    """pairs: list of (base insn, aoc insn) known to correspond"""
    if True:
        for x, y in pairs:
            for ox, oy in zip(x.operands, y.operands):
                if ox.type == x86.X86_OP_IMM and oy.type == x86.X86_OP_IMM:
                    vx, vy = ox.imm & 0xFFFFFFFF, oy.imm & 0xFFFFFFFF
                    if vx != vy and not (is_addr(vx) and is_addr(vy)):
                        kx, fx = plausible(vx); ky, fy = plausible(vy)
                        if kx and ky:
                            # operand address: the imm32 is the last 4 bytes of the instruction for push/mov imm32
                            rows.append(('imm', va, name, x.address, x.address + x.size - 4, kx, fx, fy, x.mnemonic + ' ' + x.op_str))
                elif ox.type == x86.X86_OP_MEM and oy.type == x86.X86_OP_MEM and ox.mem.base == 0 and ox.mem.index == 0:
                    ax, ay = ox.mem.disp & 0xFFFFFFFF, oy.mem.disp & 0xFFFFFFFF
                    if is_addr(ax) and is_addr(ay) and ax >= 0x823000 and ay >= 0x837000:   # constants in .rdata
                        bx, by = base.rd(ax, 4), aoc.rd(ay, 4)
                        if bx and by and bx != by and ox.size in (4,):
                            vx, vy = struct.unpack('<I', bx)[0], struct.unpack('<I', by)[0]
                            kx, fx = plausible(vx); ky, fy = plausible(vy)
                            if kx and ky and kx == ky and kx == 'f':
                                # absolute memory operand without immediate: the disp32 is the last 4 bytes
                                rows.append(('const', va, name, x.address, x.address + x.size - 4, kx, fx, fy, x.mnemonic + ' ' + x.op_str))

def fuzzy_pairs(bi, ai):
    """align two instruction lists on their normalized form; pairs from the equal blocks"""
    import difflib
    sm = difflib.SequenceMatcher(None, [norm(i) for i in bi], [norm(i) for i in ai], autojunk=False)
    out = []
    for a0, b0, n in sm.get_matching_blocks():
        out.extend(zip(bi[a0:a0 + n], ai[b0:b0 + n]))
    return out, sm.ratio()

def main():
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    out = open(sys.argv[3], 'w') if len(sys.argv) > 3 else None
    base, aoc = Exe(EXES['base'], TSV['base']), Exe(EXES['aoc'], TSV['aoc'])
    aoc_sigs = {}
    for va, name, size in aoc.funcs:
        if size < 24 or size > 60000:
            continue
        s = sig(aoc.dis(va, size))
        aoc_sigs.setdefault(s, []).append(va)
    aoc_by_va = {va: (name, size) for va, name, size in aoc.funcs}
    aoc_sorted = sorted(aoc_by_va)
    matched, rows, anchors, pending = 0, [], [], []
    for va, name, size in base.funcs:
        if not (lo <= va < hi) or size < 24:
            continue
        bi = base.dis(va, size)
        cands = aoc_sigs.get(sig(bi), [])
        if len(cands) != 1:
            pending.append((va, name, size, bi))
            continue
        matched += 1
        anchors.append((va, cands[0]))
        compare(base, aoc, va, name, list(zip(bi, aoc.dis(cands[0], size))), rows)
    # functions whose structure changed: best fuzzy match among the AoC functions between the paired neighbours
    import bisect
    anchors.sort()
    fuzzy = 0
    for va, name, size, bi in pending:
        k = bisect.bisect(anchors, (va, 0))
        lo_a = anchors[k - 1][1] if k > 0 else aoc_sorted[0]
        hi_a = anchors[k][1] if k < len(anchors) else aoc_sorted[-1]
        if hi_a - lo_a > 0x8000:
            continue
        best = None
        for ava in aoc_sorted[bisect.bisect(aoc_sorted, lo_a):bisect.bisect(aoc_sorted, hi_a)]:
            asize = aoc_by_va[ava][1]
            if not (0.7 * size <= asize <= 1.4 * size):
                continue
            pairs, ratio = fuzzy_pairs(bi, aoc.dis(ava, asize))
            if ratio > 0.8 and (not best or ratio > best[0]):
                best = (ratio, ava, pairs)
        if best:
            fuzzy += 1
            compare(base, aoc, va, name, best[2], rows)
    print('functions in range: %d matched exactly, %d of %d others matched fuzzily' % (matched, fuzzy, len(pending)))
    for r in rows:
        line = '%s\t%06x\t%s\t%06x\t%06x\t%s\t%g\t%g\t%s' % r
        print(line)
        if out: out.write(line + '\n')
    print(len(rows), 'value differences')

main()
