"""Is the DLL's machine code the same, function by function? For moves that change no code (files into folders,
functions from one file to another): the order of the functions, the constants and the variables in the DLL changes
with them, so the DLLs differ byte for byte, but every function's instructions must stay the same.

usage: python scripts/code_same.py <old: a dll/ folder or a git revision> [<new: a dll/ folder, default dll/>]
                                  [--same old_symbol=new_symbol ...]   (a variable that only got another name)
Compiles eeaimod.c of both into objects (each function and variable in its own section, otherwise as dll/build.sh
does) and compares each function's instructions without addresses: a reference to a variable by its name, to a
constant by the bytes it points at. A revision is checked out into work/code_same/tree (a git worktree)."""
import os, re, struct, subprocess, sys, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T = os.path.join(ROOT, 'tools', 'llvm-mingw', 'bin')
CC = os.path.join(T, 'i686-w64-mingw32-clang')
OBJDUMP = os.path.join(T, 'llvm-objdump')
WORK = os.path.join(ROOT, 'work', 'code_same')


def compile_dir(d, out):
    flags = ['-O2', '-mstackrealign', '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections', '-c', '-I', d,
             '-Wno-builtin-macro-redefined', '-D__DATE__="-"', '-D__TIME__="-"']   # the build time: the same
    subprocess.run([CC] + flags + ['-o', out, os.path.join(d, 'eeaimod.c')], check=True, cwd=d)


def coff(path):
    """sections [(name, raw bytes, {offset: symbol index})], symbols {index: (name, section number, value, class)}"""
    b = open(path, 'rb').read()
    nsec, symptr, nsym = struct.unpack_from('<H', b, 2)[0], struct.unpack_from('<I', b, 8)[0], struct.unpack_from('<I', b, 12)[0]
    opt = struct.unpack_from('<H', b, 16)[0]
    strtab = symptr + nsym * 18
    def name8(raw):
        if raw[:1] == b'/': o = int(raw[1:].rstrip(b'\0')); return b[strtab + o:b.index(b'\0', strtab + o)].decode()
        return raw.rstrip(b'\0').decode()
    secs = []
    for i in range(nsec):
        h = 20 + opt + i * 40
        nm = name8(b[h:h + 8])
        size, ptr, rptr, nrel = struct.unpack_from('<I', b, h + 16)[0], struct.unpack_from('<I', b, h + 20)[0], struct.unpack_from('<I', b, h + 24)[0], struct.unpack_from('<H', b, h + 32)[0]
        rel = {}
        for k in range(nrel):
            off, si, typ = struct.unpack_from('<IIH', b, rptr + k * 10)
            rel[off] = si
        secs.append((nm, b[ptr:ptr + size] if ptr else bytes(size), rel))
    syms, i = {}, 0
    while i < nsym:
        e = symptr + i * 18
        raw = b[e:e + 8]
        nm = b[strtab + struct.unpack_from('<I', b, e + 4)[0]:b.index(b'\0', strtab + struct.unpack_from('<I', b, e + 4)[0])].decode() if raw[:4] == b'\0\0\0\0' else raw.rstrip(b'\0').decode()
        value, secno, typ, cls, naux = struct.unpack_from('<IhHBB', b, e + 8)
        syms[i] = (nm, secno, value, cls)
        i += 1 + naux
    return secs, syms


def addr_names(d):
    """the address table's names in their order (g_addr's index): the rows can move between the address files
    (game/api/addr.h, functions_addr.h, hooks_addr.h) - the same name is the same address"""
    h = next((p for p in ('game/api/addr.h', 'game/addr.h') if os.path.exists(os.path.join(d, p))), None)
    if not h: return None
    src = '#include "%s"\nCODE_SAME_NAMES ADDRS(ADDR_NAME_F, ADDR_NAME_D) ADDR_NUMBERS(ADDR_NAME_D)\n' % h
    out = subprocess.run([CC, '-E', '-P', '-I', d, '-x', 'c', '-'], input=src, capture_output=True, text=True, cwd=d).stdout
    tail = out[out.rfind('CODE_SAME_NAMES'):]
    return re.findall(r'"(\w+)"', tail) or None


def functions(obj, names=None):
    secs, syms = coff(obj)
    def target(si, addend):   # what a reference points at, independent of the layout
        nm, secno, value, cls = syms[si]
        if names and nm == '_g_addr' and addend % 4 == 0 and 0 <= addend // 4 < len(names):
            return 'g_addr[%s]' % names[addend // 4]                     # an address by its name, wherever its row is
        if names and nm in ('_k_addr', '_k_addr_name'): return nm        # the tables themselves: in the rows' order
        if secno <= 0: return nm                                          # outside: an import, another symbol
        sname, data, _ = secs[secno - 1]
        if sname.startswith('.rdata'):   # a constant (a string _.str.27 - numbered in the order they come): its bytes
            s = data[value + addend:value + addend + 64]
            z = s.find(0)
            return 'const%r' % (bytes(s[:z]) if z > 0 else bytes(s[:16]))
        if not nm.startswith('.'): return nm                              # a named function or variable
        return '%s+%d' % (sname, value + addend)                          # a section of one variable (-fdata-sections)
    dis = subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', obj], capture_output=True, text=True).stdout
    fns, cur, lines = collections.OrderedDict(), None, []
    secno_of = {}
    for n, (nm, _, _) in enumerate(secs): secno_of.setdefault(nm, []).append(n)
    used = collections.Counter()
    for ln in dis.splitlines() + ['Disassembly of section END:']:
        m = re.match(r'Disassembly of section (\S+):', ln)
        if m:
            if cur:
                k = secno_of[cur][used[cur]]; used[cur] += 1
                name, data, rel = secs[k]
                offs = [o for o, _ in lines] + [len(data)]
                out = []
                for j, (o, ins) in enumerate(lines):
                    for r in sorted(x for x in rel if o <= x < offs[j + 1]):
                        addend = struct.unpack_from('<i', data, r)[0]
                        t = target(rel[r], addend)
                        nm = syms[rel[r]][0]
                        # the instruction shows the field's value (the addend, or a pc-relative distance): masked
                        ins = re.sub(r'(?<![\w+])-?0x[0-9a-f]+(?=[(,\s]|$)', 'X', ins, count=0) if nm.startswith('.') or t.startswith('g_addr[') else ins
                        ins += ' -> ' + t
                    out.append(ins)
                fns[cur + ('#%d' % used[cur] if used[cur] > 1 else '')] = out
            cur, lines = m.group(1), []
            continue
        m = re.match(r'\s+([0-9a-f]+):\s+(.*)', ln)
        if cur and m and not re.match(r'IMAGE_REL', m.group(2)):
            lines.append((int(m.group(1), 16), re.sub(r'\s+', ' ', m.group(2)).strip()))
    return fns


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--same')]
    same = dict(a.split('=', 1)[1].split('=') for a in sys.argv[1:] if a.startswith('--same='))
    old, new = args[0], os.path.abspath(args[1] if len(args) > 1 else os.path.join(ROOT, 'dll'))
    if os.path.isdir(old): old = os.path.abspath(old)
    os.makedirs(WORK, exist_ok=True)
    if not os.path.isdir(old):   # a revision
        tree = os.path.join(WORK, 'tree')
        subprocess.run(['git', 'worktree', 'remove', '--force', tree], cwd=ROOT, capture_output=True)
        subprocess.run(['git', 'worktree', 'add', '--detach', tree, old], cwd=ROOT, check=True, capture_output=True)
        old = os.path.join(tree, 'dll')
    compile_dir(old, os.path.join(WORK, 'old.o'))
    compile_dir(new, os.path.join(WORK, 'new.o'))
    na, nb = addr_names(old), addr_names(new)
    if not na or not nb: na = nb = None   # an old tree without the names: by index, as before
    a, b = functions(os.path.join(WORK, 'old.o'), na), functions(os.path.join(WORK, 'new.o'), nb)
    back = {v: k for k, v in same.items()}
    for k in b:   # a new name as the old one: both count as the same variable
        b[k] = [re.sub(r'-> (\S+)$', lambda m: '-> ' + back.get(m.group(1), m.group(1)), ln) for ln in b[k]]
    bad = 0
    for k in sorted(set(a) | set(b)):
        if k not in a or k not in b:
            print('only in %s: %s' % ('old' if k in a else 'new', k)); bad += 1
        elif a[k] != b[k]:
            i = next((i for i in range(min(len(a[k]), len(b[k]))) if a[k][i] != b[k][i]), min(len(a[k]), len(b[k])))
            print('differs: %s at instruction %d:\n   old %s\n   new %s' % (k, i, a[k][i] if i < len(a[k]) else '-', b[k][i] if i < len(b[k]) else '-'))
            bad += 1
    print('%d functions, %d differ' % (len(a), bad))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
