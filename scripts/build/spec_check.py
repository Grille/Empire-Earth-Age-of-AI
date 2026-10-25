"""The spec's rules the C# compiler leaves open (spec/game/, its folders too; dotnet build in spec/ checks
the C# itself): a class's struct holds only what acts on its object - a static member (a function of no class's, its
object an argument) belongs in a static class (the file's own: unit.cs's unit); a member's types are the spec's own
names (int32, float32 ... - environment.cs's aliases), never C#'s (int, char: 16 bits, byte ...), so a type never means
one thing in the spec and another in C; every member says where it is ([offset] a struct's field, [address], [slot] or
[import] a function, [address] a static class's data); and a member with [was] has the values of that row of today's
tables (raw/functions.h, hooks.h, data.h, fields.h), build by build - the values were copied into the spec by hand,
a slip shows here (a list: its [address] sites in order). The spec's own files (spec/attributes.cs, environment.cs),
which define all this, aren't checked. Exit 1 with what is wrong, by file and line."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = os.path.join(ROOT, 'spec', 'game')   # the game's description (the spec's own: its parent)
RAW = os.path.join(ROOT, 'dll', 'game', 'api', 'raw')
CSHARP = r'\b(int|uint|short|ushort|long|ulong|float|double|decimal|byte|sbyte|char|bool|string|object)\b'
TYPE = re.compile(r'\b(?:(static)\s+(?:unsafe\s+)?class|struct)\s+(\w+)')
WHERE_FIELD = {False: ('offset',), True: ('address',)}
WHERE_FUNC = ('address', 'slot', 'import')
BUILDS = ('ee', 'aoc')                  # builds.h's order: the rows' columns
NONE = 0xFFFFFFFF                       # values.none; a row's NA, or 0 for an address the build hasn't got
NUM = r'0x[0-9A-Fa-f]+|\d+|NA'
VALUE_ATTR = re.compile(r'\b(offset|address|slot|import)\(((?:[^()]|\([^()]*\))*)\)')


def code(line):   # the line without its comment and its strings' contents
    return re.sub(r'//.*', '', re.sub(r'"[^"]*"', '""', line)).strip()


def num(s):
    return NONE if s in ('NA', 'none') else int(s, 0)


def raw_rows():   # today's tables: name -> (its attribute, value per build); a list's: [(value per build) per site]
    rows = {}
    for f in ('functions.h', 'hooks.h', 'data.h', 'fields.h'):
        src = open(os.path.join(RAW, f), encoding='latin-1').read().replace('\\\n', ' ')
        for m in re.finditer(r'\b([DO])\((\w+),\s*(%s),\s*(%s)\)' % (NUM, NUM), src):
            rows[m.group(2)] = (row_attr(m.group(1), m.group(2)), (num(m.group(3)), num(m.group(4))))
        func = r'\bF\((\w+),\s*[^,()]+,\s*\w+,\s*\((?:[^()]|\([^()]*\))*\),\s*(%s),\s*(%s)\)'   # name, ret, cc, (args)
        for m in re.finditer(func % (NUM, NUM), src):
            rows[m.group(1)] = ('address', (num(m.group(2)), num(m.group(3))))
        for m in re.finditer(r'\bL\((\w+),\s*\d+,\s*\(([^)]*)\),\s*\(([^)]*)\)\)', src):
            cols = [[num(v.strip()) for v in c.split(',')] for c in (m.group(2), m.group(3))]
            rows[m.group(1)] = ('address', list(zip(*cols)))
    return rows


def row_attr(kind, name):   # the attribute a row's value is in the spec
    if kind == 'O':
        return 'offset'
    return 'slot' if name.startswith('SLOT_') else 'import' if name.startswith('IAT_') else 'address'


def values(args):   # an attribute's values per build: (ee)x, (aoc)y - or one plain value, every build's
    tagged = dict(re.findall(r'\((\w+)\)\s*(0x[0-9A-Fa-f]+|\d+|none)', args))
    if tagged:
        return tuple(num(tagged[b]) if b in tagged else None for b in BUILDS)
    v = num(args.strip())
    return (v,) * len(BUILDS)


def same(spec, row):   # none in the spec is a row's NA or an address's 0
    return all(s == r or (s == NONE and r == 0) for s, r in zip(spec, row))


def check(path, rows):
    errors = []
    kind, attrs, texts, depth = None, [], [], 0
    for n, raw in enumerate(open(path, encoding='utf-8'), 1):
        line = code(raw)
        if not line:
            continue
        where = '%s:%d' % (os.path.relpath(path, ROOT).replace(os.sep, '/'), n)
        if depth == 0:
            m = TYPE.search(line)
            if m:
                kind = (m.group(2), bool(m.group(1)))
        if depth == 1 and line.startswith('['):
            attrs += re.findall(r'(?:\[|,\s*)@?(\w+)\s*(?:\(|\]|,)', line)
            texts.append(re.sub(r'//.*', '', raw))
        elif depth == 1 and line.endswith(';'):
            name, in_static = kind
            decl = re.sub(r'\[.*?\]', '', line)
            func = '(' in decl
            if 'static' in decl.split() and not in_static:
                errors.append('%s: %s is a class\'s struct - a static member (no object of its own) belongs in a '
                              'static class (the file\'s own)' % (where, name))
            for t in re.findall(CSHARP, decl):
                errors.append('%s: C#\'s %s - the spec\'s own type names (int32, uint8, float32 ...: environment.cs)'
                              % (where, t))
            need = WHERE_FUNC if func else WHERE_FIELD[in_static]
            if not any(a in need for a in attrs):
                errors.append('%s: says nowhere where it is (%s)' % (where, ' / '.join('[%s]' % a for a in need)))
            errors += check_was(where, ' '.join(texts), rows)
            attrs, texts = [], []
        depth += line.count('{') - line.count('}')
    return errors


def check_was(where, text, rows):
    was = re.search(r'\bwas\("(\w+)"\)', text)
    if not was:
        return []
    name = was.group(1)
    if name not in rows:
        return ['%s: was("%s") - no such row in raw/' % (where, name)]
    attr, row = rows[name]
    spec = [values(args) for a, args in VALUE_ATTR.findall(text) if a == attr]
    if isinstance(row, list):   # a list: one [address] per site
        if len(spec) != len(row) or not all(same(s, r) for s, r in zip(spec, row)):
            return ['%s: was("%s") - the sites %s, the row %s' % (where, name, fmt(spec), fmt(row))]
        return []
    if len(spec) != 1:
        return ['%s: was("%s") - %d [%s] in the spec for a row of one' % (where, name, len(spec), attr)]
    if not same(spec[0], row):
        return ['%s: was("%s") - the spec %s, the row %s' % (where, name, fmt(spec[0]), fmt(row))]
    return []


def fmt(v):
    if isinstance(v, list):
        return '[' + ', '.join(fmt(x) for x in v) + ']'
    shown = ('none' if x == NONE else '?' if x is None else hex(x) for x in v)
    return '(' + ', '.join('%s %s' % (b, x) for b, x in zip(BUILDS, shown)) + ')'


def main():
    rows = raw_rows()
    errors = []
    for top, dirs, files in sorted(os.walk(GAME)):
        dirs.sort()
        for f in sorted(files):
            if f.endswith('.cs'):
                errors += check(os.path.join(top, f), rows)
    for e in errors:
        print(e)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
