"""The spec (spec/game/*.cs) read into Python: its types (a class's struct, a static class) with their attributes and
members, each member with its attributes, doc comment, C# type and parameters. For gen_api.py, which makes C of it;
the spec's own rules are spec_check.py's and the C# compiler's (dotnet build in spec/). Line by line, as the spec is
written: attributes in [...] on the lines before a declaration, a declaration on one line ending in ;."""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = os.path.join(ROOT, 'spec', 'game')
BUILDS = ('ee', 'aoc')   # builds.h's order: k_exe_builds' columns
NONE = 0xFFFFFFFF        # environment.cs's values.none


class Type:
    def __init__(self, name, static, attrs, doc, path, line):
        self.name, self.static, self.attrs, self.doc, self.path, self.line = name, static, attrs, doc, path, line
        self.members = []

    def attr(self, name):
        return next((a for a in self.attrs if a[0] == name), None)


class Member:
    def __init__(self, name, ctype, params, func, static, fixed, attrs, doc, where):
        self.name, self.type, self.params, self.func, self.static = name, ctype, params, func, static
        self.fixed, self.attrs, self.doc, self.where = fixed, attrs, doc, where

    def attr(self, name):
        return next((a for a in self.attrs if a[0] == name), None)

    def all(self, name):
        return [a for a in self.attrs if a[0] == name]


def split_top(s, sep=','):   # s split at sep outside (), [] and strings
    out, depth, cur, quote = [], 0, '', False
    for ch in s:
        if ch == '"':
            quote = not quote
        elif not quote and ch in '([':
            depth += 1
        elif not quote and ch in ')]':
            depth -= 1
        if ch == sep and depth == 0 and not quote:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def arg(a):
    """an attribute's argument: 'text' for a string, ('build', n) for (ee)n, ('type', T) for typeof(T), ('name', x)
    for nameof(x), ('num', n), ('word', w) (an enum member: c, none ...)"""
    a = a.strip()
    m = re.match(r'^\w+:\s*(.*)$', a)   # a named argument: its value
    if m and not a.startswith('"'):
        a = m.group(1)
    if a.startswith('"'):
        return a[1:-1]
    m = re.match(r'^\((\w+)\)\s*(\w+)$', a)
    if m:
        return ('build', m.group(1), NONE if m.group(2) == 'none' else int(m.group(2), 0))
    m = re.match(r'^(typeof|nameof)\((\w+)(?:\.\w+)?\)$', a)
    if m:
        return ('type' if m.group(1) == 'typeof' else 'name', m.group(2))
    if re.match(r'^-?(0x[0-9A-Fa-f]+|\d+)$', a):
        return ('num', int(a, 0))
    return ('word', a)


def attrs_of(text):   # [a(x), b] [c] -> [('a', [x]), ('b', []), ('c', [])]
    out = []
    for group in re.findall(r'\[((?:[^\[\]"]|"[^"]*")*)\]', text):
        for one in split_top(group):
            m = re.match(r'^@?(\w+)\s*(?:\((.*)\))?$', one, re.S)
            out.append((m.group(1), [arg(x) for x in split_top(m.group(2) or '')]))
    return out


def values(attr):
    """an attribute's value per build: [v_ee, v_aoc] - one plain number is every build's, (ee)x, (aoc)y each its own
    (the first number of the attribute; the strings before it are its names)"""
    nums = [a for a in attr[1] if isinstance(a, tuple) and a[0] in ('num', 'build')]
    if len(nums) == 1 and nums[0][0] == 'num':
        return [nums[0][1]] * len(BUILDS)
    by = {a[1]: a[2] for a in nums if a[0] == 'build'}
    return [by.get(b, NONE) for b in BUILDS]


def decl(text):   # 'extern float32 max_speed(float32* pt)' -> (name, type, params, func, static, fixed)
    words = text.rstrip(';').strip()
    static = bool(re.search(r'\bstatic\b', words))
    fixed = None
    words = re.sub(r'\b(public|static|unsafe|extern|readonly)\s+', '', words)
    m = re.match(r'^fixed\s+(.+?)\s*(\w+)\s*\[\s*(\w+)\s*\]$', words)
    if m:
        return m.group(2), m.group(1).replace(' ', ''), None, False, static, int(m.group(3), 0)
    m = re.match(r'^(.+?)\s*\b(\w+)\s*\((.*)\)$', words)
    if m:
        params = []
        for p in split_top(m.group(3)):
            pm = re.match(r'^(.+?)\s*(\w+)$', p)
            params.append((pm.group(1).replace(' ', ''), pm.group(2)))
        return m.group(2), m.group(1).replace(' ', ''), params, True, static, None
    m = re.match(r'^(.+?)\s*\b(\w+)$', words)
    return m.group(2), m.group(1).replace(' ', ''), None, False, static, fixed


def read_file(path):
    types, doc, attr_text, depth, cur = [], [], '', 0, None
    for n, raw in enumerate(open(path, encoding='utf-8'), 1):
        line = raw.strip()
        if line.startswith('///'):
            doc.append(line[3:].strip())
            continue
        code = re.sub(r'//.*', '', re.sub(r'"[^"]*"', lambda m: m.group(0).replace('//', '\0'), line)).replace('\0', '//').strip()
        if not code:
            if not line.startswith('//'):
                doc = []
            continue
        while code.startswith('['):   # attributes, alone on the line or before what they belong to
            m = re.match(r'^\[(?:[^\[\]"]|"[^"]*")*\]\s*', code)
            attr_text += ' ' + m.group(0)
            code = code[m.end():]
        if not code:
            continue
        if depth == 0:
            m = re.search(r'\b(?:(static)\s+(?:unsafe\s+)?class|struct)\s+(\w+)', code)
            if m:
                cur = Type(m.group(2), bool(m.group(1)), attrs_of(attr_text), doc, path, n)
                types.append(cur)
                attr_text, doc = '', []
        elif depth == 1 and code.endswith(';'):
            name, ctype, params, func, static, fixed = decl(code)
            where = '%s:%d' % (os.path.relpath(path, ROOT).replace(os.sep, '/'), n)
            cur.members.append(Member(name, ctype, params, func, static, fixed, attrs_of(attr_text), doc, where))
            attr_text, doc = '', []
        depth += code.count('{') - code.count('}')
    return types


def read_all():   # {file name: [types]} of every file in spec/game/ and its folders
    out = {}
    for top, dirs, files in os.walk(GAME):
        dirs.sort()
        for f in sorted(files):
            if f.endswith('.cs'):
                out[os.path.relpath(os.path.join(top, f), GAME).replace(os.sep, '/')] = read_file(os.path.join(top, f))
    return out
