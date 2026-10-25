"""The language's objects (lang/*.cs) read into Python: each class (a kind of object, names.cs's names) with its
members - a field or a method, its type, parameters (a default: optional), attributes and doc. For gen_lang.py (the C
tables), gen_eeai_readme.py and gen_vscode_grammar.py. The format is lang/language.cs's header; line by line as written:
attributes in [...] before a declaration on one line, its /// lines before it."""
import os
import re

import spec_read

ROOT = spec_read.ROOT
LANG = os.path.join(ROOT, 'lang')


class Kind:
    def __init__(self, name, static, attrs, path):
        self.name, self.static, self.attrs, self.path = name, static, attrs, path
        self.members = []

    def has(self, attr):
        return any(a[0] == attr for a in self.attrs)


class Member:
    def __init__(self, name, type_, params, attrs, doc, where):
        self.name, self.type, self.params, self.attrs, self.doc, self.where = name, type_, params, attrs, doc, where

    def has(self, attr):
        return any(a[0] == attr for a in self.attrs)

    @property
    def func(self):
        return self.params is not None

    @property
    def arity(self):   # (least, most) arguments; a field (-1, -1)
        if self.params is None:
            return -1, -1
        return sum(1 for p in self.params if not p[2]), len(self.params)


def decl(code, where):   # 'public extern int? war_minutes(player b, float minutes = 0);' -> (name, type, params)
    words = re.sub(r'\b(public|static|extern|readonly)\s+', '', code.rstrip(';').strip())
    m = re.match(r'^(.+?)\s+@?(\w+)\s*\((.*)\)$', words)
    if m:
        params = []
        for p in spec_read.split_top(m.group(3)):
            pm = re.match(r'^(.+?)\s+@?(\w+)\s*(=.*)?$', p.strip())
            if not pm:
                raise SystemExit('%s: a parameter is "type name" or "type name = default": %s' % (where, p))
            params.append((pm.group(1).replace(' ', ''), pm.group(2), bool(pm.group(3))))
        return m.group(2), m.group(1).replace(' ', ''), params
    m = re.match(r'^(.+?)\s+@?(\w+)$', words)
    if not m:
        raise SystemExit('%s: not a field or method declaration: %s' % (where, code))
    return m.group(2), m.group(1).replace(' ', ''), None


def read_file(path):
    kinds, doc, attr_text, depth, cur = [], [], '', 0, None
    for n, raw in enumerate(open(path, encoding='utf-8'), 1):
        line = raw.strip()
        if line.startswith('///'):
            doc.append(line[3:].strip())
            continue
        code = re.sub(r'//.*', '', line).strip()
        if not code:
            if not line.startswith('//'):
                doc = []
            continue
        while code.startswith('['):
            m = re.match(r'^\[(?:[^\[\]"]|"[^"]*")*\]\s*', code)
            attr_text += ' ' + m.group(0)
            code = code[m.end():]
        if not code:
            continue
        if depth == 0:
            m = re.search(r'\b(static\s+)?class\s+@?(\w+)', code)
            if m and 'Attribute' not in code:
                cur = Kind(m.group(2), bool(m.group(1)), spec_read.attrs_of(attr_text), path)
                kinds.append(cur)
            attr_text, doc = '', []
        elif depth == 1 and cur and code.endswith(';'):
            where = '%s:%d' % (os.path.relpath(path, ROOT).replace(os.sep, '/'), n)
            name, type_, params = decl(code, where)
            cur.members.append(Member(name, type_, params, spec_read.attrs_of(attr_text), ' '.join(doc), where))
            attr_text, doc = '', []
        depth += code.count('{') - code.count('}')
    return kinds


def read_all():   # {class name: Kind} of every file in lang/
    out = {}
    for f in sorted(os.listdir(LANG)):
        if f.endswith('.cs'):
            for k in read_file(os.path.join(LANG, f)):
                out[k.name] = k
    return out


def xm_kind(m):   # a player member's kind in relations.c's terms (XM_ ...): a value, a player, a relation, a method, an effect
    if m.has('relation') or m.has('with_them'):
        return 'XM_REL'
    if m.has('effect'):
        return 'XM_DO'
    if m.has('method'):
        return 'XM_FILTER'
    return 'XM_LINK' if m.type.rstrip('?') == 'player' else 'XM_PLAYER'
