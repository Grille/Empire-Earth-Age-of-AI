# The save / load code of the two exe builds compared in their machine code (from the EEScnEdit session, 2026-10-23): a
# function's stream calls - the stream's vtable slots T (w8 a byte written, wi an int, wf a float ... r8, ri ... read) -
# with its direct and import calls, the file-version tests (cmp [ebp+0xC], 0xE?/0xF?: VER) and small constants, in
# address order, side by side with its Art of Conquest pair. A diff of the decompiled text misses calls the decompiler
# folds away (EEComputerPlayer::vf9 has 7 more stream calls in its code than in the text).
# usage: python stream_diff.py <2017 addr> <AoC addr> [-k] [-d]   the two functions (hex); -k: the arguments' offsets
#                                                                   and the callees' full names, -d: "n same" for equal runs
#        python stream_diff.py --tree <2017 addr> [...]             each function and its callees (8 deep) that write the
#                                                                   stream: "same" or the writes that differ (port_addrs'
#                                                                   pairs), UNPAIRED without an AoC pair
# The slots assume the stream's vtable call form call dword ptr [reg + 0xNN] (true of every save / load function compared).
import difflib, os, re, sys
import capstone, pefile
sys.path.insert(0, os.path.dirname(__file__))
import port_addrs as P

EX = {'base': (P.EXE_A, P.FN_A), 'aoc': (P.EXE_B, P.FN_B)}
T = {'0x74': 'w8', '0x7c': 'wf', '0x90': 'wi', '0x94': 'wu', '0x8c': 'w16', '0x88': 'w16s', '0x78': 'w8s', '0x80': 'wd',
     '0x84': 'w84', '0x98': 'w98',
     '0x3c': 'r8', '0x44': 'rf', '0x58': 'ri', '0x5c': 'ru', '0x54': 'r16', '0x50': 'r16s', '0x40': 'r8s', '0x48': 'rd'}
cache = {}


def load(b):
    if b in cache: return cache[b]
    pe = pefile.PE(EX[b][0], fast_load=True)
    img, ib = pe.get_memory_mapped_image(), pe.OPTIONAL_HEADER.ImageBase
    fns = {}
    for l in open(EX[b][1], encoding='utf-8', errors='replace'):
        f = l.split('\t')
        if len(f) >= 3: fns[int(f[0], 16)] = (f[1], int(f[2]))
    cache[b] = (img, ib, fns)
    return cache[b]


def seq(b, a, keep):
    """the function's stream calls, calls, version tests and small constants: [(address, text)]"""
    img, ib, fns = load(b)
    name, size = fns[a]
    out, last = [], []
    for i in capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(img[a - ib:a - ib + size], a):
        ops = i.op_str
        if i.mnemonic in ('lea', 'push', 'mov'):
            last = (last + [ops])[-3:]
        m = re.match(r'dword ptr \[(e\w\w) \+ (0x[0-9a-f]+)\]$', ops)
        if i.mnemonic == 'call' and m and m.group(2) in T:
            arg = ''
            for l in reversed(last):
                mm = re.search(r'\[(e\w\w) \+ (0x[0-9a-f]+)\]', l)
                if mm and mm.group(2) not in ('0x8', '0xc', '0x10', '0x14', '0x18'): arg = mm.group(2); break
            out.append((i.address, T[m.group(2)] + ('(' + arg + ')' if keep else '')))
        elif i.mnemonic == 'call' and ops.startswith('0x'):
            callee = fns.get(int(ops, 16), ('?',))[0]
            out.append((i.address, 'call ' + (callee if keep else re.sub(r'_?[0-9a-f]{8}', '', callee))))
        elif i.mnemonic == 'call' and 'ptr [0x' in ops:
            out.append((i.address, 'call IAT'))
        elif i.mnemonic == 'call':
            out.append((i.address, 'call ' + (ops if keep else re.sub(r'0x[0-9a-f]+', 'N', ops))))
        elif i.mnemonic == 'cmp' and re.search(r', 0x(e[0-9a-f]|f[0-9a-f])$', ops) and 'ebp + 0xc' in ops:
            out.append((i.address, 'VER ' + ops.split(', ')[1]))
        elif i.mnemonic in ('cmp', 'push', 'mov') and re.search(r', (0x[0-9a-f]|0x[0-3][0-9a-f]|[1-9])$', ops) and 'esp' not in ops:
            out.append((i.address, 'C ' + ops.split(', ')[1]))
    return out


def side_by_side(a_addr, b_addr, keep, short):
    a, b = seq('base', a_addr, keep), seq('aoc', b_addr, keep)
    sm = difflib.SequenceMatcher(None, [x for _, x in a], [x for _, x in b], autojunk=False)
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == 'equal' and short: print('   ... %d same' % (i2 - i1)); continue
        for k in range(max(i2 - i1, j2 - j1)):
            x = a[i1 + k] if i1 + k < i2 else (0, '')
            y = b[j1 + k] if j1 + k < j2 else (0, '')
            print('%s %08x %-40s | %08x %s' % (' ' if op == 'equal' else '*', x[0], x[1], y[0], y[1]))


def tree(roots):
    P.load()
    seen = set()

    def walk(a, d):
        if a in seen or d > 8 or a not in P.A.size: return
        seen.add(a)
        sa = seq('base', a, False)
        if not any(x[1][0] in 'wr' for x in sa) and not any(x[1].startswith('call') for x in sa): return
        b = P.pair.get(a)
        hasw = any(x[1][:1] == 'w' for x in sa)
        if hasw:
            if b is None: print('%s%08x UNPAIRED %s' % ('  ' * d, a, P.A.name.get(a)))
            else:
                sb = seq('aoc', b, False)
                ta, tb = [x[1] for x in sa if x[1][:1] in 'wC'], [x[1] for x in sb if x[1][:1] in 'wC']
                dd = [l for l in difflib.unified_diff(ta, tb, lineterm='', n=0) if l[:1] in '+-' and l[:3] not in ('+++', '---')]
                print('%s%08x -> %08x %s %s' % ('  ' * d, a, b, P.A.name.get(a), 'same' if not dd else 'DIFF: ' + ' '.join(dd)[:300]))
        for _, _, t, vals in P.A.insns(a):
            if t.startswith('call X') and vals and (hasw or d == 0): walk(vals[0], d + 1)
    for x in roots: walk(int(x, 16), 0)


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('-') or a == '--tree']
    if args and args[0] == '--tree': tree(args[1:])
    elif len(args) == 2: side_by_side(int(args[0], 16), int(args[1], 16), '-k' in sys.argv, '-d' in sys.argv)
    else: print(__doc__ or 'usage: stream_diff.py <2017 addr> <AoC addr> [-k] [-d] | --tree <2017 addr> ...')
