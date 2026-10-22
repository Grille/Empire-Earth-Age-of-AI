"""The address rows into the file their use calls for: what game/api/functions.h calls into functions_addr.h, what
game/api/hooks.h patches into hooks_addr.h, the game's data (G_, read by functions.h's accessors) into data_addr.h.
A function both called and hooked is a function; a row neither uses is reported (unused: take it out). Within a file
the rows stay in the order of their address (the numbers and lists in their order). One table either way - addr.h
composes the three - so a row's move changes no code (scripts/code_same.py compares the table by name).

usage: python scripts/addr_sort.py           move the rows where they belong, print the moves
       python scripts/addr_sort.py --check   only print them (exit 1 when a row is in the wrong file)"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
API = os.path.join(ROOT, 'dll', 'game', 'api')
FILES = {'FUNCTIONS': 'functions_addr.h', 'HOOKS': 'hooks_addr.h', 'DATA': 'data_addr.h'}
BLOCKS = ('ADDRS', 'ADDR_NUMBERS', 'ADDR_LISTS')


def read(f):
    s = open(os.path.join(API, f), 'rb').read().decode('latin-1')
    return s.replace('\r\n', '\n'), '\r\n' in s


def strip_comments(t):
    return re.sub(r'//[^\n]*', ' ', re.sub(r'/\*.*?\*/', ' ', t, flags=re.S))


def block_span(s, name):
    """the lines of #define NAME(...) \\ up to its first line without a backslash: (start, end, header, lines)"""
    m = re.search(r'^#define ' + name + r'\(([^)]*)\) *\\\n', s, re.M)
    if not m: return None
    pos, lines = m.end(), []
    while True:
        e = s.index('\n', pos)
        l = s[pos:e]
        pos = e + 1
        if not l.strip(): break   # the empty line after the last row's backslash ends it
        lines.append(l)
        if not l.rstrip().endswith('\\'): break
    return m.start(), pos, m.group(0), lines


def rows(lines):
    """[(name, [lines])]: a row with the comment lines before it; a list row with its continuation lines"""
    out, pend = [], []
    for l in lines:
        s = l.strip().rstrip('\\').strip()
        if not s: continue
        m = re.match(r'\s*[FDL]\((\w+)', l)
        if m: out.append((m.group(1), pend + [l])); pend = []
        elif s.startswith('/*'): pend.append(l)   # a comment line: the next row's
        else: out[-1][1].append(l)                # a list's continuation
    return out


def first_value(r):
    v = re.findall(r'0x[0-9A-Fa-f]+', strip_comments(' '.join(r[1])))
    return int(v[0], 16) if v else 0


def main():
    check = '--check' in sys.argv
    texts = {k: read(f) for k, f in FILES.items()}
    fun = strip_comments(open(os.path.join(API, 'functions.h'), encoding='latin-1').read())
    hk = strip_comments(open(os.path.join(API, 'hooks.h'), encoding='latin-1').read())
    def used(t, n): return re.search(r'\b' + n + r'\b', t) is not None
    def where(n):
        if n.startswith('G_'): return 'DATA'
        a, h = used(fun, n), used(hk, n)
        if a and (not h or n.startswith(('FN_', 'SIZE_'))): return 'FUNCTIONS'
        return 'HOOKS' if h else None   # unused
    moves, unused, new_rows = [], [], {}
    for blk in BLOCKS:
        allr = []
        for k in FILES:
            sp = block_span(texts[k][0], '%s_%s' % (blk, k))
            for n, ls in rows(sp[3]) if sp else []:
                allr.append((n, ls, k))
        for n, ls, k in allr:
            if where(n) is None: unused.append('%s (%s): used by neither functions.h nor hooks.h' % (n, FILES[k]))
            elif where(n) != k: moves.append('%s: %s -> %s' % (n, FILES[k], FILES[where(n)]))
            if where(n) not in (None, k) and not block_span(texts[where(n)][0], '%s_%s' % (blk, where(n))):
                sys.exit('addr_sort: %s belongs into %s, which has no %s_%s' % (n, FILES[where(n)], blk, where(n)))
        for k in FILES:
            mine = [(n, ls) for n, ls, c in allr if (where(n) or c) == k]
            if blk == 'ADDRS': mine.sort(key=first_value)
            new_rows[blk, k] = mine
    for m in moves + unused: print(('addr_sort: ' if check else '') + m)
    if check:
        if moves: print('addr_sort: rows in the wrong file - python scripts/addr_sort.py moves them')
        sys.exit(1 if moves or unused else 0)
    if not moves: return
    for k, f in FILES.items():
        s, crlf = texts[k]
        for blk in BLOCKS:
            sp = block_span(s, '%s_%s' % (blk, k))
            if not sp: continue
            body = []
            for _, ls in new_rows[blk, k]:
                for l in ls:
                    l = l.rstrip()
                    body.append(l if l.endswith('\\') else l + ' \\')
            s = s[:sp[0]] + sp[2] + ''.join(l + '\n' for l in body) + '\n' + s[sp[1]:]
        open(os.path.join(API, f), 'wb').write((s.replace('\n', '\r\n') if crlf else s).encode('latin-1'))
    print('%d rows moved' % len(moves))


if __name__ == '__main__':
    main()
