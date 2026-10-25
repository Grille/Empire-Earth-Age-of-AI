"""The spec (spec/game/) for Ghidra, one build: its classes as structures (their fields at their offsets, a base's
fields first, the size [size] or the last field's end), its enums, its functions (by class: Class::member, a static
class's by its name; their convention and types) and its places (data, places in the code, vtables, call sites) as
labels, each with its doc - local/work/spec_<build>.json, which scripts/re/ghidra/ApplySpec.java puts into a Ghidra
project (README "Setup"). A value the build hasn't got is left out; an engine DLL's class ([dll]) too.
usage: python scripts/re/spec_ghidra.py [ee|aoc]"""
import json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build'))
import spec_read

SIZES = {'int8': 1, 'uint8': 1, 'char8': 1, 'int16': 2, 'uint16': 2, 'int32': 4, 'uint32': 4, 'float32': 4,
         'int64': 8, 'uint64': 8, 'float64': 8, 'nint': 4, 'nuint': 4}
LAYOUT = {'deque': 0x14, 'vector': 0xC, 'ustring': 0x10, 'wstring': 0x10}   # their bytes (environment.cs, types.h)
VALUES = {'vec2': [('x', 'float32'), ('y', 'float32')], 'vec3': [('x', 'float32'), ('y', 'float32'), ('z', 'float32')],
          'ivec2': [('x', 'int32'), ('y', 'int32')]}


def main():
    build = sys.argv[1] if len(sys.argv) > 1 else 'ee'
    b = spec_read.BUILDS.index(build)
    files = spec_read.read_all()
    types = {t.name: t for ts in files.values() for t in ts}

    def ty(cs):   # the spec's type as the JSON says it: int32, ptr:T, struct:T, enum:T, bytes:n, void
        stars = len(cs) - len(cs.rstrip('*'))
        base = cs.rstrip('*')
        if base == 'cstr':
            base, stars = 'char8', stars + 1
        if base in SIZES or base == 'void':
            t = base
        elif base in VALUES:
            t = 'struct:' + base
        elif base in LAYOUT:
            t = 'bytes:%d' % LAYOUT[base]
        elif base in ('block', 'code', 'vftable'):
            t = 'bytes:0'
        elif base in types and types[base].enum:
            t = 'enum:' + base
        elif base in types and not types[base].static:
            t = 'struct:' + base
        else:
            t = 'void' if stars else 'bytes:4'
        for _ in range(stars):
            t = 'ptr:' + t
        return t

    def val(attr):
        v = spec_read.values(attr)[b]
        return None if v == spec_read.NONE else v

    def doc(m):
        return '\n'.join(m.doc)

    out = {'build': build, 'enums': [], 'structs': [], 'functions': [], 'labels': []}
    out['structs'] += [{'name': n, 'size': 4 * len(f), 'doc': '', 'fields': [{'name': x, 'offset': 4 * k, 'type': t}
                        for k, (x, t) in enumerate(f)]} for n, f in VALUES.items()]

    def fields_of(t):   # its fields with a base's first (the struct starts with the base's)
        a = t.attr('base')
        out = fields_of(types[a[1][0][1]]) if a and a[1][0][1] in types else []
        for m in t.members:
            if m.func or m.static or not m.attr('offset'):
                continue
            off = val(m.attr('offset'))
            if off is None:
                continue
            tt = ty(m.type)
            if m.fixed:
                tt = 'array:%s:%d' % (tt, m.fixed)
            out.append({'name': m.name, 'offset': off, 'type': tt, 'doc': doc(m)})
        return out

    for f, ts in sorted(files.items()):
        for t in ts:
            if t.attr('dll'):
                continue
            if t.enum:
                v, vals = -1, []
                for n, x in t.values:
                    v = x if x is not None else v + 1
                    vals.append([n, v])
                if vals:
                    out['enums'].append({'name': t.name, 'size': 4, 'values': vals, 'doc': '\n'.join(t.doc)})
                continue
            ns = t.name
            if not t.static:
                size = t.attr('size')
                out['structs'].append({'name': t.name, 'size': val(size) if size else None, 'doc': '\n'.join(t.doc),
                                       'fields': fields_of(t)})
                vt = t.attr('vtable')
                if vt and val(vt):
                    out['labels'].append({'address': val(vt), 'namespace': ns, 'name': 'vftable', 'doc': ''})
            for m in t.members:
                if m.func:
                    conv = 'thiscall' if not t.static else next((a[0] for a in m.attrs if a[0] in ('cdecl', 'stdcall', 'thiscall')), 'cdecl')
                    params = [{'name': n, 'type': ty(c)} for c, n in m.params]
                    if not t.static:
                        params.insert(0, {'name': 'this', 'type': 'ptr:struct:' + t.name})
                    for a in m.all('address'):
                        if val(a):
                            out['functions'].append({'address': val(a), 'namespace': ns, 'name': m.name, 'convention': conv,
                                                     'ret': ty(m.type), 'params': params, 'doc': doc(m)})
                    sites = m.all('call_site')
                    for k, a in enumerate(sites):
                        many = sum(1 for x in sites if x[1][0] == a[1][0]) > 1
                        if val(a):
                            out['labels'].append({'address': val(a), 'namespace': ns,
                                                  'name': 'call_%s' % a[1][0] + ('_%d' % [x for x in sites if x[1][0] == a[1][0]].index(a) if many else ''),
                                                  'doc': 'calls %s::%s (the mod: patch_%s)' % (ns, m.name, a[1][0])})
                elif t.static:
                    addrs = m.all('address')
                    for k, a in enumerate(addrs):
                        if val(a):
                            out['labels'].append({'address': val(a), 'namespace': ns,
                                                  'name': m.name + ('_%d' % k if len(addrs) > 1 else ''), 'doc': doc(m)})
    # each field's length and each structure's size (an embedded structure's before its holder's - Ghidra needs it)
    by = {x['name']: x for x in out['structs']}
    enums = {x['name'] for x in out['enums']}

    def length(tt):
        if tt.startswith('ptr:') or tt.startswith('enum:'):
            return 4
        if tt.startswith('bytes:'):
            return int(tt[6:])
        if tt.startswith('array:'):
            inner, n = tt[6:].rsplit(':', 1)
            return length(inner) * int(n)
        if tt.startswith('struct:'):
            return size_of(by[tt[7:]])
        return SIZES.get(tt, 4)

    def size_of(st):
        if st.get('done'):
            return st['size']
        for fl in st['fields']:
            fl['length'] = length(fl['type'])
        end = max([fl['offset'] + fl['length'] for fl in st['fields']] + [1])
        st['size'] = max(st['size'] or 0, end)
        st['done'] = True
        return st['size']

    for st in out['structs']:
        size_of(st)
    for st in out['structs']:
        del st['done']
    path = os.path.join(spec_read.ROOT, 'local', 'work', 'spec_%s.json' % build)
    json.dump(out, open(path, 'w'), indent=1)
    print('%s: %d structures, %d enums, %d functions, %d labels' % (path, len(out['structs']), len(out['enums']),
                                                                   len(out['functions']), len(out['labels'])))


if __name__ == '__main__':
    main()
