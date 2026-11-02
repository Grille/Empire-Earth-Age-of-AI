"""ageofai.ini's keys (dll_cs/settings/*.cs) read into Python: each section (a class with [section]) with its keys in
order - type, default, attributes and the /// lines verbatim (the file's lines above it) - the section's enums (a
CHOICE's names) and structs (a FLAGS row's fields), and its text before its header and after its last key. For
gen_settings.py. The format is dll_cs/settings/settings.cs's header; line by line as written: attributes in [...]
on their own line above a declaration (as spec/), its /// lines above them, an enum or a struct on one line."""
import os
import re

import spec_read

ROOT = spec_read.ROOT
SETTINGS = os.path.join(ROOT, 'dll_cs', 'settings')


class Section:
    def __init__(self, name, attrs, before, path):
        self.name, self.attrs, self.before, self.path = name, dict((a[0], a[1]) for a in attrs), before, path
        self.keys, self.enums, self.structs, self.trailer = [], {}, {}, []
        self.docs = {}   # an enum's or struct's {member or field: its /// lines} (written as a block: a line each)

    def has(self, attr):
        return attr in self.attrs

    @property
    def place(self):
        return self.attrs['section'][0][1]

    @property
    def title(self):   # ageofai_config.exe's page: [title("t")], else the class's name (ai_base: Ai base)
        return arg_text(self.attrs['title'][0]) if 'title' in self.attrs else title_of(self.name)

    @property
    def kind(self):   # rows.h's SETTING_SECTIONS kind
        return 'ALL' if self.has('all') else 'DIFF' if self.has('diff') else 'TEST' if self.has('test') else 'PLAIN'


class Key:
    def __init__(self, name, type_, default, attrs, doc, where):
        self.name, self.type, self.default, self.doc, self.where = name, type_, default, doc, where
        self.attrs = dict((a[0], a[1]) for a in attrs)
        self.master = None   # a member of a [master] group: the master's key

    def has(self, attr):
        return attr in self.attrs


MASTER_TYPE = 'master'   # a [master] group's own key: a CHOICE of MASTER_VALUES
MASTER_VALUES = {'default': 'each its shipped default (a key below set in the file is not used)',
                 'custom': 'each as set below, else its default',
                 'enabled': 'every part on (a factor: its default)',
                 'disabled': "every part off (a factor: 100%, the game's own)"}


def snake(ident):   # ImprovedUnitMovement -> improved_unit_movement
    return re.sub(r'(?<=[a-z0-9])(?=[A-Z])', '_', ident).lower()


def title_of(ident):   # a name as a title: a section's or key's snake_case, a group's PascalCase (CitizensUnderAttack:
    # Citizens under attack; a word in capitals stays: AIChat - AI chat)
    words = re.findall(r'[A-Z]+(?![a-z])|[A-Z]?[a-z0-9]+', ident) if ident[:1].isupper() else ident.split('_')
    t = ' '.join(w if w.isupper() and len(w) > 1 else w.lower() for w in words)
    return t[:1].upper() + t[1:]


def percent(x, where):   # a share's fraction as the whole percents rows.h keeps: 0.35 -> 35
    from decimal import Decimal
    d = Decimal(str(x)) * 100
    if d != d.to_integral_value():
        raise SystemExit('%s: %s is no whole percent' % (where, x))
    return str(int(d))


def arg_text(a):   # an attribute's argument as written: a string's text, a number, a name (nameof(x) -> x)
    if isinstance(a, str):   # its escapes undone: "_aimod\\config" -> _aimod\config
        return re.sub(r'\\(.)', r'\1', a)
    if a[0] == 'num':
        return str(a[1])
    if re.match(r'^-?\d+\.\d+$', a[1]):   # a decimal (a share's range: 0.25)
        return a[1]
    return a[1].split('.')[-1]   # nameof(x): x; limits.MAX_PATH: MAX_PATH


def kind_of(sec, k):   # rows.h's kind of a key
    if k.type == MASTER_TYPE:
        return 'CHOICE'
    if k.type == 'factor':
        return 'FACTOR'
    if k.type == 'bool':
        return 'FLAG'
    if k.type in sec.enums:
        return 'CHOICE'
    if k.type in sec.structs:
        return 'FLAGS'
    if k.type == 'string':
        return 'STR_P' if k.has('per_player') else 'STR'
    if k.type == 'share':
        return 'SHARE'
    if k.type == 'int':
        if not k.has('range'):
            return 'NUM'
        return 'LIMIT' if k.attrs['range'][2:] == [('word', 'true')] else 'INT'
    raise SystemExit('%s: %s %s: no kind (settings.cs)' % (k.where, k.type, k.name))


def read_file(path):
    secs, doc, attr_text, depth, cur, group = [], [], '', 0, None, None   # group: the title of a group just begun
    master = None   # the [master] group's key its keys follow, while in it
    inner = None   # an enum or struct written as a block: [kind, name, its doc, {member: file name} or [fields], {member: doc}]
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
        if inner:   # a member (enum: name, - struct: bool name;) with its /// lines and attributes above it
            if code == '{':
                continue
            if code == '}':
                kind, name, sdoc, members, docs = inner
                if name in cur.enums or name in cur.structs:
                    raise SystemExit('%s: a second %s in [%s] - a type\'s name is one of its section' % (where, name, cur.name))
                if kind == 'enum':
                    cur.enums[name] = members
                else:
                    cur.structs[name] = (members, sdoc)
                cur.docs[name], inner, doc, attr_text = docs, None, [], ''
                continue
            while code.startswith('['):
                m = re.match(r'^\[(?:[^\[\]"]|"[^"]*")*\]\s*', code)
                attr_text += ' ' + m.group(0)
                code = code[m.end():]
            if not code:
                continue
            if inner[0] == 'enum':
                m = re.match(r'^@?(\w+),?$', code)
                if not m:
                    raise SystemExit('%s: an enum member is "name,": %s' % (where, code))
                named = dict((x[0], x[1]) for x in spec_read.attrs_of(attr_text))
                inner[3][m.group(1)] = arg_text(named['name'][0]) if 'name' in named else m.group(1)
                inner[4][m.group(1)] = doc
            else:
                m = re.match(r'^(\w+)\s+(\w+);$', code)
                if not m:
                    raise SystemExit('%s: a struct field is "type name;": %s' % (where, code))
                inner[3].append((m.group(1), m.group(2)))
                inner[4][m.group(2)] = doc
            doc, attr_text = [], ''
            continue
        if depth in (1, 2) and cur:   # a type, above the key it is of (in its group too): the section's
            m = re.match(r'^(enum|struct)\s+(\w+)$', code)
            if m:   # a block: its members on the lines below, each with its doc
                inner = [m.group(1), m.group(2), doc, {} if m.group(1) == 'enum' else [], {}]
                doc = []
                continue
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
        if depth == 1 and cur:
            if code == '}':   # the class ends: the /// lines left are the text after its last key
                cur.trailer, doc = doc, []
                depth -= 1
                continue
        if depth == 2 and cur and code == '}':   # a group ends
            master = None
            if doc:
                raise SystemExit('%s: /// lines at the end of a group - they belong before a key, or after the group' % where)
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
        elif depth == 1 and cur and re.match(r'^class\s+@?\w+$', code):   # a group: ageofai_config.exe's topic, from its first key on
            name = re.match(r'^class\s+@?(\w+)$', code).group(1)
            named = dict((a[0], a[1]) for a in spec_read.attrs_of(attr_text))
            group = arg_text(named['title'][0]) if 'title' in named else title_of(name)
            if 'master' in named:   # its own key first, its /// lines the class's
                k = Key(snake(name), MASTER_TYPE, MASTER_TYPE + '.default', [], doc, where)
                k.attrs['group'], group = [group], None
                cur.keys.append(k)
                cur.enums[MASTER_TYPE] = {v: v for v in MASTER_VALUES}
                cur.docs[MASTER_TYPE] = {v: [t] for v, t in MASTER_VALUES.items()}
                master = k.name
            attr_text, doc = '', []
        elif depth in (1, 2) and cur and code.endswith(';'):
            m = re.match(r'^(\w+)\s+@?(\w+)\s*(?:=\s*(.+))?;$', code)
            if not m:
                raise SystemExit('%s: a key is "type name = default;": %s' % (where, code))
            k = Key(m.group(2), m.group(1), m.group(3), spec_read.attrs_of(attr_text), doc, where)
            if group is not None:
                k.attrs['group'], group = [group], None
            if master:
                if k.type not in ('bool', 'factor', 'int'):
                    raise SystemExit('%s: %s is a %s - a [master] group holds bool, factor and int keys only' % (where, k.name, k.type))
                k.master = master
            cur.keys.append(k)
            attr_text, doc = '', []
        depth += code.count('{') - code.count('}')
    return secs


def read_all():   # [Section] of every file in dll_cs/settings/, in the file's order (by place; the test ini's last)
    out = []
    for f in sorted(os.listdir(SETTINGS)):
        if f.endswith('.cs'):
            out += read_file(os.path.join(SETTINGS, f))
    return sorted(out, key=lambda s: (s.kind == 'TEST', s.place))
