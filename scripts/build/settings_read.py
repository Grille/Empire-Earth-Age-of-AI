"""ageofai.ini's keys (dll_cs/settings/*.cs) read into Python: each section (a class with [section]) with its keys in
order - type, default, attributes and the /// lines verbatim (the file's lines above it) - the section's enums (a
CHOICE's names) and structs (a FLAGS row's fields), and its text before its header and after its last key. For
gen_settings.py. The format is dll_cs/settings/settings.cs's header; line by line as written: attributes in [...]
before a declaration on one line, its /// lines before it, an enum or a struct on one line."""
import os
import re

import spec_read

ROOT = spec_read.ROOT
SETTINGS = os.path.join(ROOT, 'dll_cs', 'settings')


class Section:
    def __init__(self, name, attrs, before, path):
        self.name, self.attrs, self.before, self.path = name, dict((a[0], a[1]) for a in attrs), before, path
        self.keys, self.enums, self.structs, self.trailer = [], {}, {}, []

    def has(self, attr):
        return attr in self.attrs

    @property
    def place(self):
        return self.attrs['section'][0][1]

    @property
    def title(self):
        return self.attrs['section'][1]

    @property
    def kind(self):   # rows.h's SETTING_SECTIONS kind
        return 'ALL' if self.has('all') else 'DIFF' if self.has('diff') else 'TEST' if self.has('test') else 'PLAIN'


class Key:
    def __init__(self, name, type_, default, attrs, doc, where):
        self.name, self.type, self.default, self.doc, self.where = name, type_, default, doc, where
        self.attrs = dict((a[0], a[1]) for a in attrs)

    def has(self, attr):
        return attr in self.attrs


def arg_text(a):   # an attribute's argument as written: a string's text, a number, a name (nameof(x) -> x)
    if isinstance(a, str):   # its escapes undone: "_aimod\\config" -> _aimod\config
        return re.sub(r'\\(.)', r'\1', a)
    if a[0] == 'num':
        return str(a[1])
    return a[1].split('.')[-1]   # nameof(x): x; limits.MAX_PATH: MAX_PATH


def kind_of(sec, k):   # rows.h's kind of a key
    if k.has('child'):
        return 'CHILD'
    if k.type == 'bool':
        return 'FLAG'
    if k.type in sec.enums:
        return 'CHOICE'
    if k.type in sec.structs:
        return 'FLAGS'
    if k.type == 'string':
        return 'STR_P' if k.has('per_player') else 'STR'
    if k.type == 'int':
        for a in ('pct', 'share', 'num'):
            if k.has(a):
                return a.upper()
        if k.has('range'):
            return 'INT'
    raise SystemExit('%s: %s %s: no kind (an int needs [range], [num], [pct] or [share] - settings.cs)' % (k.where, k.type, k.name))


def read_file(path):
    secs, doc, attr_text, depth, cur = [], [], '', 0, None
    rel = os.path.relpath(path, ROOT).replace(os.sep, '/')
    for n, raw in enumerate(open(path, encoding='utf-8'), 1):
        line = raw.strip()
        if line.startswith('///'):
            text = line[3:]
            doc.append(text[1:] if text.startswith(' ') else text)
            continue
        code = re.sub(r'^((?:[^"/]|"(?:[^"\\]|\\.)*"|/(?!/))*)//.*$', r'\1', line).strip()   # a // outside strings
        if not code:
            if not line.startswith('//'):
                doc = []
            continue
        where = '%s:%d' % (rel, n)
        if depth == 1 and cur:
            m = re.match(r'^enum\s+(\w+)\s*\{(.*)\}$', code)
            if m:
                names = {}   # the C# member -> its name in the file, in order
                for one in spec_read.split_top(m.group(2)):
                    a = re.match(r'^((?:\[[^\]]*\]\s*)*)@?(\w+)$', one.strip())
                    named = dict((x[0], x[1]) for x in spec_read.attrs_of(a.group(1)))
                    names[a.group(2)] = arg_text(named['name'][0]) if 'name' in named else a.group(2)
                cur.enums[m.group(1)] = names
                doc = []
                continue
            m = re.match(r'^struct\s+(\w+)\s*\{(.*)\}$', code)
            if m:
                fields = []
                for decl in m.group(2).split(';'):
                    d = re.match(r'^(\w+)\s+(.+)$', decl.strip())
                    if d:
                        fields += [(d.group(1), f.strip()) for f in d.group(2).split(',')]
                cur.structs[m.group(1)] = (fields, doc)
                doc = []
                continue
            if code == '}':   # the class ends: the /// lines left are the text after its last key
                cur.trailer, doc = doc, []
                depth -= 1
                continue
        while code.startswith('['):
            m = re.match(r'^\[(?:[^\[\]"]|"[^"]*")*\]\s*', code)
            attr_text += ' ' + m.group(0)
            code = code[m.end():]
        if not code:
            continue
        if depth == 0:
            m = re.search(r'\bclass\s+@?(\w+)', code)
            if m and 'Attribute' not in code and 'section' in attr_text:
                cur = Section(m.group(1), spec_read.attrs_of(attr_text), doc, path)
                secs.append(cur)
            attr_text, doc = '', []
        elif depth == 1 and cur and code.endswith(';'):
            m = re.match(r'^(\w+)\s+@?(\w+)\s*(?:=\s*(.+))?;$', code)
            if not m:
                raise SystemExit('%s: a key is "type name = default;": %s' % (where, code))
            cur.keys.append(Key(m.group(2), m.group(1), m.group(3), spec_read.attrs_of(attr_text), doc, where))
            attr_text, doc = '', []
        depth += code.count('{') - code.count('}')
    return secs


def read_all():   # [Section] of every file in dll_cs/settings/, in the file's order (by place; the test ini's last)
    out = []
    for f in sorted(os.listdir(SETTINGS)):
        if f.endswith('.cs'):
            out += read_file(os.path.join(SETTINGS, f))
    return sorted(out, key=lambda s: (s.kind == 'TEST', s.place))
