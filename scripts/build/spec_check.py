"""The spec's rules the C# compiler leaves open (spec/game/, its folders too; dotnet build in spec/ checks
the C# itself): a class's struct holds only what acts on its object - a static member (a function of no class's, its
object an argument) belongs in a static class (the file's own: unit.cs's unit); a member's types are the spec's own
names (int32, float32 ... - environment.cs's aliases), never C#'s (int, char: 16 bits, byte ...), so a type never means
one thing in the spec and another in C; every member says where it is ([offset] a struct's field, [address], [slot] or
[import] a function, [address] or [value] a static class's data); a [guess] says its evidence in its doc comment.
The spec's own files (spec/attributes.cs, environment.cs),
which define all this, aren't checked. Exit 1 with what is wrong, by file and line."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = os.path.join(ROOT, 'spec', 'game')   # the game's description (the spec's own: its parent)
CSHARP = r'\b(int|uint|short|ushort|long|ulong|float|double|decimal|byte|sbyte|char|bool|string|object)\b'
TYPE = re.compile(r'\b(?:(static)\s+(?:unsafe\s+)?class|struct)\s+(\w+)')
WHERE_FIELD = {False: ('offset',), True: ('address', 'value')}
WHERE_FUNC = ('address', 'slot', 'import')
def code(line):   # the line without its comment and its strings' contents
    return re.sub(r'//.*', '', re.sub(r'"[^"]*"', '""', line)).strip()


def check(path):
    errors = []
    kind, attrs, depth, doc = None, [], 0, False
    for n, raw in enumerate(open(path, encoding='utf-8'), 1):
        doc = doc or raw.strip().startswith('///')
        line = code(raw)
        if not line:
            continue
        where = '%s:%d' % (os.path.relpath(path, ROOT).replace(os.sep, '/'), n)
        if depth == 0:
            m = TYPE.search(line)
            if m:
                kind = (m.group(2), bool(m.group(1)))
            doc = False   # the class's own comment, not its first member's
        if depth == 1 and line.startswith('['):
            attrs += re.findall(r'(?:\[|,\s*)@?(\w+)\s*(?:\(|\]|,)', line)
        elif depth == 1 and line.endswith(';'):
            name, in_static = kind
            decl = re.sub(r'@\w+', 'x', re.sub(r'\[.*?\]', '', line))   # @string: a name, not C#'s type
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
            if 'guess' in attrs and not doc:
                errors.append('%s: a [guess] without its evidence (a /// comment: what the code does with it)' % where)
            attrs, doc = [], False
        depth += line.count('{') - line.count('}')
    return errors


def main():
    errors = []
    for top, dirs, files in sorted(os.walk(GAME)):
        dirs.sort()
        for f in sorted(files):
            if f.endswith('.cs'):
                errors += check(os.path.join(top, f))
    for e in errors:
        print(e)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
