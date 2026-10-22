"""The address rows into the file their use calls for: what game/api/'s files call into raw/functions.h, what they
patch (their HOOKS, hooks.c, patching.c) into raw/hooks.h, the game's data (G_, read by their accessors) into raw/data.h.
A function both called and hooked is a function; a row neither uses is reported (unused: take it out). Within a file
the rows stay in the order of their address (the numbers and lists in their order). One table either way - raw/tables.c
composes the three - so a row's move changes no code (scripts/code_same.py compares the table by name).
And the field rows of raw/fields.h: a kind's name at the start of each, one row per field, each used (field_problems).

usage: python scripts/addr_sort.py           move the rows where they belong, print the moves
       python scripts/addr_sort.py --check   only print them (exit 1 when a row is in the wrong file)"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
API = os.path.join(ROOT, 'dll', 'game', 'api')
FILES = {'FUNCTIONS': 'raw/functions.h', 'HOOKS': 'raw/hooks.h', 'DATA': 'raw/data.h'}
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


def field_problems(code):
    """raw/fields.h's rows: each name a kind of its header's list and something (PL_TECH_TREE), one row per kind and
    2017 offset (the first column: a second name for a field would let the two drift apart), each row used"""
    s = read('raw/fields.h')[0]
    head = s[:s.index('#define FIELDS')]
    kinds = set(re.findall(r'^//   ([A-Z][A-Z0-9_]*)\s', head, re.M)) | set(re.findall(r'\b(E_[A-Z0-9]+)\b', head))
    out, seen = [], {}
    for n, ee in re.findall(r'^\s*O\((\w+), (0x[0-9A-F]+|NA),', s, re.M):
        k = max((k for k in kinds if n.startswith(k + '_') and len(n) > len(k) + 1), key=len, default=None)
        if not k: out.append('%s (raw/fields.h): no kind of the list at its start' % n); continue
        if (k, ee) in seen: out.append('%s (raw/fields.h): the same field as %s (%s +%s)' % (n, seen[k, ee], k, ee))
        seen[k, ee] = n
        if not re.search(r'\b' + n + r'\b', code): out.append('%s (raw/fields.h): used by none of game/api/' % n)
    return out


def api_code():
    """game/api/'s code (not raw/, api.c, exe.c): (what it calls and reads, what it hooks) - an object's file has its
    hooks after its "// HOOKS" title, hooks.c and patching.c are hooks throughout"""
    fun, hk = '', ''
    for f in sorted(os.listdir(API)):
        if not f.endswith('.c') or f in ('api.c', 'exe.c'): continue
        t = open(os.path.join(API, f), encoding='latin-1').read()
        if f in ('hooks.c', 'patching.c'): hk += t; continue
        i = t.find('\n// HOOKS\n')
        fun += t if i < 0 else t[:i]
        hk += '' if i < 0 else t[i:]
    return strip_comments(fun), strip_comments(hk)


def main():
    check = '--check' in sys.argv
    texts = {k: read(f) for k, f in FILES.items()}
    fun, hk = api_code()
    fields = field_problems(fun + hk)
    for m in fields: print('addr_sort: ' + m)
    if fields: sys.exit(1)
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
            if where(n) is None: unused.append('%s (%s): used by none of game/api/' % (n, FILES[k]))
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
