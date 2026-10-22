# Layout hashes of the blocks dll/persist.c saves into .aimod files -> dll/persist_layout.h (run by dll/build.sh).
# A block is one global variable (persist.c k_blocks); its layout is the text of its definition plus the definitions
# of the structs it names (recursively), comments and spacing left out, hashed (FNV-1a). A struct changed to the same
# size (fields reordered, int <-> float) changes the hash: a save's block of the old layout is then not loaded.
# usage: python scripts/persist_layout.py [--check | --show]
#   --check: exit 1 if dll/persist_layout.h is not up to date; --show: print what each hash is made of
import glob, os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
dll = os.path.join(here, '..', 'dll')
BS = chr(92)

def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', ' ', s, flags=re.S)
    return re.sub(r'//[^\n]*', ' ', s)

def statements(s):
    # file-scope statements: to the ';' at brace depth 0, or to a '}' back at depth 0 with nothing after it on its
    # line (a function body); preprocessor lines left out
    out, i, n = [], 0, len(s)
    while i < n:
        while i < n and s[i] in ' \t\r\n':
            i += 1
        if i >= n:
            break
        if s[i] == '#':
            j = s.find('\n', i)
            while j > 0 and s[j - 1] == BS:
                j = s.find('\n', j + 1)
            i = n if j < 0 else j + 1
            continue
        a, depth, j = i, 0, i
        while j < n:
            c = s[j]
            if c == '"' or c == "'":
                k = j + 1
                while k < n and s[k] != c:
                    k += 2 if s[k] == BS else 1
                j = k + 1
                continue
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    e = s.find('\n', j)
                    if e < 0 or not s[j + 1:e].strip():
                        j += 1
                        break
            elif c == ';' and depth == 0:
                j += 1
                break
            j += 1
        out.append(' '.join(s[a:j].split()))
        i = j
    return out

stmts = []
src = {}
for fn in sorted(glob.glob(os.path.join(dll, '**', '*.c'), recursive=True)):
    src[os.path.basename(fn)] = strip_comments(open(fn, 'rb').read().decode('latin-1'))
    stmts += statements(src[os.path.basename(fn)])

def outside_braces(t):
    o, depth = [], 0
    for c in t:
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
        elif depth == 0:
            o.append(c)
    return ''.join(o)

def definition(var):
    # a variable definition: starts with internal, no function (no '(' before its first '{' / its name), names var
    pat = re.compile(r'\b%s\b' % re.escape(var))
    found = [t for t in stmts if t.startswith('internal ') and t.endswith(';') and pat.search(outside_braces(t))
             and '(' not in outside_braces(t).split(var)[0]]
    if len(found) != 1:
        raise SystemExit('persist_layout: %d definitions of %s' % (len(found), var))
    return found[0]

def struct_def(name):
    pat = re.compile(r'^(internal )?struct %s \{' % re.escape(name))
    found = [t for t in stmts if pat.match(t)]
    return found[0] if found else ''

alltext = '\n'.join(src[k] for k in sorted(src))
def macro(name):   # the value of #define NAME (array sizes: two swapped sizes keep the total)
    m = re.search(r'^\s*#define\s+%s\s+([^\n]*)' % re.escape(name), alltext, flags=re.M)
    return ' '.join(m.group(1).split()) if m else None

def layout(var):
    seen, todo, parts = set(), [definition(var)], []
    while todo:
        t = todo.pop(0)
        parts.append(t)
        for n in re.findall(r'\bstruct (\w+)', t):
            if n not in seen:
                seen.add(n)
                b = struct_def(n)
                if b and b not in parts and b not in todo:
                    todo.append(b)
    names, todo = set(), re.findall(r'\b[A-Z][A-Z0-9_]+\b', '\n'.join(parts))
    while todo:   # with the macros their values name
        m = todo.pop()
        if m in names or macro(m) is None:
            continue
        names.add(m)
        todo += re.findall(r'\b[A-Z][A-Z0-9_]+\b', macro(m))
    parts += ['%s=%s' % (m, macro(m)) for m in sorted(names)]
    # the game's objects typed (game/api/types.h: ee_unit * ...) are the same pointers the blocks held as uint8_t * before -
    # the same bytes, the same hash: a save of before the typing still loads them; likewise internal (base/lang.h) is
    # the static the definitions were written with before
    text = re.sub(r'^internal ', 'static ', '\n'.join(parts), flags=re.M)
    return re.sub(r'\bee_\w+ \*', 'uint8_t *', text)

def fnv(s):
    h = 0x811C9DC5
    for b in s.encode('latin-1'):
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h

per = src['persist.c']
blk = per[per.index('k_blocks[] = {'):]
blk = '\n'.join(l for l in blk[:blk.index('};')].split('\n') if not l.lstrip().startswith('#'))
tags = re.findall(r'\bB\((\w+)\)', blk)
if '--show' in sys.argv:
    for t in tags:
        print('== %s %08X' % (t, fnv(layout(t))))
        print(layout(t))
    sys.exit(0)
lines = ['// generated by scripts/persist_layout.py (dll/build.sh): the layout hash of each block dll/persist.c saves',
         'internal const struct { const char *tag; uint32_t hash; } k_layout[] = {']
for t in tags:
    lines.append('    { "%s", 0x%08XU },' % (t, fnv(layout(t))))
lines.append('};')
out = '\n'.join(lines) + '\n'
path = os.path.join(dll, 'persist_layout.h')
old = open(path, 'rb').read().decode('latin-1') if os.path.exists(path) else ''
if '--check' in sys.argv:
    sys.exit(0 if old == out else 1)
if old != out:
    open(path, 'wb').write(out.encode('latin-1'))
    print('persist_layout.h: %d blocks' % len(tags))
