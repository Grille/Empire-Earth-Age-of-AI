"""Which part of the DLL may use which: the folders of dll/ and their rules, checked (run by dll/build.sh).

The DLL is one translation unit (eeaimod.c includes every file), so the compiler lets any file use any other's
names; this check holds the line. A file uses another when it names one of its top-level definitions (a function,
a variable, a type, a struct tag, an enum constant, a macro). Allowed: its own folder, and the folders its folder
lists below. A use that crosses another line must be in EXCEPTIONS (each with why it is still there) - the build
fails on any other, and on an exception that isn't needed any more.

    python scripts/layers.py            the check (exit 1 on a violation; --quiet: only the problems, dll/build.sh)
    python scripts/layers.py --map      who uses what, by folder
    python scripts/layers.py --why X    every use of folder (or file) X by the others
"""
import collections, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DLL = os.path.join(ROOT, 'dll')

# THE RULES: what each folder may use besides itself. eeaimod.c and persist.c (dll/ itself: the wiring, the saves)
# and test/ may use everything; nothing may use test/ or the wiring.
MAY_USE = {
    'base':   [],                                          # the DLL's own helpers: no game, no part
    'game':   ['base'],                                    # what the mod knows of the game
    'engine': ['base', 'game'],                            # fixes and features of the game
    'ai':     ['base', 'game', 'voice/say.h'],             # the computer players - they speak through say.h only
    'script': ['base'],                                    # the language: knows no game (script/host.h asks it)
    'game_script': ['base', 'game', 'ai', 'script', 'voice'],   # the game in the language: constants, values, relations
    'voice':  ['base', 'game', 'ai', 'script'],            # what the AI says: reads the AI, speaks the language
    'test':   ['base', 'game', 'engine', 'ai', 'script', 'game_script', 'voice', 'dll'],
    'dll':    ['base', 'game', 'engine', 'ai', 'script', 'game_script', 'voice', 'test'],
}

# A FILE OF A FOLDER as the rule names it (voice/say.h): only what that file declares - its prototypes, types and
# variables - not the rest of its folder

# THE EXCEPTIONS still in the code: (using file, used file, names, why). Each is a debt; the next steps remove them.
EXCEPTIONS = [
    # single crossings, to look at
    ('ai/diplo.c', 'engine/scenario_ai.c', 'g_is_scenario', 'a scenario: no diplomacy of the mod'),
    ('ai/attack.c', 'test/testcmd.c', 'charge_tick', 'a test command ticked from the AI update'),
    ('ai/diplo.c', 'test/ecolog.c', 'eco_death', 'the economy log counts deaths (autotest)'),
    ('ai/diplo.c', 'test/randmap.c', 'g_rm', 'the test game\'s settings'),
    ('ai/econ.c', 'test/trace.c', 'trace_train', 'call tracing (autotest)'),
    ('ai/sites.c', 'test/trace.c', 'CALLER Fn2 Fn3 trace_emit trace_obj tracing', 'call tracing (autotest)'),
    ('engine/aim.c', 'ai/attack.c', 'seen_by', 'leading only targets the player sees'),
    ('engine/display.c', 'test/control.c', 'g_turbo', 'turbo test runs draw nothing'),
    ('engine/display_diag.c', 'test/control.c', 'g_req', 'the frame watch reports a pending pipe command'),
    ('engine/display_diag.c', 'voice/llm.c', 'llm_busy', 'the frame watch reports the language model'),
    ('voice/llm.c', 'test/control.c', 'queue_pause_toggle', 'the game paused while the language model answers'),
]

# THE FILES MOVED OVER to the typed game functions (game/api.h): GF there fails the build. When every file outside
# game/ is in it, the rule becomes "GF only in game/".
TYPED = ['ai/econ.c']

IDENT = re.compile(r'[A-Za-z_]\w*')
KEYWORDS = set('''if for while switch return sizeof static const struct enum union typedef unsigned signed int char void
float double long short volatile extern inline do else case default break continue goto __attribute__ __asm__ asm
__thiscall __cdecl __stdcall __fastcall __declspec'''.split())


def strip(src):
    """comments and string/char literals blanked (same length, newlines kept)"""
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith('//', i):
            j = src.find('\n', i); j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2); j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', src[i:j])); i = j
        elif c in '"\'':
            j = i + 1
            while j < n and src[j] != c and src[j] != '\n':
                j += 2 if src[j] == '\\' else 1
            j += 1
            out.append(c + ' ' * max(0, j - i - 2) + c); i = j
        else:
            out.append(c); i += 1
    return ''.join(out)


def top_level(code):
    """the code with every brace body blanked (the braces stay), and the preprocessor lines apart"""
    out, depth = [], 0
    for ch in code:
        if ch == '}': depth -= 1
        out.append(ch if depth == 0 or ch == '\n' else ' ')
        if ch == '{': depth += 1
    return ''.join(out)


def definitions(code):
    """the names a file defines at top level: functions, variables, typedefs, 'struct x' tags, enum constants, macros"""
    names = set()
    lines = code.split('\n')
    for ln in lines:
        m = re.match(r'\s*#\s*define\s+(\w+)', ln)
        if m: names.add(m.group(1))
    body, pre = [], False   # the preprocessor lines (with their continuations) apart
    for ln in lines:
        pre = pre or bool(re.match(r'\s*#', ln))
        body.append('' if pre else ln)
        pre = pre and ln.rstrip().endswith('\\')
    body = '\n'.join(body)
    # enum constants of enums at top level
    t = top_level(body)
    depth = 0
    for m in re.finditer(r'\benum\b\s*\w*\s*\{', body):
        if body[:m.start()].count('{') - body[:m.start()].count('}') != 0: continue
        end = body.index('}', m.end())
        for item in body[m.end():end].split(','):
            n = re.match(r'\s*(\w+)', item)
            if n: names.add(n.group(1))
    for m in re.finditer(r'\b(struct|union|enum)\s+(\w+)\s*\{', t):
        names.add('%s %s' % (m.group(1), m.group(2)))
    # statements at top level: up to ; or a function body's }
    for st in re.split(r';|\}(?=\s*(?:\n|$))', t):
        s = ' '.join(st.split())
        if not s: continue
        s = re.sub(r'__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)', ' ', s)
        m = re.match(r'.*?\b(\w+)\s*\(([^{}]*)\)\s*\{\s*$', s)            # a function definition: name(...) {
        if m and m.group(1) not in KEYWORDS:
            names.add(m.group(1)); continue
        if s.startswith('typedef'):
            m = re.search(r'\(\s*(?:__\w+\s*)?\*\s*(\w+)\s*\)\s*\(', s) or re.search(r'(\w+)\s*(?:\[[^\]]*\]\s*)*$', s)
            if m: names.add(m.group(1))
            continue
        head = s.split('=')[0]
        fp = re.match(r'[^(]*\(\s*(?:__\w+\s*)?\*\s*(\w+)\s*\)\s*[(\[]', head)   # R (CC *name)(...): a function pointer
        if '(' in head and not fp: continue                              # a prototype
        if fp: names.add(fp.group(1)); continue
        decl = re.sub(r'\{[^{}]*\}', '{}', s)                            # struct bodies (blanked) and initializers
        parts, d, cur = [], 0, ''
        for ch in decl:
            if ch in '({[': d += 1
            elif ch in ')}]': d -= 1
            if ch == ',' and d == 0: parts.append(cur); cur = ''
            else: cur += ch
        parts.append(cur)
        for p in parts:
            p = p.split('=')[0]
            p = re.sub(r'\[[^\]]*\]', '', p).strip()
            m = re.search(r'(\w+)\s*$', p)
            if m and m.group(1) not in KEYWORDS and not re.match(r'(struct|union|enum)\s+\w+\s*(\{\s*\})?$', p):
                names.add(m.group(1))
    return names


def uses(code):
    """the names a file mentions: identifiers (not after . or ->), 'struct x' tags"""
    out = set()
    for m in re.finditer(r'\b(struct|union|enum)\s+(\w+)', code):
        out.add('%s %s' % (m.group(1), m.group(2)))
    for m in IDENT.finditer(code):
        before = code[max(0, m.start() - 2):m.start()]
        if before.endswith('.') or before.endswith('->'): continue
        if code[max(0, m.start() - 7):m.start()].rstrip().endswith(('struct', 'union', 'enum')): continue
        out.add(m.group(0))
    if re.search(r'\bG[AFL]\(', code): out |= {'GA', 'GF', 'GL'}
    return out


def folder(f):
    return f.split('/')[0] if '/' in f else 'dll'


def analyse():
    files = sorted(os.path.relpath(p, DLL).replace(os.sep, '/') for p in
                   glob.glob(os.path.join(DLL, '**', '*.c'), recursive=True) + glob.glob(os.path.join(DLL, '**', '*.h'), recursive=True))
    files = [f for f in files if not f.startswith('dinput/')]   # another DLL (its own rule below)
    code = {f: strip(open(os.path.join(DLL, f), 'rb').read().decode('latin-1')) for f in files}
    owner = collections.defaultdict(set)
    for f in files:
        for n in definitions(code[f]): owner[n].add(f)
    # the X-macro names of addr.h and unit_ids.h: A_x, x_fn, UT_x are made there
    edges = collections.defaultdict(lambda: collections.defaultdict(set))   # user -> used file -> names
    for f in files:
        for n in uses(code[f]):
            o = owner.get(n)
            if not o or f in o: continue
            if len(o) > 1:   # declared in several (a forward declaration): the one that defines it - most mentions
                o = {max(o, key=lambda x: len(re.findall(r'\b%s\b' % re.escape(n.split()[-1]), code[x])))}
                if f in o: continue
            edges[f][next(iter(o))].add(n)
    return files, code, edges


def declared(code):
    """what a header declares: its definitions and its prototypes"""
    names = definitions(code)
    for m in re.finditer(r'^static\s[^;{=]*?\b(\w+)\s*\([^;{]*\)\s*;', code, re.M):
        names.add(m.group(1))
    return names


def excepted(u, d, n):
    for eu, ed, names, why in EXCEPTIONS:
        if (eu == u or eu.endswith('/*') and u.startswith(eu[:-1])) and (ed == d or ed.endswith('/*') and d.startswith(ed[:-1])) \
                and (names == '*' or n in names.split()):
            return (eu, ed, names)
    return None


def main():
    files, code, edges = analyse()
    if '--map' in sys.argv:
        order = ['dll', 'base', 'game', 'engine', 'ai', 'script', 'game_script', 'voice', 'test']
        m = collections.defaultdict(collections.Counter)
        for u, ds in edges.items():
            for d, ns in ds.items():
                if folder(u) != folder(d): m[folder(u)][folder(d)] += len(ns)
        print('%-8s' % 'uses ->' + ''.join('%8s' % g for g in order))
        for g in order: print('%-8s' % g + ''.join('%8s' % (m[g][h] or '.') for h in order))
        return 0
    if '--why' in sys.argv:
        x = sys.argv[sys.argv.index('--why') + 1]
        for u in sorted(edges):
            for d, ns in sorted(edges[u].items()):
                if (d == x or folder(d) == x) and folder(u) != folder(d): print('%s -> %s: %s' % (u, d, ', '.join(sorted(ns))))
        return 0
    iface = {f: declared(code[f]) for f in {x for v in MAY_USE.values() for x in v if '/' in x}}
    bad, used = [], set()
    for u in sorted(edges):
        for d, ns in sorted(edges[u].items()):
            fu, fd = folder(u), folder(d)
            if fu == fd or fd in MAY_USE[fu]: continue
            ns = {n for n in ns if not any(f in MAY_USE[fu] and n in iface[f] for f in iface)}   # a door it may use
            if not ns: continue
            for n in sorted(ns):
                e = excepted(u, d, n)
                if e: used.add(e)
                else: bad.append('%s uses %s of %s (%s may use %s)' % (u, n, d, fu, ', '.join(MAY_USE[fu]) or 'nothing but itself'))
    # dinput.dll: its own file and base/headless_win.c
    for f in glob.glob(os.path.join(DLL, 'dinput', '*.c')):
        for inc in re.findall(r'#include\s+"([^"]+)"', open(f, encoding='latin-1').read()):
            if inc != '../base/headless_win.c': bad.append('dinput/%s includes %s' % (os.path.basename(f), inc))
    # the files moved over to game/api.h: the game's functions only through its typed wrappers
    for f in TYPED:
        for m in re.finditer(r'\bGF\s*\(\s*(\w+)', code[f]):
            bad.append('%s calls %s through GF - moved over to game/api.h: add a wrapper there' % (f, m.group(1)))
    stale = [e for e in EXCEPTIONS if (e[0], e[1], e[2]) not in used]
    for b in bad: print('layers: ' + b)
    for e in stale: print('layers: exception no longer needed: %s -> %s (%s) - take it out of scripts/layers.py' % e[:3])
    if not bad and not stale and '--quiet' not in sys.argv:
        print('layers: %d files, %d exceptions' % (len(files), len(EXCEPTIONS)))
    return 1 if bad or stale else 0


if __name__ == '__main__':
    sys.exit(main())
