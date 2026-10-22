"""A game object's fields read by name: FIELD(p, OF(KIND, off), T) outside dll/game/api/ becomes objects.c's accessor
(kind_get_x(p)), FIELD(...) = v its setter (kind_set_x(p, v)). The names come from objects.c itself - an accessor is
written there first (internal inline T kind_get_x(const ee_kind *p) { return FIELD(p, OF(KIND, off), T); }), then this
moves the code over. A use of another type than the accessor's keeps it with a cast ((unsigned)unit_get_x(u)): the
machine code stays FIELD's (scripts/code_same.py). A use that is neither a read nor a plain write (&FIELD(...), +=,
++), or a read as float of an int field (or the other way: FIELD reinterprets the bits, a cast would convert them),
stops it, with the place - those are for a hand.

    python scripts/field_names.py KIND [KIND ...]     rewrite the uses of these kinds' fields
    python scripts/field_names.py --left [KIND]       the FIELD(.., OF(KIND, ..)) uses outside game/api/ left, by field
"""
import collections, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DLL = os.path.join(ROOT, 'dll')
OBJECTS = os.path.join(DLL, 'game', 'api', 'objects.c')
LAYOUT = os.path.join(DLL, 'game', 'api', 'layout.h')
# a getter; an indexed one reads OF(K, x) + i * stride (stride 1: + i)
GETTER = re.compile(r'internal inline (.+?)\b(\w+_get_\w+)\(const \w+ \*\w+(, int \w+)?\) \{ return FIELD\(\w+, OF\((\w+), (0x[0-9A-F]+)\)'
                    r'(?: \+ \w+(?: \* (\w+))?)?, (.+?)\); \}')
SETTER = re.compile(r'internal inline void (\w+_set_\w+)\(\w+ \*\w+(, int \w+)?, (.+?) \w+\) \{ FIELD\(\w+, OF\((\w+), (0x[0-9A-F]+)\)'
                    r'(?: \+ \w+(?: \* (\w+))?)?, (.+?)\) = \w+; \}')
# a field's address (an array or a structure inside the object): FIELD_AT, cast to its type
AT = re.compile(r'internal inline (.+?)\b(\w+_at)\(const \w+ \*\w+(, int \w+)?\) \{ return (?:\(.+?\))?FIELD_AT\(\w+, '
                r'OF\((\w+), (0x[0-9A-F]+)\)(?: \+ \w+(?: \* (\w+))?)?\); \}')


def calls(s, name):
    """each NAME(...) with balanced parentheses: (start, end, the text between them)"""
    for m in re.finditer(r'\b%s\s*\(' % name, s):
        i, d = m.end(), 1
        while d and i < len(s):
            d += s[i] == '('
            d -= s[i] == ')'
            i += 1
        yield m.start(), i, s[m.end():i - 1]


def split_args(a):
    out, d, cur = [], 0, ''
    for c in a:
        d += c in '([{'
        d -= c in ')]}'
        if c == ',' and d == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += c
    out.append(cur.strip())
    return out


def norm(t):
    return re.sub(r'\s*\*', ' *', ' '.join(t.split())).replace('* *', '**')


def accessors():
    """(KIND, offset) -> (getter, its type, its index's stride or None) as objects.c defines them"""
    out = {}
    text = open(OBJECTS, encoding='latin-1').read()
    for m in GETTER.finditer(text):
        stride = int(m.group(6), 0) if m.group(6) else 1 if m.group(3) else None
        out[(m.group(4), m.group(5))] = (m.group(2), norm(m.group(7)), stride)
    for m in SETTER.finditer(text):   # a field only written: its would-be getter's name (a write uses _set_)
        key = (m.group(4), m.group(5))
        if key not in out:
            stride = int(m.group(6), 0) if m.group(6) else 1 if m.group(2) else None
            out[key] = (m.group(1).replace('_set_', '_get_'), norm(m.group(7)), stride)
    return out


def addresses():
    """(KIND, offset) -> (the _at accessor, its result type, its index's stride or None): a field's address"""
    out = {}
    for m in AT.finditer(open(OBJECTS, encoding='latin-1').read()):
        stride = int(m.group(6), 0) if m.group(6) else 1 if m.group(3) else None
        out[(m.group(4), m.group(5))] = (m.group(2), norm(m.group(1)), stride)
    return out


def aliases():
    """layout.h's names for a field: NAME -> 'OF(KIND, offset)'"""
    text = open(LAYOUT, encoding='latin-1').read()
    return {m.group(1): m.group(2) for m in re.finditer(r'#define (\w+)\s+(OF\(\w+, 0x[0-9A-F]+\))', text)}


NUMBER = re.compile(r'0x[0-9a-fA-F]+|\d+')


def unparen(e):
    e = e.strip()
    return e[1:-1] if e.startswith('(') and e.endswith(')') and e[1:-1].count('(') == e[1:-1].count(')') else e


def sources():
    for p in sorted(glob.glob(os.path.join(DLL, '**', '*.[ch]'), recursive=True)):
        f = os.path.relpath(p, DLL).replace(os.sep, '/')
        if not f.startswith(('game/api/', 'dinput/')):
            yield f, p


def field_use(ar, kinds, table, alias):
    """((KIND, offset), the index or None) of a FIELD's arguments when it is one of kinds' with an accessor, else None"""
    if len(ar) != 3:
        return None
    off = ar[1]
    m = re.match(r'(\w+)\b(.*)$', off, re.S)
    if m and m.group(1) in alias:   # layout.h's name for it (PL_INDEX)
        off = alias[m.group(1)] + m.group(2)
    m = re.match(r'OF\((\w+), (0x[0-9A-F]+)\)\s*(?:\+\s*(.+))?$', off, re.S)
    if not m or m.group(1) not in kinds or (m.group(1), m.group(2)) not in table:
        return None
    key, rest, stride = (m.group(1), m.group(2)), (m.group(3) or '').strip(), table[(m.group(1), m.group(2))][2]
    if stride is None:
        return (key, None) if not rest else None
    if not rest:
        return key, '0'
    if stride == 1:
        return key, unparen(rest)
    t = re.match(r'(.+?)\s*\*\s*(\w+)$', rest, re.S)
    if t and NUMBER.fullmatch(t.group(2)) and int(t.group(2), 0) == stride and t.group(1).count('(') == t.group(1).count(')'):
        return key, unparen(t.group(1))
    if NUMBER.fullmatch(rest) and int(rest, 0) % stride == 0:
        return key, str(int(rest, 0) // stride)
    return None


def rewrite(kinds):
    table, at, alias, changed = accessors(), addresses(), aliases(), {}
    for f, p in sources():
        raw = open(p, 'rb').read().decode('latin-1')
        s = raw
        while True:   # one use a pass, the last found: a FIELD inside another one's object or index goes first
            best = None
            for a, b, args in calls(s, 'FIELD'):
                ar = split_args(args)
                use = field_use(ar, kinds, table, alias)
                if use and not any(field_use(split_args(x), kinds, table, alias) for _, _, x in calls(ar[0] + ' ' + ar[1], 'FIELD')):
                    best = (a, b, ar, table[use[0]], use[1])
            if not best:
                break
            a, b, (obj, _, t), (name, typ, _), idx = best
            args = obj if idx is None else '%s, %s' % (obj, idx)
            rest, pre = s[b:], s[:a].rstrip()
            w = re.match(r'\s*=(?!=)\s*', rest)
            if w:   # FIELD(...) = v;  ->  kind_set_x(obj, v);
                e = rest.index(';')
                val = rest[w.end():e].strip()
                if norm(t) != typ and ('float' in (norm(t), typ) or 'double' in (norm(t), typ)):
                    sys.exit('%s:%d: written as %s, the setter takes %s - by hand' % (f, s[:a].count('\n') + 1, t, typ))
                if norm(t) != typ:
                    val = '(%s)(%s)' % (typ, val)
                s = s[:a] + '%s(%s, %s)' % (name.replace('_get_', '_set_'), args, val) + rest[e:]
                continue
            if re.match(r'\s*([-+*/|&^]=|<<=|>>=|\+\+|--)', rest) or (pre.endswith('&') and not pre.endswith('&&')) or pre.endswith(('++', '--')):
                sys.exit('%s:%d: neither a read nor a plain write - by hand: %s' % (f, s[:a].count('\n') + 1, s[a:b + 4]))
            if norm(t) != typ and ('float' in (norm(t), typ) or 'double' in (norm(t), typ)):   # FIELD reinterprets, a cast converts
                sys.exit('%s:%d: read as %s, the accessor gives %s - by hand: %s' % (f, s[:a].count('\n') + 1, t, typ, s[a:b]))
            call = '%s(%s)' % (name, args)
            s = s[:a] + (call if norm(t) == typ else '((%s)%s)' % (t, call)) + s[b:]
        while True:   # FIELD_AT(obj, OF(K, x) [+ i * stride]) -> kind_x_at(obj[, i]); its cast to the result type goes
            best = None
            for a, b, args in calls(s, 'FIELD_AT'):
                ar = split_args(args)
                use = len(ar) == 2 and field_use(ar + ['-'], kinds, at, alias)
                if use:
                    best = (a, b, ar[0], at[use[0]], use[1])
            if not best:
                break
            a, b, obj, (name, typ, _), idx = best
            call = '%s(%s)' % (name, obj if idx is None else '%s, %s' % (obj, idx))
            cast = re.search(r'\((?:const )?(\w+ \*)\)\s*$', s[:a])
            if cast and norm(cast.group(1)) == typ:
                s = s[:cast.start()] + call + s[b:]
            elif typ == 'uint8_t *':
                s = s[:a] + call + s[b:]
            else:   # FIELD_AT gave a uint8_t *
                s = s[:a] + '((uint8_t *)%s)' % call + s[b:]
        if s != raw:
            changed[f] = s
    for f, s in changed.items():
        open(os.path.join(DLL, f), 'wb').write(s.encode('latin-1'))
    print('field_names: %d files' % len(changed))
    return 0


def left(kind):
    table, n = accessors(), collections.Counter()
    for f, p in sources():
        s = open(p, 'rb').read().decode('latin-1')
        for m in re.finditer(r'\bOF\((\w+), (0x[0-9A-F]+)\)', s):
            if kind in (None, m.group(1)):
                n[(m.group(1), m.group(2), 'accessor' if (m.group(1), m.group(2)) in table else 'none', f)] += 1
    for (k, o, a, f), c in sorted(n.items()):
        print('%-10s %-8s %-8s %4d  %s' % (k, o, a, c, f))
    return 0


if __name__ == '__main__':
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    if args[0] == '--left':
        sys.exit(left(args[1] if len(args) > 1 else None))
    sys.exit(rewrite(set(args)))
