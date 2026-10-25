"""The spec made C (run by dll/build.sh): dll/game/api/gen/<file>.c for each spec file (but UNMOVED), and
gen/gen.c, which includes them and selects the running build's values (spec_select, spec_log). Plain C - no tables:
a variable per value, a typedef per function, an accessor or a wrapper each, one assignment per value and build.

For a class's struct (its C type: [c_type], else its name in C's spelling; P that type without ee_, S the class's own
spelling - ee_complex_unit):
  a field          of_P_x (its offset), P_get_x(o), P_set_x(o, v), P_x_at(o) its address - an array's by index; a
                   runtime library layout (deque, vector) only P_x_at(o)
  a function       typedef P_x_fn, fn_P_x, P_x(o, ...) - thiscall, the object the hidden first argument
  a [slot]         slot_P_x, P_x(o, ...) through the object's vtable; with [address] or [overrides] the class's own
                   function too: fn_S_x, S_x(o, ...) called directly (the original of a hook in the slot)
  a [call_site]    call_n (several of one name: calls_n[]), patch_n(hook) - the hook typed P_x_fn
  a [vtable_site]  patch_n(hook): the class's vtable slot (vt_S) while it holds fn_S_x
  [vtable], [size] vt_S, size_S
A static class's functions: P its name, their convention [cdecl] / [stdcall]. A class types.h hasn't got is declared
by the generated file. A value the build hasn't got (none) is 0 for an address, NA for an offset or a slot - spec_log
names them. The spec's types: environment.cs's sizes as <stdint.h>'s (int32 int, as the code writes it), the game's
classes by their C type."""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spec_read   # noqa: E402

ROOT = spec_read.ROOT
OUT = os.path.join(ROOT, 'dll', 'game', 'api', 'gen')
TYPES_H = os.path.join(ROOT, 'dll', 'game', 'api', 'types.h')
UNMOVED = ()   # spec files whose rows are still raw/'s (their members not made yet)
SIZES = {'int8': 'int8_t', 'uint8': 'uint8_t', 'int16': 'int16_t', 'uint16': 'uint16_t', 'int32': 'int',
         'uint32': 'uint32_t', 'int64': 'int64_t', 'uint64': 'uint64_t', 'float32': 'float', 'float64': 'double',
         'nint': 'intptr_t', 'nuint': 'uintptr_t', 'char8': 'char', 'void': 'void'}
LAYOUTS = ('deque', 'vector', 'block')   # environment.cs's: only their address
CONV = {'cdecl': '__cdecl', 'stdcall': '__stdcall', 'thiscall': '__thiscall'}   # a static class's function's


class Fail(Exception):
    pass


def snake(name):   # EEComplexUnit -> ee_complex_unit, RTSUnit -> rts_unit
    s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
    return re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s).lower()


def decl(c, name):   # 'ee_player *', 'f' -> 'ee_player *f'
    return c + name if c.endswith('*') else c + ' ' + name


def ptr(c):   # the pointer type to c
    return c + '*' if c.endswith('*') else c + ' *'


class Gen:
    def __init__(self, files, known):
        self.env = {t.name: t for t in spec_read.read_environment()}
        self.types = dict(self.env, **{t.name: t for ts in files.values() for t in ts})
        self.known, self.new_types = known, []
        self.lines, self.assign, self.typedefs = [], [], {}

    def c_type_of(self, t):   # a class's C type; an enum's or a value's (vec2) is C's own, a class without one void
        a = t.attr('c_type')
        c = a[1][0] if a else snake(t.name)
        if c != 'void' and not t.enum and t.name not in self.env and c not in self.known and c not in self.new_types:
            self.new_types.append(c)
        return c

    def prefix(self, t):
        a = t.attr('c_prefix')
        if a:
            return a[1][0]
        if t.static:
            return t.name
        c = self.c_type_of(t)   # a class without a C type: its own name in C's spelling
        return re.sub(r'^ee_', '', snake(t.name) if c == 'void' else c)

    def name(self, P, x):   # a C name of the prefix P: P_x, or x alone under c_prefix("")
        return '%s_%s' % (P, x) if P else x

    def ctype(self, cs, where):   # 'EEUnit**' -> 'ee_unit **'
        base = cs.rstrip('*')
        stars = '*' * (len(cs) - len(base))
        if base in SIZES:
            c = SIZES[base]
        elif base in self.types and not self.types[base].static:
            c = self.c_type_of(self.types[base])
        else:
            raise Fail('%s: the type %s - not a size of environment.cs nor a class of the spec' % (where, cs))
        return c + (' ' + stars if stars else '')

    def var(self, cs, name, where):   # 'ee_unit **target'
        return decl(self.ctype(cs, where), name)

    def doc(self, m):
        for d in m.doc:
            self.lines.append('// ' + d if d else '//')

    def value(self, name, attr, kind, label, cast=None):   # kind addr (none: 0), off / num (none: NA); cast: a function's type
        self.assign.append((name, spec_read.values(attr), kind, label, cast))

    def gen_type(self, t):
        P, S = self.prefix(t), snake(t.name)
        obj = None if t.static else self.c_type_of(t)
        self.lines.append('')
        self.lines.append('// ' + ('static class ' + t.name if t.static else 'class %s (%s)' % (t.name, obj)))
        for d in t.doc:
            self.lines.append('// ' + d)
        for name, kind, label in (('vtable', 'addr', 'vt_'), ('size', 'off', 'size_')):
            a = t.attr(name)
            if a:
                self.lines.append('internal uint32_t %s%s;' % (label, S))
                self.value(label + S, a, kind, '%s [%s]' % (t.name, name))
        for m in t.members:
            if m.func:
                self.gen_func(t, m, P, S, obj)
            else:
                self.gen_field(t, m, P, obj)

    def gen_static(self, t, m, P):   # a static class's data, a place in the code, a number
        label = '%s.%s' % (t.name, m.name)
        self.doc(m)
        if m.attr('value'):   # a number: its variable only
            v = 'value_' + self.name(P, m.name)
            self.lines.append('internal uint32_t %s;' % v)
            self.value(v, m.attr('value'), 'num', label)
            return
        addrs = m.all('address')
        if not addrs:
            raise Fail('%s: a static class\'s member without [address] or [value]' % m.where)
        v = 'data_' + self.name(P, m.name)
        if len(addrs) > 1:   # a list of places, in its order
            self.lines.append('internal uint32_t %s[%d];' % (v, len(addrs)))
            for k, a in enumerate(addrs):
                self.value('%s[%d]' % (v, k), a, 'addr', '%s %d' % (label, k))
            return
        self.lines.append('internal uint32_t %s;' % v)
        self.value(v, addrs[0], 'addr', label)
        at = '(uint8_t *)(uintptr_t)%s' % v
        if m.type in LAYOUTS + ('code',):
            self.lines.append('internal inline uint8_t *%s_at(void) { return %s; }' % (self.name(P, m.name), at))
            return
        c = self.ctype(m.type, m.where)
        p = ptr(c)
        self.lines += ['internal inline %s(void) { return *(%s)%s; }' % (decl(c, self.name(P, 'get_' + m.name)), p, at),
                       'internal inline void %s(%s) { *(%s)%s = v; }' % (self.name(P, 'set_' + m.name), self.var(m.type, 'v', m.where), p, at),
                       'internal inline %s(void) { return (%s)%s; }' % (decl(p, self.name(P, m.name) + '_at'), p, at)]

    def gen_field(self, t, m, P, obj):
        if t.static:
            return self.gen_static(t, m, P)
        of = 'of_' + self.name(P, m.name)
        self.doc(m)
        self.lines.append('internal uint32_t %s;' % of)
        self.value(of, m.attr('offset'), 'off', '%s.%s' % (t.name, m.name))
        at = '(uint8_t *)o + %s' % of
        if m.type in LAYOUTS:
            self.lines.append('internal inline uint8_t *%s_at(const %s *o) { return %s; }' % (self.name(P, m.name), obj, at))
            return
        cls = self.types.get(m.type)
        if cls and not cls.static and not cls.enum and m.type not in self.env:   # an object inside the object: its address
            c = self.c_type_of(cls)
            self.lines.append('internal inline %s(const %s *o) { return (%s)(%s); }' % (decl(ptr(c), self.name(P, m.name) + '_at'), obj, ptr(c), at))
            return
        c = self.ctype(m.type, m.where)
        p, v = ptr(c), self.var(m.type, 'v', m.where)
        get, set_, at_ = decl(c, self.name(P, 'get_' + m.name)), self.name(P, 'set_' + m.name), decl(p, self.name(P, m.name) + '_at')
        if m.fixed:
            self.lines += ['internal inline %s(const %s *o, int i) { return ((%s)(%s))[i]; }' % (get, obj, p, at),
                           'internal inline void %s(%s *o, int i, %s) { ((%s)(%s))[i] = v; }' % (set_, obj, v, p, at),
                           'internal inline %s(const %s *o) { return (%s)(%s); }' % (at_, obj, p, at)]
        else:
            self.lines += ['internal inline %s(const %s *o) { return *(%s)(%s); }' % (get, obj, p, at),
                           'internal inline void %s(%s *o, %s) { *(%s)(%s) = v; }' % (set_, obj, v, p, at),
                           'internal inline %s(const %s *o) { return (%s)(%s); }' % (at_, obj, p, at)]

    def fn_type(self, t, m, P, obj):   # the typedef P_x_fn (once: a class's own function in a base's slot shares it)
        name = self.name(P, m.name) + '_fn'
        ret = self.ctype(m.type, m.where)
        params = ([obj + ' *'] if not t.static else []) + [self.ctype(c, m.where) for c, _ in m.params]
        conv = '__thiscall' if not t.static else next((CONV[a[0]] for a in m.attrs if a[0] in CONV), None)
        if not conv:
            raise Fail('%s: a static class\'s function without its convention ([cdecl] / [stdcall])' % m.where)
        text = 'typedef %s(%s *%s)(%s);' % (ret if ret.endswith('*') else ret + ' ', conv, name, ', '.join(params) or 'void')
        if name in self.typedefs and self.typedefs[name] != text:
            raise Fail('%s: %s differs from its base\'s' % (m.where, name))
        if name not in self.typedefs:
            self.typedefs[name] = text
            self.lines.append(text)
        return name

    def wrapper(self, t, m, fname, obj, call, this):
        ret = self.ctype(m.type, m.where)
        params = ([obj + ' *' + this] if not t.static else []) + [self.var(c, n, m.where) for c, n in m.params]
        args = ([this] if not t.static else []) + [n for _, n in m.params]
        body = '%s%s(%s);' % ('' if ret == 'void' else 'return ', call, ', '.join(args))
        self.lines.append('internal inline %s(%s) { %s }' % (decl(ret, fname), ', '.join(params) or 'void', body))

    def gen_func(self, t, m, P, S, obj):
        if t.static != m.static:
            raise Fail('%s: a static member belongs in a static class' % m.where)
        if m.attr('import'):
            raise Fail('%s: an [import] - not made yet (gen_api.py)' % m.where)
        this = P[0] if P and P[0] not in [n for _, n in m.params] else 'o'
        self.doc(m)
        fn_t = self.fn_type(t, m, P, obj)
        label = '%s.%s' % (t.name, m.name)
        slot, addr, over = m.attr('slot'), m.attr('address'), m.attr('overrides')
        if slot:   # the virtual function: called through the object's vtable
            sv = 'slot_' + self.name(P, m.name)
            self.lines.append('internal uint32_t %s;' % sv)
            self.value(sv, slot, 'num', label + ' [slot]')
            self.wrapper(t, m, self.name(P, m.name), obj, '((%s)(*(void ***)%s)[%s])' % (fn_t, this, sv), this)
        if over and not self.base_slot(m, over[1][0][1]):
            raise Fail('%s: [overrides] - %s has no [slot] %s' % (m.where, over[1][0][1], m.name))
        fv = None
        if addr:   # a class's own function in a slot is called directly by its class's name
            own = slot or over
            fv, direct = ('fn_%s_%s' % (S, m.name), '%s_%s' % (S, m.name)) if own else ('fn_' + self.name(P, m.name), self.name(P, m.name))
            self.lines.append('internal %s %s;' % (fn_t, fv))
            self.value(fv, addr, 'addr', label, fn_t)
            self.wrapper(t, m, direct, obj, fv, this)
        sites = {}
        for a in m.all('call_site'):
            sites.setdefault(a[1][0], []).append(a)
        for n, list_ in sites.items():
            if not fv:
                raise Fail('%s: a [call_site] of a function without its [address]' % m.where)
            if len(list_) == 1:
                self.lines.append('internal uint32_t call_%s;' % n)
                self.value('call_' + n, list_[0], 'addr', '%s [call_site %s]' % (label, n))
                self.lines.append('internal inline int patch_%s(%s h) { return patch_call(call_%s, (uintptr_t)%s, (void *)h); }'
                                  % (n, fn_t, n, fv))
            else:
                self.lines.append('internal uint32_t calls_%s[%d];' % (n, len(list_)))
                for k, a in enumerate(list_):
                    self.value('calls_%s[%d]' % (n, k), a, 'addr', '%s [call_site %s %d]' % (label, n, k))
                self.lines.append('internal inline int patch_%s(%s h, int *of) { return patch_calls(calls_%s, %d, (uintptr_t)%s, '
                                  '(void *)h, of); }' % (n, fn_t, n, len(list_), fv))
        site = m.attr('vtable_site')
        if site:
            if not (over and fv and t.attr('vtable')):
                raise Fail('%s: a [vtable_site] needs [overrides], [address] and its class\'s [vtable]' % m.where)
            self.lines.append('internal inline int patch_%s(%s h) { return patch_vtable(vt_%s, (int)slot_%s, (uintptr_t)%s, '
                              '(void *)h); }' % (site[1][0], fn_t, S, self.name(P, m.name), fv))

    def base_slot(self, m, base):   # a [slot] of the same name in base or its bases
        b = self.types.get(base)
        while b:
            if any(x.name == m.name and x.attr('slot') for x in b.members):
                return True
            a = b.attr('base')
            b = self.types.get(a[1][0][1]) if a else None
        return False


def fmt(v, kind, cast):
    if v == spec_read.NONE:
        s = '0' if kind == 'addr' else 'NA'
    else:
        s = '0x%08X' % v if kind == 'addr' else '0x%X' % v if kind == 'off' else str(v)
    return '(%s)%s' % (cast, s) if cast else s


def write(path, text):   # only when it changed: the build's other steps see the file's time
    old = open(path, newline='').read() if os.path.exists(path) else None
    if old != text:
        open(path, 'w', newline='\n').write(text)


def main():
    files = spec_read.read_all()
    known = re.findall(r'typedef struct \w+ (\w+);', open(TYPES_H, encoding='latin-1').read())
    os.makedirs(OUT, exist_ok=True)
    stems, new_types, values = [], [], []
    moved = [f for f in sorted(files) if f not in UNMOVED]
    for f in moved:
        g = Gen(files, known)
        for t in files[f]:
            g.gen_type(t)
        new_types += [c for c in g.new_types if c not in new_types]
        stem = re.sub(r'\.cs$', '', f).replace('/', '_')
        stems.append(stem)
        out = ['// GENERATED by scripts/build/gen_api.py from spec/game/%s - not to be edited: the spec is changed, then' % f,
               '// sh dll/build.sh. What each name is: gen_api.py\'s header; the classes, fields and functions: the spec.']
        # the plain values (offsets, addresses, slots, numbers) into values.c: included early, the module's own
        # helpers (patching.c's lists, the scenario editor's classes) read them before the rest of gen/ comes
        plain = re.compile(r'^internal uint32_t \w+(\[\d+\])?;$')
        values += ['', '// %s' % f] + [l for l in g.lines if plain.match(l)]
        out += [l for l in g.lines if not plain.match(l)]
        out += ['', '// the running build\'s values (spec_select, with addr_select): how many it hasn\'t got',
                'internal int spec_%s_select(int build) {' % stem, '    switch (build) {']
        missing = []
        for b, build in enumerate(spec_read.BUILDS):
            out.append('    case %d:   // %s' % (b, build))
            for name, vs, kind, _, cast in g.assign:
                out.append('        %s = %s;' % (name, fmt(vs[b], kind, cast)))
            gone = [label for _, vs, _, label, _ in g.assign if vs[b] == spec_read.NONE]
            out.append('        return %d;' % len(gone))
            missing.append(' '.join(gone))
        out += ['    }', '    return 0;', '}', '// the names a build hasn\'t got, by build',
                'internal const char *const k_spec_%s_missing[ADDR_BUILDS] = { %s };' % (stem, ', '.join('"%s"' % x for x in missing))]
        write(os.path.join(OUT, stem + '.c'), '\n'.join(out) + '\n')
    gen = ['// GENERATED by scripts/build/gen_api.py - the spec\'s C (spec/game/), included by game/api/api.c. spec_select',
           '// takes the running build\'s values (game_build, beside addr_select); spec_log names those it hasn\'t got.']
    if new_types:
        gen += ['', '// the classes types.h hasn\'t got'] + ['typedef struct %s %s;' % (c, c) for c in new_types]
    gen += ['']
    gen += ['#include "game/api/gen/%s.c"' % s for s in stems]
    gen += ['', '// g_spec_missing: exe.c\'s (game_build sets it)',
            'internal int spec_select(int build) { return %s; }' % ' + '.join('spec_%s_select(build)' % s for s in stems),
            'internal void spec_log(FILE *log) {',
            '    if (!log || !g_spec_missing) return;',
            '    fprintf(log, "# spec: build %d hasn\'t got %d:", g_exe_build, g_spec_missing);']
    gen += ['    if (*k_spec_%s_missing[g_exe_build]) fprintf(log, " %%s", k_spec_%s_missing[g_exe_build]);' % (s, s) for s in stems]
    gen += ['    fputc(10, log);', '}']
    write(os.path.join(OUT, 'gen.c'), '\n'.join(gen) + '\n')
    head = ['// GENERATED by scripts/build/gen_api.py - the spec\'s plain values (an offset, an address, a slot, a number),',
            '// each the running build\'s once spec_select has run; included by game/api/api.c ahead of the module\'s code.']
    write(os.path.join(OUT, 'values.c'), '\n'.join(head + values) + '\n')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Fail as e:
        print(e)
        sys.exit(1)
